#include "FlowFieldTestActor.h"

#include "FlowFieldManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AFlowFieldTestActor::AFlowFieldTestActor()
{
        PrimaryActorTick.bCanEverTick = true;
}

void AFlowFieldTestActor::OnConstruction(const FTransform& Transform)
{
        Super::OnConstruction(Transform);

        if (!FlowFieldManager && bAutoFindManager)
        {
                FlowFieldManager = ResolveFlowFieldManager();
        }
}

void AFlowFieldTestActor::BeginPlay()
{
        Super::BeginPlay();

        if (!FlowFieldManager && bAutoFindManager)
        {
                FlowFieldManager = ResolveFlowFieldManager();
        }

        if (!FlowFieldManager)
        {
                UE_LOG(LogTemp, Warning, TEXT("FlowFieldTestActor %s could not find a FlowFieldManager."), *GetName());
                return;
        }

        bHasDestination = TryAssignNewDestination();
}

void AFlowFieldTestActor::Tick(float DeltaSeconds)
{
        Super::Tick(DeltaSeconds);

        if (!FlowFieldManager)
        {
                if (bAutoFindManager)
                {
                        FlowFieldManager = ResolveFlowFieldManager();
                }

                return;
        }

        if (!bHasDestination)
        {
                bHasDestination = TryAssignNewDestination();
                return;
        }

        const FVector FlowDirection = FlowFieldManager->GetDirectionForWorldPosition(GetActorLocation());
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

bool AFlowFieldTestActor::TryAssignNewDestination()
{
        if (!FlowFieldManager)
        {
                return false;
        }

        const FFlowFieldSettings& Settings = FlowFieldManager->GetSettings();
        if (Settings.GridSize.X <= 0 || Settings.GridSize.Y <= 0)
        {
                return false;
        }

        const int32 MaxAttempts = FMath::Max(1, MaxDestinationAttempts);
        const FIntPoint CurrentCell = FlowFieldManager->WorldToCell(GetActorLocation());
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

                if (!FlowFieldManager->IsWalkable(Candidate))
                {
                        continue;
                }

                if (!FlowFieldManager->BuildFlowFieldToCell(Candidate))
                {
                        continue;
                }

                CurrentDestination = FlowFieldManager->CellToWorld(Candidate);
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

AFlowFieldManager* AFlowFieldTestActor::ResolveFlowFieldManager()
{
        if (!GetWorld())
        {
                return nullptr;
        }

        for (TActorIterator<AFlowFieldManager> It(GetWorld()); It; ++It)
        {
                if (AFlowFieldManager* Manager = *It)
                {
                        return Manager;
                }
        }

        return nullptr;
}
