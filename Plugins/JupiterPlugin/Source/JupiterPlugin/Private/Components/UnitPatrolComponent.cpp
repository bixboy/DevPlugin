#include "Components/UnitPatrolComponent.h"

#include "Components/SplineComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Math/UnrealMathUtility.h"

namespace
{
    /** Default colour assigned to locally edited patrol previews. */
    constexpr FColor PendingRouteColour = FColor::Yellow;
}

UUnitPatrolComponent::UUnitPatrolComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bAutoActivate = true;
    SetIsReplicatedByDefault(true);
}

void UUnitPatrolComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* OwnerActor = GetOwner())
    {
        CachedSelectionComponent = OwnerActor->FindComponentByClass<UUnitSelectionComponent>();
    }

    // CLIENT: ensure we start with a clean visualisation state
    UpdateSplineVisualisation();
    UpdatePendingSpline();
}

void UUnitPatrolComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearSplineComponents();
    DestroyPendingSpline();

    Super::EndPlay(EndPlayReason);
}

void UUnitPatrolComponent::AddPendingPatrolPoint(const FVector& WorldLocation)
{
    if (!IsLocallyControlled())
    {
        return;
    }

    PendingPatrolPoints.Add(WorldLocation);
    bIsCreatingPatrol = true;
    UpdatePendingSpline();
}

void UUnitPatrolComponent::ClearPendingPatrol()
{
    PendingPatrolPoints.Reset();
    DestroyPendingSpline();
    bIsCreatingPatrol = false;
    bPendingConfirmationClick = false;
    bConsumePendingLeftRelease = false;
}

void UUnitPatrolComponent::ConfirmPendingPatrol(bool bLoop)
{
    if (!IsLocallyControlled())
    {
        return;
    }

    if (PendingPatrolPoints.Num() < MinimumPointsForPreview)
    {
        CancelPatrolCreation();
        return;
    }

    ServerConfirmPatrol(PendingPatrolPoints, bLoop);
    PendingPatrolPoints.Reset();
    DestroyPendingSpline();
    bIsCreatingPatrol = false;
    bPendingConfirmationClick = false;
}

void UUnitPatrolComponent::SetShowPatrols(bool bEnable)
{
    bShowPatrols = bEnable;
    UpdatePendingSpline();
    UpdateSplineVisualisation();
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
            return true;
        }

        if (StartPatrolCreation())
        {
            const bool bPlacedPoint = TryAddPatrolPoint();
            if (!bPlacedPoint)
            {
                CancelPatrolCreation();
            }

            return true;
        }

        return false;
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

    const bool bShouldConfirm = bPendingConfirmationClick && bIsCreatingPatrol;
    bPendingConfirmationClick = false;

    if (bShouldConfirm)
    {
        ConfirmPatrolCreation();
        return true;
    }

    if (bIsCreatingPatrol)
    {
        return true;
    }

    return false;
}

bool UUnitPatrolComponent::HandleLeftClick(EInputEvent InputEvent)
{
    if (!IsLocallyControlled())
    {
        return false;
    }

    if (InputEvent == IE_Pressed)
    {
        if (bIsCreatingPatrol)
        {
            CancelPatrolCreation();
            bConsumePendingLeftRelease = true;
            return true;
        }

        bConsumePendingLeftRelease = false;
        return false;
    }

    if (InputEvent == IE_Released)
    {
        const bool bShouldConsume = bConsumePendingLeftRelease;
        bConsumePendingLeftRelease = false;
        return bShouldConsume;
    }

    return bIsCreatingPatrol;
}

void UUnitPatrolComponent::ServerConfirmPatrol_Implementation(const TArray<FVector>& RoutePoints, bool bLoop)
{
    HandleServerPatrolConfirmation(RoutePoints, bLoop);
}

void UUnitPatrolComponent::OnRep_ActiveRoutes()
{
    // CLIENT: Updates spline visualization when routes are replicated
    UpdateSplineVisualisation();
}

void UUnitPatrolComponent::UpdateSplineVisualisation()
{
    ClearSplineComponents();

    if (!bShowPatrols)
    {
        return;
    }

    const AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    UWorld* World = OwnerActor->GetWorld();
    if (!World || World->GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    for (const FPatrolRoute& Route : ActiveRoutes)
    {
        if (Route.PatrolPoints.Num() == 0)
        {
            continue;
        }

        USplineComponent* Spline = NewObject<USplineComponent>(OwnerActor);
        if (!Spline)
        {
            continue;
        }

        Spline->SetMobility(EComponentMobility::Movable);
        Spline->bIsEditorOnly = false;
        Spline->SetClosedLoop(Route.bIsLoop, true);
        Spline->SetDrawDebug(true);
        Spline->SetDrawDebugColor(FLinearColor(Route.RouteColor));
#if WITH_EDITORONLY_DATA
        Spline->SetSplineColor(FLinearColor(Route.RouteColor));
#endif

        if (USceneComponent* OwnerRoot = OwnerActor->GetRootComponent())
        {
            Spline->SetupAttachment(OwnerRoot);
        }

        Spline->RegisterComponent();
        Spline->ClearSplinePoints(false);

        for (const FVector& PatrolPoint : Route.PatrolPoints)
        {
            const FVector ElevatedPoint = PatrolPoint + FVector(0.f, 0.f, SplineHeightOffset);
            Spline->AddSplinePoint(ElevatedPoint, ESplineCoordinateSpace::World, false);
        }

        Spline->UpdateSpline();
        ActiveSplines.Add(Spline);
    }
}

void UUnitPatrolComponent::UpdatePendingSpline()
{
    if (!IsLocallyControlled())
    {
        DestroyPendingSpline();
        return;
    }

    if (!bShowPatrols || PendingPatrolPoints.Num() < MinimumPointsForPreview)
    {
        DestroyPendingSpline();
        return;
    }

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        DestroyPendingSpline();
        return;
    }

    UWorld* World = OwnerActor->GetWorld();
    if (!World || World->GetNetMode() == NM_DedicatedServer)
    {
        DestroyPendingSpline();
        return;
    }

    if (!PendingSpline)
    {
        PendingSpline = NewObject<USplineComponent>(OwnerActor);
        if (!PendingSpline)
        {
            return;
        }

        PendingSpline->SetMobility(EComponentMobility::Movable);
        PendingSpline->bIsEditorOnly = false;
        PendingSpline->SetDrawDebug(true);
        PendingSpline->SetDrawDebugColor(FLinearColor(PendingRouteColour));
#if WITH_EDITORONLY_DATA
        PendingSpline->SetSplineColor(FLinearColor(PendingRouteColour));
#endif

        if (USceneComponent* OwnerRoot = OwnerActor->GetRootComponent())
        {
            PendingSpline->SetupAttachment(OwnerRoot);
        }

        PendingSpline->RegisterComponent();
    }

    PendingSpline->SetClosedLoop(false, false);
    PendingSpline->ClearSplinePoints(false);

    for (const FVector& PatrolPoint : PendingPatrolPoints)
    {
        const FVector ElevatedPoint = PatrolPoint + FVector(0.f, 0.f, SplineHeightOffset);
        PendingSpline->AddSplinePoint(ElevatedPoint, ESplineCoordinateSpace::World, false);
    }

    PendingSpline->UpdateSpline();
}

void UUnitPatrolComponent::ClearSplineComponents()
{
    for (USplineComponent* Spline : ActiveSplines)
    {
        if (IsValid(Spline))
        {
            Spline->DestroyComponent();
        }
    }

    ActiveSplines.Reset();
}

void UUnitPatrolComponent::DestroyPendingSpline()
{
    if (PendingSpline && IsValid(PendingSpline))
    {
        PendingSpline->DestroyComponent();
    }

    PendingSpline = nullptr;
}

bool UUnitPatrolComponent::IsLocallyControlled() const
{
    if (const APawn* PawnOwner = Cast<APawn>(GetOwner()))
    {
        return PawnOwner->IsLocallyControlled();
    }

    if (const APlayerController* PlayerController = Cast<APlayerController>(GetOwner()))
    {
        return PlayerController->IsLocalController();
    }

    return false;
}

void UUnitPatrolComponent::HandleServerPatrolConfirmation(const TArray<FVector>& RoutePoints, bool bLoop)
{
    if (!GetOwner() || RoutePoints.Num() == 0)
    {
        return;
    }

    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    FPatrolRoute NewRoute;
    NewRoute.PatrolPoints = RoutePoints;
    NewRoute.bIsLoop = bLoop;
    NewRoute.RouteColor = bLoop ? FColor::Green : FColor::Cyan;

    ActiveRoutes.Add(NewRoute);

    // SERVER: keep listen servers in sync with their own visualisation
    UpdateSplineVisualisation();
}

bool UUnitPatrolComponent::StartPatrolCreation()
{
    if (!IsLocallyControlled())
    {
        return false;
    }

    if (!CachedSelectionComponent)
    {
        if (AActor* OwnerActor = GetOwner())
        {
            CachedSelectionComponent = OwnerActor->FindComponentByClass<UUnitSelectionComponent>();
        }
    }

    if (!CachedSelectionComponent)
    {
        return false;
    }

    PendingPatrolPoints.Reset();
    bIsCreatingPatrol = true;
    bPendingConfirmationClick = false;
    UpdatePendingSpline();
    return true;
}

bool UUnitPatrolComponent::TryAddPatrolPoint()
{
    FVector CursorLocation;
    if (!TryGetCursorLocation(CursorLocation))
    {
        return false;
    }

    AddPendingPatrolPoint(CursorLocation);
    return true;
}

void UUnitPatrolComponent::ConfirmPatrolCreation()
{
    if (!IsLocallyControlled())
    {
        return;
    }

    if (!bIsCreatingPatrol)
    {
        return;
    }

    if (PendingPatrolPoints.Num() < MinimumPointsForPreview)
    {
        CancelPatrolCreation();
        return;
    }

    bool bLoop = false;
    if (PendingPatrolPoints.Num() >= 3)
    {
        const FVector& FirstPoint = PendingPatrolPoints[0];
        const FVector& LastPoint = PendingPatrolPoints.Last();
        if (FVector::DistSquared(FirstPoint, LastPoint) <= FMath::Square(LoopClosureThreshold))
        {
            bLoop = true;
            PendingPatrolPoints.Last() = FirstPoint;
        }
    }

    ConfirmPendingPatrol(bLoop);
}

void UUnitPatrolComponent::CancelPatrolCreation()
{
    ClearPendingPatrol();
}

bool UUnitPatrolComponent::TryGetCursorLocation(FVector& OutLocation)
{
    if (!IsLocallyControlled())
    {
        return false;
    }

    if (!CachedSelectionComponent)
    {
        if (AActor* OwnerActor = GetOwner())
        {
            CachedSelectionComponent = OwnerActor->FindComponentByClass<UUnitSelectionComponent>();
        }
    }

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

void UUnitPatrolComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UUnitPatrolComponent, ActiveRoutes);
}

