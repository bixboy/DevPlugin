#include "FlowFieldManager.h"

#include "DrawDebugHelpers.h"

AFlowFieldManager::AFlowFieldManager()
{
        PrimaryActorTick.bCanEverTick = true;
        PrimaryActorTick.bStartWithTickEnabled = true;
}

void AFlowFieldManager::Tick(float DeltaSeconds)
{
        Super::Tick(DeltaSeconds);
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

FVector AFlowFieldManager::GetDirectionForWorldPosition(const FVector& WorldPosition) const
{
        if (!bHasBuiltField)
        {
                return FVector::ZeroVector;
        }

        return FlowField.GetDirectionForWorldPosition(WorldPosition);
}

void AFlowFieldManager::UpdateFlowFieldSettings()
{
        FlowFieldSettings.GridSize = GridSize;
        FlowFieldSettings.CellSize = CellSize;

        const FVector ActorLocation = GetActorLocation();
        const FVector GridHalfExtent = FVector(GridSize.X * CellSize * 0.5f, GridSize.Y * CellSize * 0.5f, 0.0f);
        FlowFieldSettings.Origin = ActorLocation - GridHalfExtent + AdditionalOriginOffset;
        FlowFieldSettings.Origin.Z = ActorLocation.Z + AdditionalOriginOffset.Z;

        FlowFieldSettings.bAllowDiagonal = bAllowDiagonal;
        FlowFieldSettings.DiagonalCostMultiplier = DiagonalCostMultiplier;
        FlowFieldSettings.StepCost = StepCost;
        FlowFieldSettings.CellTraversalWeightMultiplier = CellTraversalWeightMultiplier;
        FlowFieldSettings.HeuristicWeight = HeuristicWeight;
        FlowFieldSettings.bSmoothDirections = bSmoothDirections;

        FlowField.ApplySettings(FlowFieldSettings);
        bHasBuiltField = false;
        bHasDebugSnapshot = false;
}

void AFlowFieldManager::UpdateTraversalWeights()
{
        const int32 NumCells = FlowFieldSettings.GridSize.X * FlowFieldSettings.GridSize.Y;
        if (NumCells <= 0)
        {
                TraversalWeights.Reset();
                FlowField.SetTraversalWeights(TraversalWeights);
                return;
        }

        if (TraversalWeights.Num() != NumCells)
        {
                TraversalWeights.SetNum(NumCells);
        }

        const uint8 WeightValue = static_cast<uint8>(FMath::Clamp(DefaultTraversalWeight, 0, 255));
        for (int32 Index = 0; Index < TraversalWeights.Num(); ++Index)
        {
                TraversalWeights[Index] = WeightValue;
        }

        FlowField.SetTraversalWeights(TraversalWeights);
        bHasBuiltField = false;
        bHasDebugSnapshot = false;
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

        if (bDrawGrid)
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

                if (bDrawDirections)
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

                if (bDrawIntegrationValues)
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
                }
                else if (bHighlightBlockedCells && !bIsWalkable)
                {
                        const FVector Extent(CellSizeLocal * 0.5f, CellSizeLocal * 0.5f, 10.0f);
                        DrawDebugSolidBox(World, CellCenter, Extent, BlockedCellColor, false, 0.0f, 0);
                }
        }

        if (bDrawDestination && bHasBuiltField)
        {
                const FVector DestinationLocation = CachedDestinationWorld + UpOffset;
                DrawDebugSphere(World, DestinationLocation, DestinationMarkerRadius, 16, DirectionColor, false, 0.0f, 0, DebugLineThickness);
        }
}

