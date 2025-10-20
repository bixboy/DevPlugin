#include "Components/UnitPatrolComponent.h"

#include "DrawDebugHelpers.h"
#include "Player/PlayerCamera.h"
#include "Components/UnitOrderComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "Interfaces/Selectable.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

namespace
{
    constexpr float DebugDrawDuration = 0.f; // Draw every frame without persistence.
}

UUnitPatrolComponent::UUnitPatrolComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    bAutoActivate = true;
}

void UUnitPatrolComponent::BeginPlay()
{
    Super::BeginPlay();

    CacheDependencies();

    if (CachedSelectionComponent)
    {
        CachedSelectionComponent->OnSelectionChanged.AddDynamic(this, &UUnitPatrolComponent::HandleSelectionChanged);
        RefreshSelectionCache(CachedSelectionComponent->GetSelectedActors());
    }

    if (CachedOrderComponent)
    {
        CachedOrderComponent->OnOrdersDispatched.AddDynamic(this, &UUnitPatrolComponent::HandleOrdersDispatched);
    }
}

void UUnitPatrolComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (CachedSelectionComponent)
    {
        CachedSelectionComponent->OnSelectionChanged.RemoveDynamic(this, &UUnitPatrolComponent::HandleSelectionChanged);
    }

    if (CachedOrderComponent)
    {
        CachedOrderComponent->OnOrdersDispatched.RemoveDynamic(this, &UUnitPatrolComponent::HandleOrdersDispatched);
    }

    Super::EndPlay(EndPlayReason);
}

void UUnitPatrolComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!IsLocallyControlled())
    {
        return;
    }

    if (CachedSelectionComponent)
    {
        FVector CursorLocation;
        if (TryGetCursorLocation(CursorLocation))
        {
            CachedCursorLocation = CursorLocation;
            bHasCursorLocation = true;
        }
        else
        {
            bHasCursorLocation = false;
        }
    }
    else
    {
        bHasCursorLocation = false;
    }

    CleanupActiveRoutes();

    DrawPendingRoute();
    DrawActiveRoutes();
}

bool UUnitPatrolComponent::HandleRightClickPressed(bool bAltDown)
{
    if (!IsLocallyControlled())
    {
        return false;
    }

    if (bAltDown)
    {
        if (bIsCreatingPatrol)
        {
            bPendingConfirmationClick = true;
            return true; // Consume the event so default handling does not interfere.
        }

        return StartPatrolCreation();
    }

    if (bIsCreatingPatrol)
    {
        return TryAddPatrolPoint();
    }

    return false;
}

bool UUnitPatrolComponent::HandleRightClickReleased(bool bAltDown)
{
    if (!IsLocallyControlled())
    {
        return false;
    }

    (void)bAltDown;

    const bool bShouldConfirm = bPendingConfirmationClick;
    bPendingConfirmationClick = false;

    if (bShouldConfirm)
    {
        return ConfirmPatrolCreation();
    }

    return false;
}

void UUnitPatrolComponent::HandleLeftClick(EInputEvent InputEvent)
{
    if (!IsLocallyControlled() || InputEvent != IE_Pressed)
    {
        return;
    }

    if (bIsCreatingPatrol)
    {
        CancelPatrolCreation();
    }
}

void UUnitPatrolComponent::CacheDependencies()
{
    CachedPlayerCamera = Cast<APlayerCamera>(GetOwner());

    if (CachedPlayerCamera)
    {
        if (CachedPlayerCamera->SelectionComponent)
        {
            CachedSelectionComponent = CachedPlayerCamera->SelectionComponent;
        }
        else
        {
            CachedSelectionComponent = CachedPlayerCamera->FindComponentByClass<UUnitSelectionComponent>();
        }

        if (CachedPlayerCamera->OrderComponent)
        {
            CachedOrderComponent = CachedPlayerCamera->OrderComponent;
        }
        else
        {
            CachedOrderComponent = CachedPlayerCamera->FindComponentByClass<UUnitOrderComponent>();
        }
    }
    else if (AActor* OwnerActor = GetOwner())
    {
        CachedSelectionComponent = OwnerActor->FindComponentByClass<UUnitSelectionComponent>();
        CachedOrderComponent = OwnerActor->FindComponentByClass<UUnitOrderComponent>();
    }
    else
    {
        CachedSelectionComponent = nullptr;
        CachedOrderComponent = nullptr;
    }
}

bool UUnitPatrolComponent::IsLocallyControlled() const
{
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    return OwnerPawn && OwnerPawn->IsLocallyControlled();
}

bool UUnitPatrolComponent::StartPatrolCreation()
{
    if (!CachedSelectionComponent)
    {
        return false;
    }

    TArray<AActor*> SelectedActors = CachedSelectionComponent->GetSelectedActors();
    SelectedActors.RemoveAll([](AActor* Actor) { return !IsValid(Actor); });

    if (SelectedActors.Num() == 0)
    {
        return false;
    }

    PendingPatrolPoints.Reset();
    PendingAssignedUnits.Reset();
    for (AActor* Actor : SelectedActors)
    {
        PendingAssignedUnits.Add(Actor);
    }

    bIsCreatingPatrol = true;
    bPendingConfirmationClick = false;

    return true;
}

bool UUnitPatrolComponent::ConfirmPatrolCreation()
{
    if (!bIsCreatingPatrol)
    {
        return false;
    }

    if (PendingPatrolPoints.Num() < 2)
    {
        CancelPatrolCreation();
        return true;
    }

    FPatrolRoute NewRoute;
    NewRoute.PatrolPoints = PendingPatrolPoints;

    const bool bShouldLoop = FVector::Dist(PendingPatrolPoints[0], PendingPatrolPoints.Last()) <= LoopClosureThreshold;
    NewRoute.bIsLoop = bShouldLoop;

    for (const TWeakObjectPtr<AActor>& WeakActor : PendingAssignedUnits)
    {
        if (WeakActor.IsValid())
        {
            NewRoute.AssignedUnits.Add(WeakActor.Get());
        }
    }

    RegisterPatrolRoute(NewRoute);

    CancelPatrolCreation();
    return true;
}

bool UUnitPatrolComponent::TryAddPatrolPoint()
{
    FVector CursorLocation;
    if (!TryGetCursorLocation(CursorLocation))
    {
        return true;
    }

    if (!PendingPatrolPoints.IsEmpty())
    {
        const float Distance = FVector::Dist(PendingPatrolPoints.Last(), CursorLocation);
        if (Distance <= KINDA_SMALL_NUMBER)
        {
            return true; // Ignore negligible movements but consume the click.
        }
    }

    PendingPatrolPoints.Add(CursorLocation);
    return true;
}

void UUnitPatrolComponent::CancelPatrolCreation()
{
    bIsCreatingPatrol = false;
    bPendingConfirmationClick = false;
    PendingPatrolPoints.Reset();
    PendingAssignedUnits.Reset();
}

void UUnitPatrolComponent::RegisterPatrolRoute(const FPatrolRoute& NewRoute)
{
    FPatrolRoute SanitizedRoute = NewRoute;
    SanitizedRoute.AssignedUnits.RemoveAll([](AActor* Actor)
    {
        return !IsValid(Actor);
    });

    SanitizedRoute.PatrolPoints.RemoveAllSwap([](const FVector& Point)
    {
        return !Point.IsFinite();
    }, false);

    if (SanitizedRoute.AssignedUnits.IsEmpty() || SanitizedRoute.PatrolPoints.Num() < 2)
    {
        return;
    }

    FPatrolRoute& StoredRoute = ActivePatrolRoutes.Add_GetRef(SanitizedRoute);
    IssuePatrolCommands(StoredRoute);
    CleanupActiveRoutes();
}

void UUnitPatrolComponent::IssuePatrolCommands(FPatrolRoute& Route)
{
    APlayerController* RequestingController = ResolveOwningController();

    for (int32 Index = Route.AssignedUnits.Num() - 1; Index >= 0; --Index)
    {
        AActor* Unit = Route.AssignedUnits[Index];
        if (!IsValid(Unit) || !Unit->Implements<USelectable>())
        {
            Route.AssignedUnits.RemoveAt(Index);
            continue;
        }

        FCommandData PatrolCommand;
        PatrolCommand.RequestingController = RequestingController;
        PatrolCommand.Type = ECommandType::CommandPatrol;
        PatrolCommand.SourceLocation = Route.PatrolPoints[0];
        PatrolCommand.Location = Route.PatrolPoints[0];
        PatrolCommand.Rotation = FRotator::ZeroRotator;
        PatrolCommand.Target = nullptr;
        PatrolCommand.Radius = 0.f;
        PatrolCommand.PatrolPath = Route.PatrolPoints;
        PatrolCommand.bPatrolLoop = Route.bIsLoop;

        ISelectable::Execute_CommandMove(Unit, PatrolCommand);
    }
}

void UUnitPatrolComponent::RemoveUnitsFromRoutes(const TArray<AActor*>& UnitsToRemove)
{
    if (UnitsToRemove.IsEmpty())
    {
        return;
    }

    for (FPatrolRoute& Route : ActivePatrolRoutes)
    {
        Route.AssignedUnits.RemoveAll([&UnitsToRemove](AActor* Actor)
        {
            return !IsValid(Actor) || UnitsToRemove.Contains(Actor);
        });
    }
}

void UUnitPatrolComponent::CleanupActiveRoutes()
{
    for (int32 RouteIndex = ActivePatrolRoutes.Num() - 1; RouteIndex >= 0; --RouteIndex)
    {
        FPatrolRoute& Route = ActivePatrolRoutes[RouteIndex];

        Route.AssignedUnits.RemoveAll([](AActor* Actor)
        {
            return !IsValid(Actor);
        });

        Route.PatrolPoints.RemoveAllSwap([](const FVector& Point)
        {
            return !Point.IsFinite();
        }, false);

        if (Route.AssignedUnits.IsEmpty() || Route.PatrolPoints.Num() < 2)
        {
            ActivePatrolRoutes.RemoveAt(RouteIndex);
        }
    }
}

void UUnitPatrolComponent::RefreshSelectionCache(const TArray<AActor*>& SelectedActors)
{
    CachedSelection.Reset();
    for (AActor* Actor : SelectedActors)
    {
        if (IsValid(Actor))
        {
            CachedSelection.Add(Actor);
        }
    }
}

bool UUnitPatrolComponent::SelectionContainsRouteUnits(const FPatrolRoute& Route) const
{
    if (CachedSelection.IsEmpty())
    {
        return false;
    }

    for (const TWeakObjectPtr<AActor>& WeakSelected : CachedSelection)
    {
        if (!WeakSelected.IsValid())
        {
            continue;
        }

        for (AActor* Unit : Route.AssignedUnits)
        {
            if (!IsValid(Unit))
            {
                continue;
            }

            if (Unit == WeakSelected.Get())
            {
                return true;
            }
        }
    }

    return false;
}

bool UUnitPatrolComponent::TryGetCursorLocation(FVector& OutLocation) const
{
    if (!CachedSelectionComponent)
    {
        return false;
    }

    const FHitResult HitResult = CachedSelectionComponent->GetMousePositionOnTerrain();
    if (!HitResult.bBlockingHit)
    {
        return false;
    }

    OutLocation = HitResult.Location;
    return true;
}

void UUnitPatrolComponent::DrawPendingRoute() const
{
    if (!bIsCreatingPatrol || PendingPatrolPoints.Num() == 0)
    {
        return;
    }

    DrawRoute(PendingPatrolPoints, false, PendingRouteColor);

    if (bDrawCursorPreview && PendingPatrolPoints.Num() > 0 && bHasCursorLocation)
    {
        if (UWorld* World = GetWorld())
        {
            const FVector LastPoint = PendingPatrolPoints.Last();
            DrawDebugLine(World, LastPoint, CachedCursorLocation, PendingRouteColor, false, DebugDrawDuration, 0, DebugLineThickness);
        }
    }
}

void UUnitPatrolComponent::DrawActiveRoutes() const
{
    if (ActivePatrolRoutes.Num() == 0)
    {
        return;
    }

    for (const FPatrolRoute& Route : ActivePatrolRoutes)
    {
        if (!SelectionContainsRouteUnits(Route))
        {
            continue;
        }

        DrawRoute(Route.PatrolPoints, Route.bIsLoop, ActiveRouteColor);
    }
}

void UUnitPatrolComponent::DrawRoute(const TArray<FVector>& Points, bool bLoop, const FColor& Color) const
{
    if (Points.Num() == 0)
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        for (int32 Index = 0; Index < Points.Num(); ++Index)
        {
            const FVector& Point = Points[Index];
            DrawDebugSphere(World, Point, DebugPointRadius, 16, Color, false, DebugDrawDuration);

            const int32 NextIndex = Index + 1;
            if (NextIndex < Points.Num())
            {
                DrawDebugLine(World, Point, Points[NextIndex], Color, false, DebugDrawDuration, 0, DebugLineThickness);
            }
        }

        if (bLoop && Points.Num() > 1)
        {
            DrawDebugLine(World, Points.Last(), Points[0], Color, false, DebugDrawDuration, 0, DebugLineThickness);
        }
    }
}

void UUnitPatrolComponent::HandleSelectionChanged(const TArray<AActor*>& SelectedActors)
{
    RefreshSelectionCache(SelectedActors);

    if (bIsCreatingPatrol)
    {
        PendingAssignedUnits.Reset();
        for (AActor* Actor : SelectedActors)
        {
            if (IsValid(Actor))
            {
                PendingAssignedUnits.Add(Actor);
            }
        }
    }
}

void UUnitPatrolComponent::HandleOrdersDispatched(const TArray<AActor*>& AffectedUnits, const FCommandData& CommandData)
{
    if (CommandData.Type == ECommandType::CommandPatrol)
    {
        return;
    }

    RemoveUnitsFromRoutes(AffectedUnits);
    CleanupActiveRoutes();
}

APlayerController* UUnitPatrolComponent::ResolveOwningController() const
{
    if (CachedPlayerCamera)
    {
        return Cast<APlayerController>(CachedPlayerCamera->GetController());
    }

    if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
    {
        return Cast<APlayerController>(OwnerPawn->GetController());
    }

    return nullptr;
}

