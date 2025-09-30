#include "FlowFieldTestActor.h"

AFlowFieldTestActor::AFlowFieldTestActor()
{
        PrimaryActorTick.bCanEverTick = true;
}

void AFlowFieldTestActor::OnConstruction(const FTransform& Transform)
{
        Super::OnConstruction(Transform);
        AnchorLocation = Transform.GetLocation();
        UpdateFlowFieldSettings();
}

void AFlowFieldTestActor::BeginPlay()
{
        Super::BeginPlay();
        AnchorLocation = GetActorLocation();
        UpdateFlowFieldSettings();
        bHasDestination = TryAssignNewDestination();
}

void AFlowFieldTestActor::Tick(float DeltaSeconds)
{
        Super::Tick(DeltaSeconds);

        if (!bHasDestination)
        {
                AnchorLocation = GetActorLocation();
                UpdateFlowFieldSettings();
                bHasDestination = TryAssignNewDestination();
                return;
        }

        const FVector FlowDirection = FlowField.GetDirectionForWorldPosition(GetActorLocation());
        if (!FlowDirection.IsNearlyZero())
        {
                const FVector NewLocation = GetActorLocation() + (FlowDirection * MovementSpeed * DeltaSeconds);
                SetActorLocation(NewLocation);
        }
        else
        {
                bHasDestination = false;
                return;
        }

        if (HasReachedDestination())
        {
                bHasDestination = false;
        }
}

void AFlowFieldTestActor::UpdateFlowFieldSettings()
{
        FlowFieldSettings.GridSize = GridSize;
        FlowFieldSettings.CellSize = CellSize;
        FlowFieldSettings.Origin = AnchorLocation - FVector(GridSize.X * CellSize * 0.5f, GridSize.Y * CellSize * 0.5f, 0.0f);
        FlowFieldSettings.bAllowDiagonal = bAllowDiagonal;
        FlowFieldSettings.DiagonalCostMultiplier = DiagonalCostMultiplier;
        FlowFieldSettings.StepCost = StepCost;
        FlowFieldSettings.CellTraversalWeightMultiplier = CellTraversalWeightMultiplier;
        FlowFieldSettings.HeuristicWeight = HeuristicWeight;
        FlowFieldSettings.bSmoothDirections = bSmoothDirections;

        FlowField.ApplySettings(FlowFieldSettings);

        const int32 ExpectedCells = GridSize.X * GridSize.Y;
        if (ExpectedCells <= 0)
        {
                TraversalWeights.Reset();
                return;
        }

        if (TraversalWeights.Num() != ExpectedCells)
        {
                TraversalWeights.Init(255, ExpectedCells);
        }

        FlowField.SetTraversalWeights(TraversalWeights);
}

bool AFlowFieldTestActor::TryAssignNewDestination()
{
        const FFlowFieldSettings& Settings = FlowField.GetSettings();
        if (Settings.GridSize.X <= 0 || Settings.GridSize.Y <= 0)
        {
                return false;
        }

        const int32 MaxAttempts = FMath::Max(1, MaxDestinationAttempts);
        const FIntPoint CurrentCell = FlowField.WorldToCell(GetActorLocation());
        const float CellSizeValue = FMath::Max(Settings.CellSize, KINDA_SMALL_NUMBER);
        const int32 RadiusInCells = FMath::Clamp(
                FMath::RoundToInt(DestinationSearchRadius / CellSizeValue),
                1,
                FMath::Max(Settings.GridSize.X, Settings.GridSize.Y));

        for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
        {
                const int32 OffsetX = FMath::RandRange(-RadiusInCells, RadiusInCells);
                const int32 OffsetY = FMath::RandRange(-RadiusInCells, RadiusInCells);
                FIntPoint Candidate = CurrentCell + FIntPoint(OffsetX, OffsetY);
                Candidate.X = FMath::Clamp(Candidate.X, 0, Settings.GridSize.X - 1);
                Candidate.Y = FMath::Clamp(Candidate.Y, 0, Settings.GridSize.Y - 1);

                if (Candidate == CurrentCell)
                {
                        continue;
                }

                if (!FlowField.IsWalkable(Candidate))
                {
                        continue;
                }

                if (!FlowField.Build(Candidate))
                {
                        continue;
                }

                CurrentDestination = FlowField.CellToWorld(Candidate);
                return true;
        }

        return false;
}

bool AFlowFieldTestActor::HasReachedDestination() const
{
        const FVector CurrentLocation = GetActorLocation();
        const float DistanceSquared = FVector::DistSquared2D(CurrentLocation, CurrentDestination);
        return DistanceSquared <= FMath::Square(AcceptanceRadius);
}
