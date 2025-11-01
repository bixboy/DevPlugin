#include "Components/UnitPatrolComponent.h"

#include "Components/LineBatchComponent.h"
#include "Engine/EngineTypes.h"
#include "Player/PlayerCamera.h"
#include "Components/UnitOrderComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "Interfaces/Selectable.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Math/UnrealMathUtility.h"
#include "Net/UnrealNetwork.h"

UUnitPatrolComponent::UUnitPatrolComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    bAutoActivate = true;
    SetIsReplicatedByDefault(true);
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
    if (LineBatchComponent)
    {
        LineBatchComponent->DestroyComponent();
        LineBatchComponent = nullptr;
    }

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

    if (bDrawCursorPreview && bHasCursorLocation)
    {
        if (!LastPreviewCursorLocation.Equals(CachedCursorLocation, 1.0f))
        {
            LastPreviewCursorLocation = CachedCursorLocation;
            MarkVisualsDirty();
        }
    }
    else if (!LastPreviewCursorLocation.IsNearlyZero())
    {
        LastPreviewCursorLocation = FVector::ZeroVector;
        MarkVisualsDirty();
    }

    if (HasAuthority())
    {
        CleanupActiveRoutes();
    }

    UpdateRouteVisuals(DeltaTime);
}

void UUnitPatrolComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UUnitPatrolComponent, ActivePatrolRoutes);
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

        if (StartPatrolCreation())
        {
            // The very first Alt+Right click should also place the initial patrol point.
            TryAddPatrolPoint();
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
        return true; // Swallow the release to avoid issuing move orders while editing.
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

bool UUnitPatrolComponent::HasAuthority() const
{
    const AActor* OwnerActor = GetOwner();
    return OwnerActor && OwnerActor->HasAuthority();
}

void UUnitPatrolComponent::EnsureLineBatchComponent()
{
    if (LineBatchComponent && LineBatchComponent->IsRegistered())
    {
        return;
    }

    if (AActor* OwnerActor = GetOwner())
    {
        LineBatchComponent = NewObject<ULineBatchComponent>(OwnerActor, TEXT("PatrolRouteLineBatcher"));
        if (LineBatchComponent)
        {
            if (USceneComponent* RootComponent = OwnerActor->GetRootComponent())
            {
                LineBatchComponent->SetupAttachment(RootComponent);
            }

            LineBatchComponent->SetAutoActivate(true);
            LineBatchComponent->RegisterComponent();
            LineBatchComponent->SetHiddenInGame(false);
            LineBatchComponent->SetRelativeLocation(FVector::ZeroVector);
        }
    }
}

void UUnitPatrolComponent::MarkVisualsDirty()
{
    bVisualsDirty = true;
}

void UUnitPatrolComponent::UpdateRouteVisuals(float DeltaSeconds)
{
    EnsureLineBatchComponent();

    if (!LineBatchComponent)
    {
        return;
    }

    VisualRefreshTimer -= DeltaSeconds;
    if (!bVisualsDirty && VisualRefreshTimer > 0.f)
    {
        return;
    }

    LineBatchComponent->Flush();
    DrawPendingRouteVisualization();
    DrawActiveRouteVisualizations();

    VisualRefreshTimer = VisualRefreshInterval;
    bVisualsDirty = false;
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

    MarkVisualsDirty();

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

    if (HasAuthority())
    {
        RegisterPatrolRoute(NewRoute);
    }
    else
    {
        ServerRegisterPatrolRoute(NewRoute);
    }

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
    MarkVisualsDirty();
    return true;
}

void UUnitPatrolComponent::CancelPatrolCreation()
{
    bIsCreatingPatrol = false;
    bPendingConfirmationClick = false;
    bConsumePendingLeftRelease = false;
    PendingPatrolPoints.Reset();
    PendingAssignedUnits.Reset();
    MarkVisualsDirty();
}

void UUnitPatrolComponent::RegisterPatrolRoute(const FPatrolRoute& NewRoute)
{
    if (!HasAuthority())
    {
        return;
    }

    FPatrolRoute SanitizedRoute = NewRoute;
    SanitizedRoute.AssignedUnits.RemoveAll([](AActor* Actor)
    {
        return !IsValid(Actor);
    });

    SanitizedRoute.PatrolPoints.RemoveAllSwap([](const FVector& Point)
    {
        return !FMath::IsFinite(Point.X) || !FMath::IsFinite(Point.Y) || !FMath::IsFinite(Point.Z);
    }, EAllowShrinking::No);

    if (SanitizedRoute.AssignedUnits.IsEmpty() || SanitizedRoute.PatrolPoints.Num() < 2)
    {
        return;
    }

    FPatrolRoute& StoredRoute = ActivePatrolRoutes.Add_GetRef(SanitizedRoute);
    IssuePatrolCommands(StoredRoute);
    CleanupActiveRoutes();
    MarkVisualsDirty();
}

void UUnitPatrolComponent::IssuePatrolCommands(FPatrolRoute& Route)
{
    if (!HasAuthority())
    {
        return;
    }

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
    if (!HasAuthority())
    {
        return;
    }

    if (UnitsToRemove.IsEmpty())
    {
        return;
    }

    bool bRoutesModified = false;
    for (FPatrolRoute& Route : ActivePatrolRoutes)
    {
        const int32 PreviousNum = Route.AssignedUnits.Num();
        Route.AssignedUnits.RemoveAll([&UnitsToRemove](AActor* Actor)
        {
            return !IsValid(Actor) || UnitsToRemove.Contains(Actor);
        });

        bRoutesModified |= PreviousNum != Route.AssignedUnits.Num();
    }

    if (bRoutesModified)
    {
        MarkVisualsDirty();
    }
}

void UUnitPatrolComponent::CleanupActiveRoutes()
{
    if (!HasAuthority())
    {
        return;
    }

    bool bRoutesModified = false;
    for (int32 RouteIndex = ActivePatrolRoutes.Num() - 1; RouteIndex >= 0; --RouteIndex)
    {
        FPatrolRoute& Route = ActivePatrolRoutes[RouteIndex];

        const int32 PreviousAssigned = Route.AssignedUnits.Num();
        Route.AssignedUnits.RemoveAll([](AActor* Actor)
        {
            return !IsValid(Actor);
        });

        if (Route.AssignedUnits.Num() != PreviousAssigned)
        {
            bRoutesModified = true;
        }

        const int32 PreviousPoints = Route.PatrolPoints.Num();
        Route.PatrolPoints.RemoveAllSwap([](const FVector& Point)
        {
            return !FMath::IsFinite(Point.X) || !FMath::IsFinite(Point.Y) || !FMath::IsFinite(Point.Z);
        }, EAllowShrinking::No);

        if (Route.PatrolPoints.Num() != PreviousPoints)
        {
            bRoutesModified = true;
        }

        if (Route.AssignedUnits.IsEmpty() || Route.PatrolPoints.Num() < 2)
        {
            ActivePatrolRoutes.RemoveAt(RouteIndex);
            bRoutesModified = true;
        }
    }

    if (bRoutesModified)
    {
        MarkVisualsDirty();
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

    MarkVisualsDirty();
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

void UUnitPatrolComponent::DrawPendingRouteVisualization()
{
    if (!LineBatchComponent || !bIsCreatingPatrol || PendingPatrolPoints.Num() == 0)
    {
        return;
    }

    DrawRouteVisualization(PendingPatrolPoints, false, PendingRouteColor);

    if (bDrawCursorPreview && PendingPatrolPoints.Num() > 0 && bHasCursorLocation)
    {
        LineBatchComponent->DrawLine(PendingPatrolPoints.Last(), CachedCursorLocation, PendingRouteColor, SDPG_World, DebugLineThickness, VisualRefreshInterval + KINDA_SMALL_NUMBER);
        LineBatchComponent->DrawPoint(CachedCursorLocation, PendingRouteColor, DebugPointRadius, SDPG_World, VisualRefreshInterval + KINDA_SMALL_NUMBER);
    }
}

void UUnitPatrolComponent::DrawActiveRouteVisualizations()
{
    if (!LineBatchComponent || ActivePatrolRoutes.Num() == 0)
    {
        return;
    }

    for (const FPatrolRoute& Route : ActivePatrolRoutes)
    {
        if (!SelectionContainsRouteUnits(Route))
        {
            continue;
        }

        DrawRouteVisualization(Route.PatrolPoints, Route.bIsLoop, ActiveRouteColor);
    }
}

void UUnitPatrolComponent::DrawRouteVisualization(const TArray<FVector>& Points, bool bLoop, const FColor& Color)
{
    if (!LineBatchComponent || Points.Num() == 0)
    {
        return;
    }

    DrawRoutePoints(Points, Color);

    if (Points.Num() == 1)
    {
        return;
    }

    TArray<FVector> Samples;
    BuildSplineSamples(Points, bLoop, Samples);

    if (Samples.Num() < 2)
    {
        return;
    }

    for (int32 Index = 0; Index < Samples.Num() - 1; ++Index)
    {
        LineBatchComponent->DrawLine(Samples[Index], Samples[Index + 1], Color, SDPG_World, DebugLineThickness, VisualRefreshInterval + KINDA_SMALL_NUMBER);
    }

    if (bLoop)
    {
        LineBatchComponent->DrawLine(Samples.Last(), Samples[0], Color, SDPG_World, DebugLineThickness, VisualRefreshInterval + KINDA_SMALL_NUMBER);
    }
}

void UUnitPatrolComponent::DrawRoutePoints(const TArray<FVector>& Points, const FColor& Color)
{
    if (!LineBatchComponent)
    {
        return;
    }

    for (const FVector& Point : Points)
    {
        LineBatchComponent->DrawPoint(Point, Color, DebugPointRadius, SDPG_World, VisualRefreshInterval + KINDA_SMALL_NUMBER);
    }
}

void UUnitPatrolComponent::BuildSplineSamples(const TArray<FVector>& Points, bool bLoop, TArray<FVector>& OutSamples) const
{
    OutSamples.Reset();

    const int32 NumPoints = Points.Num();
    if (NumPoints < 2)
    {
        OutSamples = Points;
        return;
    }

    const int32 NumSegments = bLoop ? NumPoints : NumPoints - 1;
    if (NumSegments <= 0)
    {
        OutSamples = Points;
        return;
    }

    const int32 StepsPerSegment = FMath::Max(1, SamplesPerSegment);

    auto WrapIndex = [NumPoints, bLoop](int32 Index)
    {
        if (bLoop)
        {
            return (Index % NumPoints + NumPoints) % NumPoints;
        }

        return FMath::Clamp(Index, 0, NumPoints - 1);
    };

    for (int32 SegmentIndex = 0; SegmentIndex < NumSegments; ++SegmentIndex)
    {
        const FVector& P0 = Points[WrapIndex(SegmentIndex - 1)];
        const FVector& P1 = Points[WrapIndex(SegmentIndex)];
        const FVector& P2 = Points[WrapIndex(SegmentIndex + 1)];
        const FVector& P3 = Points[WrapIndex(SegmentIndex + 2)];

        for (int32 Step = 0; Step < StepsPerSegment; ++Step)
        {
            const float T = static_cast<float>(Step) / static_cast<float>(StepsPerSegment);
            const FVector Sample = EvaluateCatmullRom(P0, P1, P2, P3, T);

            if (OutSamples.IsEmpty() || !Sample.Equals(OutSamples.Last(), 1.0f))
            {
                OutSamples.Add(Sample);
            }
        }
    }

    OutSamples.Add(Points[bLoop ? 0 : NumPoints - 1]);
}

FVector UUnitPatrolComponent::EvaluateCatmullRom(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T)
{
    const float T2 = T * T;
    const float T3 = T2 * T;

    return 0.5f * ((2.f * P1) +
        (-P0 + P2) * T +
        (2.f * P0 - 5.f * P1 + 4.f * P2 - P3) * T2 +
        (-P0 + 3.f * P1 - 3.f * P2 + P3) * T3);
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

    MarkVisualsDirty();
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

void UUnitPatrolComponent::ServerRegisterPatrolRoute_Implementation(const FPatrolRoute& NewRoute)
{
    RegisterPatrolRoute(NewRoute);
}

void UUnitPatrolComponent::OnRep_ActivePatrolRoutes()
{
    MarkVisualsDirty();
}

