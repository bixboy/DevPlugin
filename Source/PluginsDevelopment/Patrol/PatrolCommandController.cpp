#include "Patrol/PatrolCommandController.h"

#include "DrawDebugHelpers.h"
#include "Patrol/PatrolRouteComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "UObject/UObjectGlobals.h"

APatrolCommandController::APatrolCommandController()
{
        bShowMouseCursor = true;
        bEnableClickEvents = true;
        bEnableMouseOverEvents = true;
}

void APatrolCommandController::SetupInputComponent()
{
        Super::SetupInputComponent();

        if (!InputComponent)
        {
                return;
        }

        InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &APatrolCommandController::HandleRightClick);
        InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &APatrolCommandController::HandleLeftClick);
}

void APatrolCommandController::PlayerTick(float DeltaTime)
{
        Super::PlayerTick(DeltaTime);

        DrawPatrolPreview();
        DrawSelectedRoutes();
}

void APatrolCommandController::SetSelectedUnits(const TArray<AActor*>& Units)
{
        SelectedUnits.Reset();
        for (AActor* Actor : Units)
        {
                if (IsValid(Actor))
                {
                        SelectedUnits.Add(Actor);
                }
        }

        if (bIsCreatingPatrol && SelectedUnits.Num() == 0)
        {
                CancelPatrolCreation();
        }
}

void APatrolCommandController::ClearSelectedUnits()
{
        SelectedUnits.Reset();

        if (bIsCreatingPatrol)
        {
                CancelPatrolCreation();
        }
}

void APatrolCommandController::HandleRightClick()
{
        FVector HitLocation;
        if (!TryGetCursorWorldLocation(HitLocation))
        {
                return;
        }

        const bool bAltHeld = IsAltKeyDown();
        if (bAltHeld)
        {
                if (!bIsCreatingPatrol)
                {
                        StartPatrolCreation(HitLocation);
                }
                else
                {
                        ConfirmPatrolCreation();
                }
        }
        else if (bIsCreatingPatrol)
        {
                AddPatrolPoint(HitLocation);
        }
}

void APatrolCommandController::HandleLeftClick()
{
        if (bIsCreatingPatrol)
        {
                CancelPatrolCreation();
        }
}

bool APatrolCommandController::TryGetCursorWorldLocation(FVector& OutLocation) const
{
        FHitResult HitResult;
        const bool bHit = GetHitResultUnderCursor(ECC_Visibility, false, HitResult);
        if (bHit)
        {
                OutLocation = HitResult.Location;
                return true;
        }

        return false;
}

void APatrolCommandController::StartPatrolCreation(const FVector& InitialPoint)
{
        int32 ValidSelectionCount = 0;
        for (const TWeakObjectPtr<AActor>& WeakActor : SelectedUnits)
        {
                if (WeakActor.IsValid())
                {
                        ++ValidSelectionCount;
                }
        }

        if (ValidSelectionCount == 0)
        {
                return;
        }

        bIsCreatingPatrol = true;
        PendingPatrolPoints.Reset();
        PendingPatrolPoints.Add(InitialPoint);
}

void APatrolCommandController::ConfirmPatrolCreation()
{
        if (!bIsCreatingPatrol || PendingPatrolPoints.Num() < 2)
        {
                CancelPatrolCreation();
                return;
        }

        FPatrolRoute Route;
        Route.PatrolPoints = PendingPatrolPoints;

        const bool bCanCreateLoop = Route.PatrolPoints.Num() >= 3;
        const float StartToEndDistance = FVector::Dist(Route.PatrolPoints[0], Route.PatrolPoints.Last());
        if (bCanCreateLoop && StartToEndDistance <= LoopClosureThreshold)
        {
                Route.bIsLoop = true;
                Route.PatrolPoints.RemoveAt(Route.PatrolPoints.Num() - 1);
        }

        if (Route.PatrolPoints.Num() < 2)
        {
                CancelPatrolCreation();
                return;
        }

        for (const TWeakObjectPtr<AActor>& WeakActor : SelectedUnits)
        {
                if (!WeakActor.IsValid())
                {
                        continue;
                }

                Route.AssignedUnits.AddUnique(WeakActor.Get());
                UPatrolRouteComponent* Component = WeakActor->FindComponentByClass<UPatrolRouteComponent>();
                if (!Component)
                {
                        Component = NewObject<UPatrolRouteComponent>(WeakActor.Get());
                        if (Component)
                        {
                                Component->RegisterComponent();
                        }
                }

                if (Component)
                {
                        Component->AssignRoute(Route);
                }
        }

        CancelPatrolCreation();
}

void APatrolCommandController::CancelPatrolCreation()
{
        bIsCreatingPatrol = false;
        PendingPatrolPoints.Reset();
}

void APatrolCommandController::AddPatrolPoint(const FVector& NewPoint)
{
        if (!bIsCreatingPatrol)
        {
                return;
        }

        PendingPatrolPoints.Add(NewPoint);
}

bool APatrolCommandController::IsAltKeyDown() const
{
        return IsInputKeyDown(EKeys::LeftAlt) || IsInputKeyDown(EKeys::RightAlt);
}

void APatrolCommandController::DrawPatrolPreview() const
{
        if (!bIsCreatingPatrol || PendingPatrolPoints.Num() == 0)
        {
                return;
        }

        UWorld* World = GetWorld();
        if (!World)
        {
                return;
        }

        for (int32 Index = 0; Index < PendingPatrolPoints.Num(); ++Index)
        {
                const FVector Current = PendingPatrolPoints[Index];
                DrawDebugSphere(World, Current, 24.0f, 12, PreviewColor, false, 0.0f, 0, 1.0f);

                if (Index > 0)
                {
                        DrawDebugLine(World, PendingPatrolPoints[Index - 1], Current, PreviewColor, false, 0.0f, 0, 1.5f);
                }
        }

        if (PendingPatrolPoints.Num() >= 3)
        {
                const float StartToEndDistance = FVector::Dist(PendingPatrolPoints[0], PendingPatrolPoints.Last());
                if (StartToEndDistance <= LoopClosureThreshold)
                {
                        DrawDebugLine(World, PendingPatrolPoints.Last(), PendingPatrolPoints[0], PreviewColor, false, 0.0f, 0, 1.5f);
                }
        }
}

void APatrolCommandController::DrawSelectedRoutes() const
{
        UWorld* World = GetWorld();
        if (!World)
        {
                return;
        }

        for (const TWeakObjectPtr<AActor>& WeakActor : SelectedUnits)
        {
                if (!WeakActor.IsValid())
                {
                        continue;
                }

                if (const UPatrolRouteComponent* Component = WeakActor->FindComponentByClass<UPatrolRouteComponent>())
                {
                        Component->DrawRoute(World, SelectedRouteColor);
                }
        }
}
