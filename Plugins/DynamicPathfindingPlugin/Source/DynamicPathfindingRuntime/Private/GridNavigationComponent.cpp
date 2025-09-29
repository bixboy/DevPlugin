#include "GridNavigationComponent.h"
#include "DrawDebugHelpers.h"
#include "Async/Async.h"
#include "Containers/Queue.h"
#include "Engine/World.h"

UGridNavigationComponent::UGridNavigationComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    GridSize = FIntVector(64, 64, 8);
    VoxelSize = 100.f;
    GridOrigin = FVector::ZeroVector;
    DirtyPropagationDepth = 6;
    bDrawDebug = false;
    DebugDrawDuration = 0.1f;

    NumVoxels = 0;
    bHasValidFlowField = false;
    bHasDirtyRegion = false;
    bPendingFullRebuild = true;
    bAsyncTaskRunning = false;
    bDestroying = false;
}

void UGridNavigationComponent::BeginPlay()
{
    Super::BeginPlay();

    InitializeGrid();
    RequestFlowFieldRebuild(true);
}

void UGridNavigationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bDestroying = true;
    Super::EndPlay(EndPlayReason);
}

void UGridNavigationComponent::InitializeGrid()
{
    NumVoxels = GridSize.X * GridSize.Y * GridSize.Z;
    WalkableFlags.Init(1, NumVoxels);
    FlowFieldDirections.Init(FVector::ZeroVector, NumVoxels);
    DistanceField.Init(TNumericLimits<int32>::Max(), NumVoxels);
}

void UGridNavigationComponent::SetTargetLocation(const FVector& InTargetLocation, bool bImmediate /*= false*/)
{
    TargetLocation = InTargetLocation;
    RequestFlowFieldRebuild(true);
    if (bImmediate)
    {
        LaunchFlowFieldTask();
    }
}

void UGridNavigationComponent::SetVoxelBlocked(const FIntVector& Coordinates, bool bBlocked)
{
    if (!IsValidCoordinates(Coordinates))
    {
        return;
    }

    const int32 Index = GetIndex(Coordinates);
    {
        FWriteScopeLock WriteLock(DataLock);
        WalkableFlags[Index] = bBlocked ? 0 : 1;
    }

    ExpandDirtyRegion(Coordinates);
    RequestFlowFieldRebuild(false);
}

FVector UGridNavigationComponent::GetFlowDirectionAtLocation(const FVector& WorldLocation) const
{
    if (NumVoxels == 0)
    {
        return FVector::ZeroVector;
    }

    const float InvVoxelSize = 1.f / FMath::Max(VoxelSize, 1.f);
    FIntVector Coordinates;
    Coordinates.X = FMath::FloorToInt((WorldLocation.X - GridOrigin.X) * InvVoxelSize);
    Coordinates.Y = FMath::FloorToInt((WorldLocation.Y - GridOrigin.Y) * InvVoxelSize);
    Coordinates.Z = FMath::FloorToInt((WorldLocation.Z - GridOrigin.Z) * InvVoxelSize);

    if (!IsValidCoordinates(Coordinates))
    {
        return FVector::ZeroVector;
    }

    const int32 Index = GetIndex(Coordinates);

    FReadScopeLock ReadLock(DataLock);
    if (!bHasValidFlowField || !FlowFieldDirections.IsValidIndex(Index))
    {
        return FVector::ZeroVector;
    }

    return FlowFieldDirections[Index];
}

bool UGridNavigationComponent::IsVoxelWalkable(const FIntVector& Coordinates) const
{
    if (!IsValidCoordinates(Coordinates))
    {
        return false;
    }

    const int32 Index = GetIndex(Coordinates);

    FReadScopeLock ReadLock(DataLock);
    return WalkableFlags.IsValidIndex(Index) && WalkableFlags[Index] != 0;
}

void UGridNavigationComponent::SetDebugEnabled(bool bEnabled)
{
    bDrawDebug = bEnabled;
}

void UGridNavigationComponent::DrawDebug()
{
    if (!bDrawDebug)
    {
        return;
    }

    DrawDebugInternal();
}

void UGridNavigationComponent::RequestFlowFieldRebuild(bool bForceFullRebuild)
{
    bPendingFullRebuild |= bForceFullRebuild;
    if (!bAsyncTaskRunning)
    {
        LaunchFlowFieldTask();
    }
}

void UGridNavigationComponent::LaunchFlowFieldTask()
{
    if (bAsyncTaskRunning || NumVoxels <= 0 || bDestroying)
    {
        return;
    }

    bAsyncTaskRunning = true;

    const bool bFullRebuild = bPendingFullRebuild;
    FIntVector RegionMin = FIntVector::ZeroValue;
    FIntVector RegionMax = GridSize - FIntVector(1, 1, 1);

    if (!bFullRebuild && bHasDirtyRegion)
    {
        RegionMin = DirtyMin - FIntVector(DirtyPropagationDepth);
        RegionMax = DirtyMax + FIntVector(DirtyPropagationDepth);
        RegionMin = RegionMin.ComponentMax(FIntVector::ZeroValue);
        RegionMax = RegionMax.ComponentMin(GridSize - FIntVector(1, 1, 1));
    }

    const FVector LocalTarget = TargetLocation;
    const int32 LocalNumVoxels = NumVoxels;
    const FIntVector LocalGridSize = GridSize;
    const float LocalVoxelSize = VoxelSize;
    const FVector LocalOrigin = GridOrigin;

    TArray<uint8> WalkableCopy;
    TArray<int32> ExistingDistance;
    TArray<FVector> ExistingFlow;
    {
        FReadScopeLock ReadLock(DataLock);
        WalkableCopy = WalkableFlags;
        ExistingDistance = DistanceField;
        ExistingFlow = FlowFieldDirections;
    }

    TWeakObjectPtr<UGridNavigationComponent> WeakThis(this);

    Async(EAsyncExecution::ThreadPool, [WeakThis, WalkableCopy = MoveTemp(WalkableCopy), ExistingDistance = MoveTemp(ExistingDistance), ExistingFlow = MoveTemp(ExistingFlow), LocalGridSize, LocalVoxelSize, LocalOrigin, RegionMin, RegionMax, LocalTarget, LocalNumVoxels, bFullRebuild]() mutable
    {
        if (!WeakThis.IsValid())
        {
            return;
        }

        TArray<int32> NewDistance = ExistingDistance;
        TArray<FVector> NewFlowField = ExistingFlow;

        if (bFullRebuild || NewDistance.Num() != LocalNumVoxels)
        {
            NewDistance.Init(TNumericLimits<int32>::Max(), LocalNumVoxels);
        }
        if (bFullRebuild || NewFlowField.Num() != LocalNumVoxels)
        {
            NewFlowField.Init(FVector::ZeroVector, LocalNumVoxels);
        }

        auto IndexFromCoords = [LocalGridSize](const FIntVector& Coords)
        {
            return (Coords.Z * LocalGridSize.Y * LocalGridSize.X) + (Coords.Y * LocalGridSize.X) + Coords.X;
        };

        auto IsValidCoord = [LocalGridSize](const FIntVector& Coords) -> bool
        {
            return Coords.X >= 0 && Coords.Y >= 0 && Coords.Z >= 0 &&
                   Coords.X < LocalGridSize.X && Coords.Y < LocalGridSize.Y && Coords.Z < LocalGridSize.Z;
        };

        auto GetCoordsFromIndex = [LocalGridSize](int32 Index)
        {
            const int32 XYSize = LocalGridSize.X * LocalGridSize.Y;
            const int32 Z = Index / XYSize;
            const int32 Remainder = Index % XYSize;
            const int32 Y = Remainder / LocalGridSize.X;
            const int32 X = Remainder % LocalGridSize.X;
            return FIntVector(X, Y, Z);
        };

        auto GetVoxelCenter = [LocalOrigin, LocalVoxelSize](const FIntVector& Coords)
        {
            return LocalOrigin + FVector(Coords) * LocalVoxelSize + FVector(LocalVoxelSize * 0.5f);
        };

        const bool bLimitRegion = !bFullRebuild;
        FIntVector LocalMinRegion = RegionMin;
        FIntVector LocalMaxRegion = RegionMax;

        const float InvVoxelSize = 1.f / FMath::Max(LocalVoxelSize, 1.f);
        FIntVector GoalCoords;
        GoalCoords.X = FMath::RoundToInt((LocalTarget.X - LocalOrigin.X) * InvVoxelSize);
        GoalCoords.Y = FMath::RoundToInt((LocalTarget.Y - LocalOrigin.Y) * InvVoxelSize);
        GoalCoords.Z = FMath::RoundToInt((LocalTarget.Z - LocalOrigin.Z) * InvVoxelSize);

        GoalCoords = GoalCoords.ComponentMax(FIntVector::ZeroValue);
        GoalCoords = GoalCoords.ComponentMin(LocalGridSize - FIntVector(1, 1, 1));

        const int32 GoalIndex = IndexFromCoords(GoalCoords);

        if (bLimitRegion)
        {
            LocalMinRegion = LocalMinRegion.ComponentMin(GoalCoords);
            LocalMaxRegion = LocalMaxRegion.ComponentMax(GoalCoords);
        }

        if (!WalkableCopy.IsValidIndex(GoalIndex) || WalkableCopy[GoalIndex] == 0)
        {
            AsyncTask(ENamedThreads::GameThread, [WeakThis]()
            {
                if (UGridNavigationComponent* StrongThis = WeakThis.Get())
                {
                    StrongThis->bAsyncTaskRunning = false;
                    StrongThis->bHasValidFlowField = false;
                }
            });
            return;
        }

        TQueue<int32> OpenSet;
        NewDistance[GoalIndex] = 0;
        OpenSet.Enqueue(GoalIndex);

        while (!OpenSet.IsEmpty())
        {
            int32 CurrentIndex;
            OpenSet.Dequeue(CurrentIndex);

            const FIntVector CurrentCoords = GetCoordsFromIndex(CurrentIndex);

            if (bLimitRegion)
            {
                if (CurrentCoords.X < LocalMinRegion.X || CurrentCoords.Y < LocalMinRegion.Y || CurrentCoords.Z < LocalMinRegion.Z ||
                    CurrentCoords.X > LocalMaxRegion.X || CurrentCoords.Y > LocalMaxRegion.Y || CurrentCoords.Z > LocalMaxRegion.Z)
                {
                    continue;
                }
            }

            const int32 CurrentDistance = NewDistance[CurrentIndex];
            static const FIntVector NeighborOffsets[6] = {
                FIntVector(1, 0, 0), FIntVector(-1, 0, 0),
                FIntVector(0, 1, 0), FIntVector(0, -1, 0),
                FIntVector(0, 0, 1), FIntVector(0, 0, -1)};

            for (const FIntVector& Offset : NeighborOffsets)
            {
                FIntVector NeighborCoords = CurrentCoords + Offset;
                if (!IsValidCoord(NeighborCoords))
                {
                    continue;
                }

                if (bLimitRegion)
                {
                    if (NeighborCoords.X < LocalMinRegion.X || NeighborCoords.Y < LocalMinRegion.Y || NeighborCoords.Z < LocalMinRegion.Z ||
                        NeighborCoords.X > LocalMaxRegion.X || NeighborCoords.Y > LocalMaxRegion.Y || NeighborCoords.Z > LocalMaxRegion.Z)
                    {
                        continue;
                    }
                }

                const int32 NeighborIndex = IndexFromCoords(NeighborCoords);
                if (!WalkableCopy.IsValidIndex(NeighborIndex) || WalkableCopy[NeighborIndex] == 0)
                {
                    continue;
                }

                const int32 NewCost = CurrentDistance + 1;
                if (NewCost < NewDistance[NeighborIndex])
                {
                    NewDistance[NeighborIndex] = NewCost;
                    OpenSet.Enqueue(NeighborIndex);
                }
            }
        }

        for (int32 Index = 0; Index < LocalNumVoxels; ++Index)
        {
            if (NewDistance[Index] == TNumericLimits<int32>::Max())
            {
                NewFlowField[Index] = FVector::ZeroVector;
                continue;
            }

            const FIntVector Coords = GetCoordsFromIndex(Index);
            if (bLimitRegion)
            {
                if (Coords.X < LocalMinRegion.X || Coords.Y < LocalMinRegion.Y || Coords.Z < LocalMinRegion.Z ||
                    Coords.X > LocalMaxRegion.X || Coords.Y > LocalMaxRegion.Y || Coords.Z > LocalMaxRegion.Z)
                {
                    continue;
                }
            }

            float BestNeighborCost = static_cast<float>(NewDistance[Index]);
            FVector BestDirection = FVector::ZeroVector;

            static const FIntVector NeighborOffsets2[6] = {
                FIntVector(1, 0, 0), FIntVector(-1, 0, 0),
                FIntVector(0, 1, 0), FIntVector(0, -1, 0),
                FIntVector(0, 0, 1), FIntVector(0, 0, -1)};

            for (const FIntVector& Offset : NeighborOffsets2)
            {
                FIntVector NeighborCoords = Coords + Offset;
                if (!IsValidCoord(NeighborCoords))
                {
                    continue;
                }

                const int32 NeighborIndex = IndexFromCoords(NeighborCoords);
                if (!WalkableCopy.IsValidIndex(NeighborIndex) || WalkableCopy[NeighborIndex] == 0)
                {
                    continue;
                }

                const int32 NeighborCost = NewDistance[NeighborIndex];
                if (NeighborCost < BestNeighborCost)
                {
                    BestNeighborCost = NeighborCost;
                    BestDirection = (GetVoxelCenter(NeighborCoords) - GetVoxelCenter(Coords)).GetSafeNormal();
                }
            }

            NewFlowField[Index] = BestDirection;
        }

        AsyncTask(ENamedThreads::GameThread, [WeakThis, Distance = MoveTemp(NewDistance), FlowField = MoveTemp(NewFlowField), bFullRebuild]() mutable
        {
            if (UGridNavigationComponent* StrongThis = WeakThis.Get())
            {
                StrongThis->CompleteFlowFieldTask(MoveTemp(Distance), MoveTemp(FlowField));
                StrongThis->bPendingFullRebuild = false;
                if (!bFullRebuild)
                {
                    StrongThis->ClearDirtyRegion();
                }
            }
        });
    });
}

void UGridNavigationComponent::CompleteFlowFieldTask(TArray<int32>&& InDistanceField, TArray<FVector>&& InFlowField)
{
    {
        FWriteScopeLock WriteLock(DataLock);
        DistanceField = MoveTemp(InDistanceField);
        FlowFieldDirections = MoveTemp(InFlowField);
        bHasValidFlowField = true;
    }

    bAsyncTaskRunning = false;

    if (bHasDirtyRegion || bPendingFullRebuild)
    {
        LaunchFlowFieldTask();
    }
}

bool UGridNavigationComponent::IsValidCoordinates(const FIntVector& Coordinates) const
{
    return Coordinates.X >= 0 && Coordinates.Y >= 0 && Coordinates.Z >= 0 &&
           Coordinates.X < GridSize.X && Coordinates.Y < GridSize.Y && Coordinates.Z < GridSize.Z;
}

int32 UGridNavigationComponent::GetIndex(const FIntVector& Coordinates) const
{
    return (Coordinates.Z * GridSize.Y * GridSize.X) + (Coordinates.Y * GridSize.X) + Coordinates.X;
}

FIntVector UGridNavigationComponent::GetCoordinatesFromIndex(int32 Index) const
{
    const int32 XYSize = GridSize.X * GridSize.Y;
    const int32 Z = Index / XYSize;
    const int32 Remainder = Index % XYSize;
    const int32 Y = Remainder / GridSize.X;
    const int32 X = Remainder % GridSize.X;
    return FIntVector(X, Y, Z);
}

FVector UGridNavigationComponent::GetVoxelCenter(const FIntVector& Coordinates) const
{
    return GridOrigin + FVector(Coordinates) * VoxelSize + FVector(VoxelSize * 0.5f);
}

void UGridNavigationComponent::ExpandDirtyRegion(const FIntVector& Coordinates)
{
    if (!bHasDirtyRegion)
    {
        DirtyMin = Coordinates;
        DirtyMax = Coordinates;
        bHasDirtyRegion = true;
        return;
    }

    DirtyMin = DirtyMin.ComponentMin(Coordinates);
    DirtyMax = DirtyMax.ComponentMax(Coordinates);
}

void UGridNavigationComponent::ClearDirtyRegion()
{
    bHasDirtyRegion = false;
}

void UGridNavigationComponent::DrawDebugInternal() const
{
    if (!GetWorld())
    {
        return;
    }

    const float HalfVoxel = VoxelSize * 0.5f;

    FReadScopeLock ReadLock(DataLock);
    if (!bHasValidFlowField)
    {
        return;
    }

    for (int32 Index = 0; Index < FlowFieldDirections.Num(); ++Index)
    {
        const FIntVector Coords = GetCoordinatesFromIndex(Index);
        const FVector Center = GetVoxelCenter(Coords);

        const bool bWalkable = WalkableFlags.IsValidIndex(Index) && WalkableFlags[Index] != 0;
        const FColor BoxColor = bWalkable ? FColor::Green : FColor::Red;

        DrawDebugBox(GetWorld(), Center, FVector(HalfVoxel), BoxColor, false, DebugDrawDuration, 0, 1.f);

        const FVector Direction = FlowFieldDirections[Index];
        if (!Direction.IsNearlyZero())
        {
            DrawDebugDirectionalArrow(GetWorld(), Center, Center + Direction * VoxelSize * 0.5f, 15.f, FColor::Blue, false, DebugDrawDuration, 0, 1.5f);
        }
    }
}

void UGridNavigationComponent::BuildFlowFieldData(const FIntVector& RegionMin, const FIntVector& RegionMax, TArray<int32>& OutDistance, TArray<FVector>& OutFlowField) const
{
    // Unused placeholder for potential future specialized implementations.
    // Flow field generation is handled directly inside LaunchFlowFieldTask for simplicity.
}
