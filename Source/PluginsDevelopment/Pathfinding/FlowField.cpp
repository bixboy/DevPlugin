#include "Pathfinding/FlowField.h"

#include "Algo/Heap.h"

namespace
{
        constexpr float InvalidCost = TNumericLimits<float>::Max();

        struct FOpenCell
        {
                int32 Index = INDEX_NONE;
                float Cost = InvalidCost;
        };

        struct FOpenCellPredicate
        {
                bool operator()(const FOpenCell& A, const FOpenCell& B) const
                {
                        return A.Cost > B.Cost;
                }
        };

        const TArray<FIntPoint>& GetNeighborOffsets(bool bAllowDiagonal)
        {
                static const TArray<FIntPoint> CardinalOffsets = {
                        FIntPoint(1, 0),
                        FIntPoint(-1, 0),
                        FIntPoint(0, 1),
                        FIntPoint(0, -1)
                };

                static const TArray<FIntPoint> DiagonalOffsets = {
                        FIntPoint(1, 1),
                        FIntPoint(1, -1),
                        FIntPoint(-1, 1),
                        FIntPoint(-1, -1)
                };

                static TArray<FIntPoint> CachedWithDiagonal;
                static bool bInitialised = false;

                if (!bInitialised)
                {
                        CachedWithDiagonal = CardinalOffsets;
                        CachedWithDiagonal.Append(DiagonalOffsets);
                        bInitialised = true;
                }

                return bAllowDiagonal ? CachedWithDiagonal : CardinalOffsets;
        }
}

FFlowField::FFlowField(const FFlowFieldSettings& InSettings)
{
        ApplySettings(InSettings);
}

void FFlowField::ApplySettings(const FFlowFieldSettings& InSettings)
{
        Settings = InSettings;
        const int32 ExpectedCells = Settings.GridSize.X * Settings.GridSize.Y;
        IntegrationField.Init(InvalidCost, ExpectedCells);
        DirectionField.Init(FVector2D::ZeroVector, ExpectedCells);
        TraversalWeights.Init(255, ExpectedCells);
        ResetFields();
        Destination = FIntPoint(-1, -1);
}

void FFlowField::SetTraversalWeights(const TArray<uint8>& InWeights)
{
        const int32 ExpectedCells = Settings.GridSize.X * Settings.GridSize.Y;
        if (ensureMsgf(InWeights.Num() == ExpectedCells, TEXT("Traversal weight array does not match grid dimensions")))
        {
                TraversalWeights = InWeights;
        }
}

bool FFlowField::IsWalkable(const FIntPoint& Cell) const
{
        const int32 Index = ToLinearIndex(Cell);
        if (Index == INDEX_NONE)
        {
                        return false;
        }

        return TraversalWeights.IsValidIndex(Index) && TraversalWeights[Index] > 0;
}

bool FFlowField::Build(const FIntPoint& DestinationCell)
{
        if (!IsCellValid(DestinationCell) || !IsWalkable(DestinationCell))
        {
                return false;
        }

        ResetFields();
        Destination = DestinationCell;

        const int32 DestinationIndex = ToLinearIndex(DestinationCell);
        IntegrationField[DestinationIndex] = 0.0f;

        TArray<FOpenCell> OpenSet;
        OpenSet.Reserve(IntegrationField.Num());
        Algo::HeapPush(OpenSet, FOpenCell{DestinationIndex, 0.0f}, FOpenCellPredicate());

        while (OpenSet.Num() > 0)
        {
                FOpenCell Current;
                Algo::HeapPop(OpenSet, Current, FOpenCellPredicate());

                if (!IntegrationField.IsValidIndex(Current.Index))
                {
                        continue;
                }

                const float RecordedCost = IntegrationField[Current.Index];
                if (Current.Cost > RecordedCost + KINDA_SMALL_NUMBER)
                {
                        continue;
                }

                const int32 GridX = Settings.GridSize.X;
                const FIntPoint CurrentCell(Current.Index % GridX, Current.Index / GridX);

                for (const FIntPoint& Offset : GetNeighborOffsets(Settings.bAllowDiagonal))
                {
                        const FIntPoint NeighborCell = CurrentCell + Offset;
                        const int32 NeighborIndex = ToLinearIndex(NeighborCell);
                        if (NeighborIndex == INDEX_NONE)
                        {
                                continue;
                        }

                        const uint8 Weight = TraversalWeights.IsValidIndex(NeighborIndex) ? TraversalWeights[NeighborIndex] : 0;
                        if (Weight == 0)
                        {
                                continue;
                        }

                        const bool bDiagonal = Offset.X != 0 && Offset.Y != 0;
                        const float StepCost = Settings.StepCost * (bDiagonal ? Settings.DiagonalCostMultiplier : 1.0f);
                        const float NormalisedWeight = static_cast<float>(Weight) / 255.0f;
                        const float TraversalCost = StepCost * Settings.CellTraversalWeightMultiplier / FMath::Max(NormalisedWeight, KINDA_SMALL_NUMBER);
                        const float NewCost = RecordedCost + (TraversalCost * Settings.HeuristicWeight);

                        if (NewCost + KINDA_SMALL_NUMBER < IntegrationField[NeighborIndex])
                        {
                                IntegrationField[NeighborIndex] = NewCost;
                                Algo::HeapPush(OpenSet, FOpenCell{NeighborIndex, NewCost}, FOpenCellPredicate());
                        }
                }
        }

        RebuildFlowDirections();
        return true;
}

FVector2D FFlowField::GetDirectionForCell(const FIntPoint& Cell) const
{
        const int32 Index = ToLinearIndex(Cell);
        if (!DirectionField.IsValidIndex(Index))
        {
                return FVector2D::ZeroVector;
        }

        return DirectionField[Index];
}

FIntPoint FFlowField::WorldToCell(const FVector& WorldPosition) const
{
        const FVector Local = WorldPosition - Settings.Origin;
        const int32 X = FMath::FloorToInt(Local.X / Settings.CellSize);
        const int32 Y = FMath::FloorToInt(Local.Y / Settings.CellSize);
        return FIntPoint(X, Y);
}

FVector FFlowField::CellToWorld(const FIntPoint& Cell) const
{
        return Settings.Origin + FVector((Cell.X + 0.5f) * Settings.CellSize, (Cell.Y + 0.5f) * Settings.CellSize, 0.0f);
}

FVector FFlowField::GetDirectionForWorldPosition(const FVector& WorldPosition) const
{
        return FVector(GetDirectionForCell(WorldToCell(WorldPosition)), 0.0f);
}

FFlowFieldDebugSnapshot FFlowField::CreateDebugSnapshot() const
{
        FFlowFieldDebugSnapshot Snapshot;
        Snapshot.GridSize = Settings.GridSize;
        Snapshot.CellSize = Settings.CellSize;
        Snapshot.Origin = Settings.Origin;
        Snapshot.IntegrationField = IntegrationField;
        Snapshot.DirectionField = DirectionField;
        Snapshot.WalkableField = TraversalWeights;
        Snapshot.Destination = Destination;
        return Snapshot;
}

int32 FFlowField::ToLinearIndex(const FIntPoint& Cell) const
{
        if (!IsCellValid(Cell))
        {
                return INDEX_NONE;
        }

        return Cell.Y * Settings.GridSize.X + Cell.X;
}

bool FFlowField::IsCellValid(const FIntPoint& Cell) const
{
        return Cell.X >= 0 && Cell.Y >= 0 && Cell.X < Settings.GridSize.X && Cell.Y < Settings.GridSize.Y;
}

void FFlowField::ResetFields()
{
        for (float& Value : IntegrationField)
        {
                Value = InvalidCost;
        }

        for (FVector2D& Value : DirectionField)
        {
                Value = FVector2D::ZeroVector;
        }
}

void FFlowField::RebuildFlowDirections()
{
        const int32 CellCount = Settings.GridSize.X * Settings.GridSize.Y;
        if (CellCount == 0)
        {
                return;
        }

        const TArray<FIntPoint>& Offsets = GetNeighborOffsets(Settings.bAllowDiagonal);

        for (int32 Index = 0; Index < CellCount; ++Index)
        {
                if (IntegrationField[Index] >= InvalidCost)
                {
                        DirectionField[Index] = FVector2D::ZeroVector;
                        continue;
                }

                const FIntPoint Cell(Index % Settings.GridSize.X, Index / Settings.GridSize.X);
                float BestCost = IntegrationField[Index];
                FVector2D BestDirection = FVector2D::ZeroVector;
                FVector2D SmoothedDirection = FVector2D::ZeroVector;
                float AccumulatedWeight = 0.0f;

                for (const FIntPoint& Offset : Offsets)
                {
                        const FIntPoint Neighbor = Cell + Offset;
                        const int32 NeighborIndex = ToLinearIndex(Neighbor);
                        if (NeighborIndex == INDEX_NONE)
                        {
                                continue;
                        }

                        const float NeighborCost = IntegrationField[NeighborIndex];
                        if (NeighborCost < BestCost)
                        {
                                BestCost = NeighborCost;
                                BestDirection = FVector2D(static_cast<float>(Offset.X), static_cast<float>(Offset.Y));
                        }

                        if (NeighborCost < InvalidCost)
                        {
                                const float CostDelta = IntegrationField[Index] - NeighborCost;
                                if (CostDelta > 0.0f)
                                {
                                        const FVector2D OffsetVector(static_cast<float>(Offset.X), static_cast<float>(Offset.Y));
                                        SmoothedDirection += OffsetVector * CostDelta;
                                        AccumulatedWeight += CostDelta;
                                }
                        }
                }

                if (Settings.bSmoothDirections && AccumulatedWeight > 0.0f)
                {
                        DirectionField[Index] = SmoothedDirection.GetSafeNormal();
                }
                else if (!BestDirection.IsNearlyZero())
                {
                        DirectionField[Index] = BestDirection.GetSafeNormal();
                }
                else
                {
                        DirectionField[Index] = FVector2D::ZeroVector;
                }
        }
}
