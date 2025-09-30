#include "FlowFieldManager.h"

#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "LandscapeProxy.h"
#include "Containers/ArrayView.h"
#include "Misc/ScopeLock.h"
#include "HAL/IConsoleManager.h"
#include "Async/AsyncWork.h"
#include "Stats/Stats.h"

namespace FlowField
{
        struct FTraversalEvaluationContext
        {
                TWeakObjectPtr<UWorld> World;
                TWeakObjectPtr<const AActor> ManagerActor;
                FIntPoint GridSize = FIntPoint::ZeroValue;
                float CellSize = 100.0f;
                FVector Origin = FVector::ZeroVector;
                uint8 DefaultWeight = 255;
                bool bTraceTerrain = false;
                float TraceStartZ = 0.0f;
                float TraceEndZ = 0.0f;
                float MaxSlopeAngleDegrees = 45.0f;
                TEnumAsByte<ECollisionChannel> TerrainChannel = ECC_WorldStatic;
                TEnumAsByte<ECollisionChannel> ObstacleChannel = ECC_GameTraceChannel2;
                float ObstacleHalfHeight = 100.0f;
                float ObstacleInflation = 0.0f;
                float ObstacleOffsetZ = 0.0f;
                TConstArrayView<uint8> ManualObstacleMask;
                TArray<uint8> ManualObstacleMaskStorage;
        };

        static uint8 EvaluateCell(const FTraversalEvaluationContext& Context, int32 CellIndex, bool& bOutBlockedByObstacle)
        {
                bOutBlockedByObstacle = false;

                if (!Context.GridSize.X || !Context.GridSize.Y)
                {
                        return 0;
                }

                if (Context.ManualObstacleMask.IsValidIndex(CellIndex) && Context.ManualObstacleMask[CellIndex] != 0)
                {
                        bOutBlockedByObstacle = true;
                        return 0;
                }

                const int32 CellX = CellIndex % Context.GridSize.X;
                const int32 CellY = CellIndex / Context.GridSize.X;
                const FVector CellCenter = Context.Origin + FVector((CellX + 0.5f) * Context.CellSize, (CellY + 0.5f) * Context.CellSize, 0.0f);

                bool bWalkableTerrain = true;
                UWorld* World = Context.World.Get();

                if (Context.bTraceTerrain && World)
                {
                        const FVector TraceStart(CellCenter.X, CellCenter.Y, Context.TraceStartZ);
                        const FVector TraceEnd(CellCenter.X, CellCenter.Y, Context.TraceEndZ);
                        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FlowFieldTerrainTrace), false, Context.ManagerActor.Get());

                        FHitResult Hit;
                        if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, Context.TerrainChannel, QueryParams))
                        {
                                const FVector Normal = Hit.ImpactNormal.GetSafeNormal();
                                const float Dot = FMath::Clamp(FVector::DotProduct(Normal, FVector::UpVector), -1.0f, 1.0f);
                                const float SlopeDegrees = FMath::RadiansToDegrees(FMath::Acos(Dot));
                                bWalkableTerrain = SlopeDegrees <= Context.MaxSlopeAngleDegrees;
                        }
                        else
                        {
                                bWalkableTerrain = false;
                        }
                }

                if (!bWalkableTerrain)
                {
                        return 0;
                }

                if (World)
                {
                        const FVector ObstacleCenter = CellCenter + FVector(0.0f, 0.0f, Context.ObstacleOffsetZ);
                        const FVector Extents(Context.CellSize * 0.5f + Context.ObstacleInflation, Context.CellSize * 0.5f + Context.ObstacleInflation, Context.ObstacleHalfHeight);
                        const FCollisionShape Shape = FCollisionShape::MakeBox(Extents);
                        FCollisionQueryParams ObstacleParams(SCENE_QUERY_STAT(FlowFieldObstacleOverlap), false, Context.ManagerActor.Get());

                        if (World->OverlapBlockingTestByChannel(ObstacleCenter, FQuat::Identity, Context.ObstacleChannel, Shape, ObstacleParams))
                        {
                                bOutBlockedByObstacle = true;
                                return 0;
                        }
                }

                return Context.DefaultWeight;
        }
}

namespace
{
        static int32 GFlowUpdateMaxCells = 2048;
        static FAutoConsoleVariableRef CVarFlowUpdateMaxCells(TEXT("flow.update.maxcells"), GFlowUpdateMaxCells, TEXT("Maximum number of flow field cells processed per frame."));

        static int32 GFlowUpdateAsync = 1;
        static FAutoConsoleVariableRef CVarFlowUpdateAsync(TEXT("flow.update.async"), GFlowUpdateAsync, TEXT("When set to 1 traversal weights are computed on background threads."));

        static int32 GFlowDebugGrid = -1;
        static FAutoConsoleVariableRef CVarFlowDebugGrid(TEXT("flow.debug.grid"), GFlowDebugGrid, TEXT("Toggle grid debug drawing."));

        static int32 GFlowDebugDirections = -1;
        static FAutoConsoleVariableRef CVarFlowDebugDirections(TEXT("flow.debug.directions"), GFlowDebugDirections, TEXT("Toggle flow direction debug drawing."));

        static int32 GFlowDebugCosts = -1;
        static FAutoConsoleVariableRef CVarFlowDebugCosts(TEXT("flow.debug.costs"), GFlowDebugCosts, TEXT("Toggle integration cost debug drawing."));
}

class FTraversalWeightBuildTask : public FNonAbandonableTask
{
public:
        FTraversalWeightBuildTask(FlowField::FTraversalEvaluationContext&& InContext, TArray<int32>&& InIndices)
                : Context(MoveTemp(InContext))
                , Indices(MoveTemp(InIndices))
        {
        }

        void DoWork()
        {
                const int32 Count = Indices.Num();
                Weights.SetNum(Count);
                ObstacleMask.SetNum(Count);

                for (int32 Index = 0; Index < Count; ++Index)
                {
                        bool bBlocked = false;
                        const uint8 Weight = FlowField::EvaluateCell(Context, Indices[Index], bBlocked);
                        Weights[Index] = Weight;
                        ObstacleMask[Index] = bBlocked ? 1 : 0;
                }
        }

        FORCEINLINE TStatId GetStatId() const
        {
                RETURN_QUICK_DECLARE_CYCLE_STAT(FTraversalWeightBuildTask, STATGROUP_ThreadPoolAsyncTasks);
        }

        struct FResult
        {
                TArray<int32> Indices;
                TArray<uint8> Weights;
                TArray<uint8> ObstacleMask;
        };

        FResult ConsumeResult()
        {
                FResult Result;
                Result.Indices = MoveTemp(Indices);
                Result.Weights = MoveTemp(Weights);
                Result.ObstacleMask = MoveTemp(ObstacleMask);
                return Result;
        }

private:
        FlowField::FTraversalEvaluationContext Context;
        TArray<int32> Indices;
        TArray<uint8> Weights;
        TArray<uint8> ObstacleMask;
};

AFlowFieldManager::AFlowFieldManager()
{
        PrimaryActorTick.bCanEverTick = true;
        PrimaryActorTick.bStartWithTickEnabled = true;
}

void AFlowFieldManager::Tick(float DeltaSeconds)
{
        Super::Tick(DeltaSeconds);
        ServiceTraversalWeightUpdates(DeltaSeconds);
        DrawDebug();
}

void AFlowFieldManager::OnConstruction(const FTransform& Transform)
{
        Super::OnConstruction(Transform);
        UpdateFlowFieldSettings();
        UpdateTraversalWeights();
}

void AFlowFieldManager::BeginPlay()
{
        Super::BeginPlay();
        UpdateFlowFieldSettings();
        UpdateTraversalWeights();
}

bool AFlowFieldManager::BuildFlowFieldToCell(const FIntPoint& DestinationCell)
{
        EnsureTraversalWeightsUpToDate(true);

        if (FlowField.Build(DestinationCell))
        {
                bHasBuiltField = true;
                CachedDestinationWorld = FlowField.CellToWorld(DestinationCell);
                RefreshDebugSnapshot();
                return true;
        }

        bHasBuiltField = false;
        bHasDebugSnapshot = false;
        CachedDestinationWorld = FVector::ZeroVector;
        return false;
}

bool AFlowFieldManager::BuildFlowFieldToWorldLocation(const FVector& DestinationLocation)
{
        const FIntPoint Cell = FlowField.WorldToCell(DestinationLocation);
        return BuildFlowFieldToCell(Cell);
}

bool AFlowFieldManager::RebuildFlowFieldTo(const FVector& DestinationWorld)
{
        return BuildFlowFieldToWorldLocation(DestinationWorld);
}

FVector AFlowFieldManager::GetDirectionForWorldPosition(const FVector& WorldPosition) const
{
        if (!bHasBuiltField)
        {
                return FVector::ZeroVector;
        }

        return FlowField.GetDirectionForWorldPosition(WorldPosition);
}

void AFlowFieldManager::MarkObstacleBox(const FVector& Center, const FVector& Extents, bool bBlocked)
{
        if (!bTraversalInitialised)
        {
                return;
        }

        const int32 GridX = FlowFieldSettings.GridSize.X;
        const int32 GridY = FlowFieldSettings.GridSize.Y;
        if (GridX <= 0 || GridY <= 0)
        {
                return;
        }

        const FVector Min = Center - Extents;
        const FVector Max = Center + Extents;
        const FVector MaxAdjusted(Max.X - KINDA_SMALL_NUMBER, Max.Y - KINDA_SMALL_NUMBER, Max.Z);

        FIntPoint MinCell = FlowField.WorldToCell(Min);
        FIntPoint MaxCell = FlowField.WorldToCell(MaxAdjusted);
        MinCell = ClampCellToGrid(MinCell);
        MaxCell = ClampCellToGrid(MaxCell);

        if (MaxCell.X < MinCell.X || MaxCell.Y < MinCell.Y)
        {
                return;
        }

        {
                FScopeLock Lock(&TraversalDataLock);
                if (ManualObstacleMask.Num() != GridX * GridY)
                {
                        ManualObstacleMask.SetNumZeroed(GridX * GridY);
                }

                for (int32 Y = MinCell.Y; Y <= MaxCell.Y; ++Y)
                {
                        for (int32 X = MinCell.X; X <= MaxCell.X; ++X)
                        {
                                const int32 Index = Y * GridX + X;
                                ManualObstacleMask[Index] = bBlocked ? 1 : 0;
                        }
                }
        }

        MarkRegionDirty(MinCell, MaxCell);
}

void AFlowFieldManager::UpdateFlowFieldSettings()
{
        CancelPendingAsyncTask();
        RecalculateTerrainBounds();

        FIntPoint DesiredGridSize = GridSize;
        FVector DesiredOrigin = GetActorLocation();

        if (bAutoSizeToTerrain && CachedTerrainBounds.IsValid)
        {
                const FVector BoundsSize = CachedTerrainBounds.GetSize();
                DesiredGridSize.X = FMath::Max(1, FMath::CeilToInt(BoundsSize.X / CellSize));
                DesiredGridSize.Y = FMath::Max(1, FMath::CeilToInt(BoundsSize.Y / CellSize));
                DesiredOrigin = CachedTerrainBounds.Min;
        }
        else
        {
                const FVector GridHalfExtent = FVector(GridSize.X * CellSize * 0.5f, GridSize.Y * CellSize * 0.5f, 0.0f);
                DesiredOrigin = GetActorLocation() - GridHalfExtent;
        }

        GridSize = DesiredGridSize;

        FlowFieldSettings.GridSize = DesiredGridSize;
        FlowFieldSettings.CellSize = CellSize;

        FVector FinalOrigin = DesiredOrigin + AdditionalOriginOffset;
        FinalOrigin.Z = DesiredOrigin.Z + AdditionalOriginOffset.Z;
        FlowFieldSettings.Origin = FinalOrigin;

        FlowFieldSettings.bAllowDiagonal = bAllowDiagonal;
        FlowFieldSettings.DiagonalCostMultiplier = DiagonalCostMultiplier;
        FlowFieldSettings.StepCost = StepCost;
        FlowFieldSettings.CellTraversalWeightMultiplier = CellTraversalWeightMultiplier;
        FlowFieldSettings.HeuristicWeight = HeuristicWeight;
        FlowFieldSettings.bSmoothDirections = bSmoothDirections;

        FlowField.ApplySettings(FlowFieldSettings);
        bHasBuiltField = false;
        bHasDebugSnapshot = false;
        bTraversalInitialised = false;
}

void AFlowFieldManager::UpdateTraversalWeights()
{
        CancelPendingAsyncTask();

        const int32 NumCells = FlowFieldSettings.GridSize.X * FlowFieldSettings.GridSize.Y;
        if (NumCells <= 0)
        {
                TraversalWeights.Reset();
                ObstacleDebugMask.Reset();
                {
                        FScopeLock Lock(&TraversalDataLock);
                        ManualObstacleMask.Reset();
                }
                DirtyCellMask.Empty();
                TQueue<int32> EmptyQueue;
                Swap(DirtyCellQueue, EmptyQueue);
                bTraversalInitialised = false;
                FlowField.SetTraversalWeights(TraversalWeights);
                return;
        }

        const uint8 DefaultWeightValue = static_cast<uint8>(FMath::Clamp(DefaultTraversalWeight, 0, 255));
        TraversalWeights.Init(DefaultWeightValue, NumCells);
        ObstacleDebugMask.Init(0, NumCells);
        {
                FScopeLock Lock(&TraversalDataLock);
                if (ManualObstacleMask.Num() != NumCells)
                {
                        ManualObstacleMask.SetNumZeroed(NumCells);
                }
        }

        DirtyCellMask.Init(false, NumCells);
        TQueue<int32> EmptyQueue;
        Swap(DirtyCellQueue, EmptyQueue);

        FlowField.SetTraversalWeights(TraversalWeights);
        bHasBuiltField = false;
        bHasDebugSnapshot = false;
        bTraversalInitialised = true;
        bTraversalWeightsDirtyForField = false;

        MarkAllCellsDirty();

        if (GFlowUpdateAsync == 0)
        {
                EnsureTraversalWeightsUpToDate(true);
        }
}

void AFlowFieldManager::ServiceTraversalWeightUpdates(float DeltaSeconds)
{
        UE_UNUSED(DeltaSeconds);
        ConsumeTraversalTask();

        if (!bTraversalInitialised)
        {
                return;
        }

        const int32 MaxCellsPerUpdate = FMath::Max(1, GFlowUpdateMaxCells);

        if (GFlowUpdateAsync != 0)
        {
                if (!ActiveTraversalTask.IsValid() && !DirtyCellQueue.IsEmpty())
                {
                        TArray<int32> BatchIndices;
                        BatchIndices.Reserve(MaxCellsPerUpdate);

                        int32 CellIndex = INDEX_NONE;
                        while (BatchIndices.Num() < MaxCellsPerUpdate && DirtyCellQueue.Dequeue(CellIndex))
                        {
                                if (DirtyCellMask.IsValidIndex(CellIndex))
                                {
                                        DirtyCellMask[CellIndex] = false;
                                }

                                if (!IsCellIndexValid(CellIndex))
                                {
                                        continue;
                                }

                                BatchIndices.Add(CellIndex);
                        }

                        if (BatchIndices.Num() > 0)
                        {
                                FlowField::FTraversalEvaluationContext Context = BuildTraversalContext(true);
                                ActiveTraversalTask = MakeUnique<FAsyncTask<FTraversalWeightBuildTask>>(MoveTemp(Context), MoveTemp(BatchIndices));
                                ActiveTraversalTask->StartBackgroundTask();
                        }
                }
        }
        else if (!DirtyCellQueue.IsEmpty())
        {
                ProcessDirtyCells(MaxCellsPerUpdate);
        }

        ConsumeTraversalTask();

        if (bTraversalWeightsDirtyForField)
        {
                FlushTraversalWeightsToField();
        }
}

void AFlowFieldManager::MarkAllCellsDirty()
{
        if (!bTraversalInitialised)
        {
                return;
        }

        const int32 NumCells = FlowFieldSettings.GridSize.X * FlowFieldSettings.GridSize.Y;
        DirtyCellMask.Init(false, NumCells);

        TQueue<int32> EmptyQueue;
        Swap(DirtyCellQueue, EmptyQueue);

        for (int32 Index = 0; Index < NumCells; ++Index)
        {
                DirtyCellMask[Index] = true;
                DirtyCellQueue.Enqueue(Index);
        }
}

void AFlowFieldManager::MarkRegionDirty(const FIntPoint& MinCell, const FIntPoint& MaxCell)
{
        if (!bTraversalInitialised)
        {
                return;
        }

        for (int32 Y = MinCell.Y; Y <= MaxCell.Y; ++Y)
        {
                if (Y < 0 || Y >= FlowFieldSettings.GridSize.Y)
                {
                        continue;
                }

                for (int32 X = MinCell.X; X <= MaxCell.X; ++X)
                {
                        if (X < 0 || X >= FlowFieldSettings.GridSize.X)
                        {
                                continue;
                        }

                        const int32 Index = Y * FlowFieldSettings.GridSize.X + X;
                        MarkCellDirty(Index);
                }
        }
}

void AFlowFieldManager::MarkCellDirty(int32 LinearIndex)
{
        if (!IsCellIndexValid(LinearIndex))
        {
                return;
        }

        if (!DirtyCellMask.IsValidIndex(LinearIndex))
        {
                DirtyCellMask.Init(false, TraversalWeights.Num());
        }

        if (DirtyCellMask[LinearIndex])
        {
                return;
        }

        DirtyCellMask[LinearIndex] = true;
        DirtyCellQueue.Enqueue(LinearIndex);
}

int32 AFlowFieldManager::ProcessDirtyCells(int32 MaxCellsToProcess)
{
        if (!bTraversalInitialised || MaxCellsToProcess <= 0)
        {
                return 0;
        }

        FlowField::FTraversalEvaluationContext Context = BuildTraversalContext(false);

        int32 Processed = 0;
        int32 CellIndex = INDEX_NONE;
        while (Processed < MaxCellsToProcess && DirtyCellQueue.Dequeue(CellIndex))
        {
                if (DirtyCellMask.IsValidIndex(CellIndex))
                {
                        DirtyCellMask[CellIndex] = false;
                }

                if (!IsCellIndexValid(CellIndex))
                {
                        continue;
                }

                bool bBlockedByObstacle = false;
                const uint8 Weight = FlowField::EvaluateCell(Context, CellIndex, bBlockedByObstacle);
                ApplyTraversalWeight(CellIndex, Weight, bBlockedByObstacle);
                ++Processed;
        }

        if (Processed > 0)
        {
                FlushTraversalWeightsToField();
        }

        return Processed;
}

void AFlowFieldManager::FlushTraversalWeightsToField()
{
        FlowField.SetTraversalWeights(TraversalWeights);
        bTraversalWeightsDirtyForField = false;
        bHasBuiltField = false;
        bHasDebugSnapshot = false;
}

void AFlowFieldManager::CancelPendingAsyncTask()
{
        if (ActiveTraversalTask.IsValid())
        {
                        ActiveTraversalTask->EnsureCompletion();
                        ConsumeTraversalTask();
                        ActiveTraversalTask.Reset();
        }
}

void AFlowFieldManager::ConsumeTraversalTask()
{
        if (!ActiveTraversalTask.IsValid() || !ActiveTraversalTask->IsDone())
        {
                return;
        }

        FTraversalWeightBuildTask::FResult Result = ActiveTraversalTask->GetTask().ConsumeResult();
        ActiveTraversalTask.Reset();

        const int32 Count = Result.Indices.Num();
        for (int32 Index = 0; Index < Count; ++Index)
        {
                const int32 CellIndex = Result.Indices[Index];
                const uint8 Weight = Result.Weights.IsValidIndex(Index) ? Result.Weights[Index] : 0;
                const bool bBlocked = Result.ObstacleMask.IsValidIndex(Index) && Result.ObstacleMask[Index] != 0;
                ApplyTraversalWeight(CellIndex, Weight, bBlocked);
        }
}

bool AFlowFieldManager::HasPendingTraversalUpdates() const
{
        if (!DirtyCellQueue.IsEmpty())
        {
                return true;
        }

        if (ActiveTraversalTask.IsValid() && !ActiveTraversalTask->IsDone())
        {
                return true;
        }

        return false;
}

void AFlowFieldManager::EnsureTraversalWeightsUpToDate(bool bForceSync)
{
        if (!bTraversalInitialised)
        {
                return;
        }

        if (bForceSync)
        {
                if (ActiveTraversalTask.IsValid())
                {
                        ActiveTraversalTask->EnsureCompletion();
                        ConsumeTraversalTask();
                }

                const int32 NumCells = FlowFieldSettings.GridSize.X * FlowFieldSettings.GridSize.Y;
                while (!DirtyCellQueue.IsEmpty())
                {
                        ProcessDirtyCells(NumCells);

                        if (ActiveTraversalTask.IsValid())
                        {
                                ActiveTraversalTask->EnsureCompletion();
                                ConsumeTraversalTask();
                        }
                }
        }
        else
        {
                ConsumeTraversalTask();
        }

        if (bTraversalWeightsDirtyForField)
        {
                FlushTraversalWeightsToField();
        }
}

bool AFlowFieldManager::IsCellIndexValid(int32 Index) const
{
        return Index >= 0 && TraversalWeights.IsValidIndex(Index);
}

FIntPoint AFlowFieldManager::ClampCellToGrid(const FIntPoint& Cell) const
{
        const int32 MaxX = FlowFieldSettings.GridSize.X - 1;
        const int32 MaxY = FlowFieldSettings.GridSize.Y - 1;
        return FIntPoint(FMath::Clamp(Cell.X, 0, MaxX), FMath::Clamp(Cell.Y, 0, MaxY));
}

uint8 AFlowFieldManager::SampleTraversalWeightForCell(int32 CellIndex, bool& bOutBlockedByObstacle) const
{
        FlowField::FTraversalEvaluationContext Context = BuildTraversalContext(true);
        return FlowField::EvaluateCell(Context, CellIndex, bOutBlockedByObstacle);
}

void AFlowFieldManager::ApplyTraversalWeight(int32 CellIndex, uint8 Weight, bool bBlockedByObstacle)
{
        if (!TraversalWeights.IsValidIndex(CellIndex))
        {
                return;
        }

        TraversalWeights[CellIndex] = Weight;
        if (ObstacleDebugMask.IsValidIndex(CellIndex))
        {
                ObstacleDebugMask[CellIndex] = bBlockedByObstacle ? 1 : 0;
        }

        bTraversalWeightsDirtyForField = true;
        bHasBuiltField = false;
        bHasDebugSnapshot = false;
}

FlowField::FTraversalEvaluationContext AFlowFieldManager::BuildTraversalContext(bool bCopyManualObstacles) const
{
        FlowField::FTraversalEvaluationContext Context;
        Context.World = GetWorld();
        Context.ManagerActor = this;
        Context.GridSize = FlowFieldSettings.GridSize;
        Context.CellSize = FlowFieldSettings.CellSize;
        Context.Origin = FlowFieldSettings.Origin;
        Context.DefaultWeight = static_cast<uint8>(FMath::Clamp(DefaultTraversalWeight, 0, 255));
        Context.bTraceTerrain = bAutoSizeToTerrain && CachedTerrainBounds.IsValid;
        if (Context.bTraceTerrain)
        {
                Context.TraceStartZ = CachedTerrainBounds.Max.Z + TerrainTraceHeight;
                Context.TraceEndZ = CachedTerrainBounds.Min.Z - TerrainTraceDepth;
        }
        Context.MaxSlopeAngleDegrees = MaxWalkableSlopeAngle;
        Context.TerrainChannel = TerrainCollisionChannel;
        Context.ObstacleChannel = ObstacleCollisionChannel;
        Context.ObstacleHalfHeight = ObstacleQueryHalfHeight;
        Context.ObstacleInflation = ObstacleQueryInflation;
        Context.ObstacleOffsetZ = ObstacleQueryOffsetZ;

        {
                FScopeLock Lock(&TraversalDataLock);
                if (bCopyManualObstacles)
                {
                        Context.ManualObstacleMaskStorage = ManualObstacleMask;
                        Context.ManualObstacleMask = Context.ManualObstacleMaskStorage;
                }
                else
                {
                        Context.ManualObstacleMask = MakeArrayView(ManualObstacleMask);
                }
        }

        return Context;
}

void AFlowFieldManager::RefreshDebugSnapshot()
{
        if (!bEnableDebugDraw)
        {
                bHasDebugSnapshot = false;
                return;
        }

        DebugSnapshot = FlowField.CreateDebugSnapshot();
        bHasDebugSnapshot = true;
}

void AFlowFieldManager::DrawDebug() const
{
        if (!bEnableDebugDraw)
        {
                return;
        }

        UWorld* World = GetWorld();
        if (!World)
        {
                return;
        }

        const FIntPoint Size = bHasDebugSnapshot ? DebugSnapshot.GridSize : FlowFieldSettings.GridSize;
        if (Size.X <= 0 || Size.Y <= 0)
        {
                return;
        }

        const float CellSizeLocal = bHasDebugSnapshot ? DebugSnapshot.CellSize : FlowFieldSettings.CellSize;
        const FVector Origin = bHasDebugSnapshot ? DebugSnapshot.Origin : FlowFieldSettings.Origin;
        const float ArrowLength = CellSizeLocal * DirectionArrowScale;
        const FVector UpOffset(0.0f, 0.0f, DebugHeightOffset);

        auto ResolveDebugToggle = [](bool bDefault, int32 CVarValue)
        {
                return (CVarValue < 0) ? bDefault : (CVarValue != 0);
        };

        const bool bShouldDrawGrid = ResolveDebugToggle(bDrawGrid, GFlowDebugGrid);
        const bool bShouldDrawDirections = ResolveDebugToggle(bDrawDirections, GFlowDebugDirections);
        const bool bShouldDrawCosts = ResolveDebugToggle(bDrawIntegrationValues, GFlowDebugCosts);

        if (bShouldDrawGrid)
        {
                const FVector Base = Origin + UpOffset;
                for (int32 X = 0; X <= Size.X; ++X)
                {
                        const float OffsetX = static_cast<float>(X) * CellSizeLocal;
                        const FVector Start = Base + FVector(OffsetX, 0.0f, 0.0f);
                        const FVector End = Start + FVector(0.0f, Size.Y * CellSizeLocal, 0.0f);
                        DrawDebugLine(World, Start, End, GridColor, false, 0.0f, 0, DebugLineThickness);
                }

                for (int32 Y = 0; Y <= Size.Y; ++Y)
                {
                        const float OffsetY = static_cast<float>(Y) * CellSizeLocal;
                        const FVector Start = Base + FVector(0.0f, OffsetY, 0.0f);
                        const FVector End = Start + FVector(Size.X * CellSizeLocal, 0.0f, 0.0f);
                        DrawDebugLine(World, Start, End, GridColor, false, 0.0f, 0, DebugLineThickness);
                }
        }

        if (!bHasDebugSnapshot)
        {
                return;
        }

        const int32 NumCells = Size.X * Size.Y;
        for (int32 Index = 0; Index < NumCells; ++Index)
        {
                const int32 CellX = Index % Size.X;
                const int32 CellY = Index / Size.X;

                const FVector CellCenter = Origin + FVector((CellX + 0.5f) * CellSizeLocal, (CellY + 0.5f) * CellSizeLocal, 0.0f) + UpOffset;

                const bool bIsWalkable = DebugSnapshot.WalkableField.IsValidIndex(Index) && DebugSnapshot.WalkableField[Index] > 0;
                const bool bIsObstacle = ObstacleDebugMask.IsValidIndex(Index) && ObstacleDebugMask[Index] != 0;

                if (bShouldDrawDirections)
                {
                        if (DebugSnapshot.DirectionField.IsValidIndex(Index))
                        {
                                const FVector Direction = FVector(DebugSnapshot.DirectionField[Index], 0.0f);
                                if (!Direction.IsNearlyZero())
                                {
                                        const FVector Start = CellCenter;
                                        const FVector End = Start + Direction.GetSafeNormal() * ArrowLength;
                                        DrawDebugDirectionalArrow(World, Start, End, 25.0f, DirectionColor, false, 0.0f, 0, DebugLineThickness);
                                }
                        }
                }

                if (bShouldDrawCosts)
                {
                        FString Label;
                        if (bIsWalkable && DebugSnapshot.IntegrationField.IsValidIndex(Index))
                        {
                                const float Cost = DebugSnapshot.IntegrationField[Index];
                                if (Cost < TNumericLimits<float>::Max())
                                {
                                        Label = FString::Printf(TEXT("%.1f"), Cost);
                                }
                                else
                                {
                                        Label = TEXT("∞");
                                }
                        }
                        else
                        {
                                Label = TEXT("X");
                        }

                        const FColor TextColor = bIsWalkable ? IntegrationTextColor : BlockedCellColor;
                        DrawDebugString(World, CellCenter, Label, nullptr, TextColor, 0.0f, true, DebugTextScale);

                        if (bIsObstacle)
                        {
                                const FVector Extent(CellSizeLocal * 0.5f, CellSizeLocal * 0.5f, 10.0f);
                                DrawDebugSolidBox(World, CellCenter, Extent, ObstacleDebugColor, false, 0.0f, 0);
                        }
                }
                else if (bIsObstacle || (bHighlightBlockedCells && !bIsWalkable))
                {
                        const FVector Extent(CellSizeLocal * 0.5f, CellSizeLocal * 0.5f, 10.0f);
                        const FColor Color = bIsObstacle ? ObstacleDebugColor : BlockedCellColor;
                        DrawDebugSolidBox(World, CellCenter, Extent, Color, false, 0.0f, 0);
                }
        }

        if (bDrawDestination && bHasBuiltField)
        {
                const FVector DestinationLocation = CachedDestinationWorld + UpOffset;
                DrawDebugSphere(World, DestinationLocation, DestinationMarkerRadius, 16, DirectionColor, false, 0.0f, 0, DebugLineThickness);
        }
}

void AFlowFieldManager::RecalculateTerrainBounds()
{
        CachedTerrainBounds = FBox(ForceInit);

        if (!bAutoSizeToTerrain)
        {
                return;
        }

        FBox DetectedBounds(ForceInit);

        auto AddActorBounds = [&DetectedBounds](AActor* Actor)
        {
                if (!IsValid(Actor))
                {
                        return;
                }

                const FBox ActorBounds = Actor->GetComponentsBoundingBox(true);
                if (ActorBounds.IsValid)
                {
                        DetectedBounds += ActorBounds;
                }
        };

        for (TObjectPtr<AActor> ActorPtr : TerrainActors)
        {
                AddActorBounds(ActorPtr.Get());
        }

        UWorld* World = GetWorld();

        if (!DetectedBounds.IsValid && World)
        {
                for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
                {
                        AddActorBounds(*It);
                }
        }

        if (!DetectedBounds.IsValid)
        {
                if (AActor* Parent = GetAttachParentActor())
                {
                        AddActorBounds(Parent);
                }
        }

        if (DetectedBounds.IsValid)
        {
                CachedTerrainBounds = DetectedBounds.ExpandBy(TerrainBoundsPadding);
        }
}

