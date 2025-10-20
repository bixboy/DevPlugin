#include "Patrol/PlayerCameraPatrolComponent.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Patrol/PatrolRouteComponent.h"
#include "Player/PlayerCamera.h"
#include "Components/InputComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "Engine/World.h"

UPlayerCameraPatrolComponent::UPlayerCameraPatrolComponent()
{
        PrimaryComponentTick.bCanEverTick = true;
}

void UPlayerCameraPatrolComponent::BeginPlay()
{
        Super::BeginPlay();

        CachedCamera = Cast<APlayerCamera>(GetOwner());
        if (CachedCamera.IsValid())
        {
                CachedController = CachedCamera->GetController<APlayerController>();

                if (UUnitSelectionComponent* Selection = CachedCamera->SelectionComponent)
                {
                        SelectionComponent = Selection;
                        SelectionComponent->OnSelectionChanged.AddDynamic(this, &UPlayerCameraPatrolComponent::HandleSelectionChanged);
                        HandleSelectionChanged(SelectionComponent->GetSelectedActors());
                }
        }
}

void UPlayerCameraPatrolComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
        if (SelectionComponent.IsValid())
        {
                SelectionComponent->OnSelectionChanged.RemoveDynamic(this, &UPlayerCameraPatrolComponent::HandleSelectionChanged);
        }

        Super::EndPlay(EndPlayReason);
}

void UPlayerCameraPatrolComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
        Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

        if (!CachedController.IsValid() && CachedCamera.IsValid())
        {
                CachedController = CachedCamera->GetController<APlayerController>();
        }

        DrawPatrolPreview();
        DrawSelectedRoutes();
}

void UPlayerCameraPatrolComponent::BindInput(UInputComponent* InputComponent)
{
        if (!InputComponent || bInputBound)
        {
                return;
        }

        InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &UPlayerCameraPatrolComponent::HandleRightMousePressed);
        InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &UPlayerCameraPatrolComponent::HandleLeftMousePressed);
        bInputBound = true;
}

void UPlayerCameraPatrolComponent::HandleRightMousePressed()
{
        FVector HitLocation;
        if (!TryGetCursorWorldLocation(HitLocation))
        {
                return;
        }

        if (IsAltPressed())
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

void UPlayerCameraPatrolComponent::HandleLeftMousePressed()
{
        if (bIsCreatingPatrol)
        {
                CancelPatrolCreation();
        }
}

void UPlayerCameraPatrolComponent::HandleSelectionChanged(const TArray<AActor*>& SelectedActors)
{
        SelectedUnits.Reset();
        for (AActor* Actor : SelectedActors)
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

bool UPlayerCameraPatrolComponent::TryGetCursorWorldLocation(FVector& OutLocation) const
{
        if (!CachedController.IsValid())
        {
                return false;
        }

        FHitResult HitResult;
        const bool bHit = CachedController->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);
        if (bHit)
        {
                OutLocation = HitResult.Location;
                return true;
        }

        return false;
}

void UPlayerCameraPatrolComponent::StartPatrolCreation(const FVector& InitialPoint)
{
        PruneInvalidSelection();
        if (SelectedUnits.Num() == 0)
        {
                return;
        }

        bIsCreatingPatrol = true;
        PendingPatrolPoints.Reset();
        PendingPatrolPoints.Add(InitialPoint);
}

void UPlayerCameraPatrolComponent::ConfirmPatrolCreation()
{
        if (!bIsCreatingPatrol || PendingPatrolPoints.Num() < 2)
        {
                CancelPatrolCreation();
                return;
        }

        FPatrolRoute Route;
        Route.PatrolPoints = PendingPatrolPoints;

        const bool bCanCreateLoop = Route.PatrolPoints.Num() >= 3;
        if (bCanCreateLoop)
        {
                const float Distance = FVector::Dist(Route.PatrolPoints[0], Route.PatrolPoints.Last());
                if (Distance <= LoopClosureThreshold)
                {
                        Route.bIsLoop = true;
                        Route.PatrolPoints.RemoveAt(Route.PatrolPoints.Num() - 1);
                }
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

void UPlayerCameraPatrolComponent::CancelPatrolCreation()
{
        bIsCreatingPatrol = false;
        PendingPatrolPoints.Reset();
}

void UPlayerCameraPatrolComponent::AddPatrolPoint(const FVector& Point)
{
        if (!bIsCreatingPatrol)
        {
                return;
        }

        PendingPatrolPoints.Add(Point);
}

bool UPlayerCameraPatrolComponent::IsAltPressed() const
{
        if (!CachedController.IsValid())
        {
                return false;
        }

        return CachedController->IsInputKeyDown(EKeys::LeftAlt) || CachedController->IsInputKeyDown(EKeys::RightAlt);
}

void UPlayerCameraPatrolComponent::DrawPatrolPreview() const
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
                const float Distance = FVector::Dist(PendingPatrolPoints[0], PendingPatrolPoints.Last());
                if (Distance <= LoopClosureThreshold)
                {
                        DrawDebugLine(World, PendingPatrolPoints.Last(), PendingPatrolPoints[0], PreviewColor, false, 0.0f, 0, 1.5f);
                }
        }
}

void UPlayerCameraPatrolComponent::DrawSelectedRoutes() const
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

void UPlayerCameraPatrolComponent::PruneInvalidSelection()
{
        for (int32 Index = SelectedUnits.Num() - 1; Index >= 0; --Index)
        {
                if (!SelectedUnits[Index].IsValid())
                {
                        SelectedUnits.RemoveAt(Index);
                }
        }
}
