#include "Components/UnitSelectionComponent.h"

#include "Blueprint/UserWidget.h"
#include "Engine/AssetManager.h"
#include "Interfaces/Selectable.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Widget/JupiterHudWidget.h"

namespace
{
    static void ComputeSelectionDelta(const TArray<AActor*>& PreviousSelection, const TArray<AActor*>& NewSelection, TArray<AActor*>& OutAdded, TArray<AActor*>& OutRemoved)
    {
        TSet<TWeakObjectPtr<AActor>> PreviousSet;
        for (AActor* Actor : PreviousSelection)
        {
            PreviousSet.Add(Actor);
        }

        TSet<TWeakObjectPtr<AActor>> CurrentSet;
        for (AActor* Actor : NewSelection)
        {
            CurrentSet.Add(Actor);
        }

        for (const TWeakObjectPtr<AActor>& WeakActor : CurrentSet)
        {
            AActor* Actor = WeakActor.Get();
            if (!PreviousSet.Contains(WeakActor))
            {
                OutAdded.Add(Actor);
            }
        }

        for (const TWeakObjectPtr<AActor>& WeakActor : PreviousSet)
        {
            AActor* Actor = WeakActor.Get();
            if (!CurrentSet.Contains(WeakActor))
            {
                OutRemoved.Add(Actor);
            }
        }
    }
}

UUnitSelectionComponent::UUnitSelectionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UUnitSelectionComponent::BeginPlay()
{
    Super::BeginPlay();

    AssetManager = UAssetManager::GetIfInitialized();

    if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
    {
        OwnerController = Cast<APlayerController>(OwnerPawn->GetController());
    }

    if (!OwnerController)
    {
        OwnerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    }
}

void UUnitSelectionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(UUnitSelectionComponent, SelectedActors, COND_OwnerOnly);
}

void UUnitSelectionComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    Super::OnComponentDestroyed(bDestroyingHierarchy);

    DestroyHud();
}

FHitResult UUnitSelectionComponent::GetMousePositionOnTerrain() const
{
    if (!OwnerController)
    {
        return FHitResult();
    }

    FVector WorldLocation;
    FVector WorldDirection;
    OwnerController->DeprojectMousePositionToWorld(WorldLocation, WorldDirection);

    FHitResult OutHit;
    const FVector TraceEnd = WorldLocation + (WorldDirection * TerrainTraceLength);
    if (GetWorld() && GetWorld()->LineTraceSingleByChannel(OutHit, WorldLocation, TraceEnd, TerrainTraceChannel))
    {
        return OutHit;
    }

    return FHitResult();
}

void UUnitSelectionComponent::Handle_Selection(AActor* ActorToSelect)
{
    if (!GetOwner())
    {
        return;
    }

    if (HasAuthorityOnSelection())
    {
        Server_SelectSingle_Implementation(ActorToSelect, bToggleIfAlreadySelected);
    }
    else
    {
        Server_SelectSingle(ActorToSelect, bToggleIfAlreadySelected);
    }
}

void UUnitSelectionComponent::Handle_Selection(const TArray<AActor*>& ActorsToSelect)
{
    if (!GetOwner())
    {
        return;
    }

    if (HasAuthorityOnSelection())
    {
        Server_SelectGroup_Implementation(ActorsToSelect, false);
    }
    else
    {
        Server_SelectGroup(ActorsToSelect, false);
    }
}

void UUnitSelectionComponent::ClearSelection()
{
    if (HasAuthorityOnSelection())
    {
        Server_ClearSelection_Implementation();
    }
    else
    {
        Server_ClearSelection();
    }
}

bool UUnitSelectionComponent::HasGroupSelection() const
{
    return SelectedActors.Num() > 1;
}

bool UUnitSelectionComponent::ActorSelected(AActor* ActorToCheck) const
{
    return ActorToCheck && SelectedActors.Contains(ActorToCheck);
}

void UUnitSelectionComponent::RemoveInvalidSelections()
{
    if (!HasAuthorityOnSelection())
    {
        return;
    }

    TArray<AActor*> PreviousSelection = SelectedActors;
    SanitizeSelectionList(SelectedActors);
    HandleSelectionUpdate(PreviousSelection);
}

void UUnitSelectionComponent::CreateHud()
{
    if (HudInstance || !HudClass)
    {
        return;
    }

    if (APlayerController* PC = OwnerController.Get())
    {
        HudInstance = CreateWidget<UJupiterHudWidget>(PC, HudClass);
    }
    else if (UWorld* World = GetWorld())
    {
        HudInstance = CreateWidget<UJupiterHudWidget>(World, HudClass);
    }

    if (HudInstance)
    {
        HudInstance->AddToViewport();
    }
}

void UUnitSelectionComponent::DestroyHud()
{
    if (HudInstance)
    {
        HudInstance->RemoveFromParent();
        HudInstance = nullptr;
    }
}

void UUnitSelectionComponent::Server_SelectSingle_Implementation(AActor* ActorToSelect, bool bToggleIfSelected)
{
    TArray<AActor*> PreviousSelection = SelectedActors;

    if (!ActorToSelect)
    {
        if (bClearSelectionOnEmptyClick)
        {
            SelectedActors.Empty();
        }
    }
    else if (ActorToSelect->Implements<USelectable>())
    {
        const bool bAlreadySelected = SelectedActors.Contains(ActorToSelect);

        if (!bAllowAppendOnSingleSelection)
        {
            SelectedActors.Empty();
        }

        if (bAlreadySelected && bToggleIfSelected)
        {
            SelectedActors.Remove(ActorToSelect);
        }
        else if (!SelectedActors.Contains(ActorToSelect))
        {
            SelectedActors.Add(ActorToSelect);
        }
    }
    else if (bClearSelectionOnEmptyClick)
    {
        SelectedActors.Empty();
    }

    SanitizeSelectionList(SelectedActors);
    HandleSelectionUpdate(PreviousSelection);
}

void UUnitSelectionComponent::Server_SelectGroup_Implementation(const TArray<AActor*>& ActorsToSelect, bool bAppendToSelection)
{
    TArray<AActor*> PreviousSelection = SelectedActors;

    if (!bAppendToSelection)
    {
        SelectedActors.Empty();
    }

    for (AActor* Actor : ActorsToSelect)
    {
        if (Actor && Actor->Implements<USelectable>() && !SelectedActors.Contains(Actor))
        {
            SelectedActors.Add(Actor);
        }
    }

    SanitizeSelectionList(SelectedActors);
    HandleSelectionUpdate(PreviousSelection);
}

void UUnitSelectionComponent::Server_ClearSelection_Implementation()
{
    if (SelectedActors.IsEmpty())
    {
        return;
    }

    TArray<AActor*> PreviousSelection = SelectedActors;
    SelectedActors.Empty();
    HandleSelectionUpdate(PreviousSelection);
}

void UUnitSelectionComponent::OnRep_SelectedActors()
{
    TArray<AActor*> PreviousSelection = GetCachedSelection();
    HandleSelectionUpdate(PreviousSelection);
}

void UUnitSelectionComponent::HandleSelectionUpdate(const TArray<AActor*>& PreviousSelection)
{
    TArray<AActor*> AddedActors;
    TArray<AActor*> RemovedActors;
    ComputeSelectionDelta(PreviousSelection, SelectedActors, AddedActors, RemovedActors);

    ApplySelectionVisuals(AddedActors, RemovedActors);

    OnSelectionDelta.Broadcast(AddedActors, RemovedActors);
    OnSelectedUpdate.Broadcast();
    OnSelectionChanged.Broadcast(SelectedActors);

    RefreshCachedSelection();
}

bool UUnitSelectionComponent::HasAuthorityOnSelection() const
{
    return GetOwner() && GetOwner()->HasAuthority();
}

void UUnitSelectionComponent::SanitizeSelectionList(TArray<AActor*>& InOutSelection) const
{
    for (int32 Index = InOutSelection.Num() - 1; Index >= 0; --Index)
    {
        AActor* Actor = InOutSelection[Index];
        if (!IsValid(Actor) || !Actor->Implements<USelectable>())
        {
            InOutSelection.RemoveAt(Index);
        }
    }
}

void UUnitSelectionComponent::ApplySelectionVisuals(const TArray<AActor*>& AddedActors, const TArray<AActor*>& RemovedActors) const
{
    for (AActor* Actor : RemovedActors)
    {
        if (Actor && Actor->Implements<USelectable>())
        {
            ISelectable::Execute_Deselect(Actor);
        }
    }

    for (AActor* Actor : AddedActors)
    {
        if (Actor && Actor->Implements<USelectable>())
        {
            ISelectable::Execute_Select(Actor);
        }
    }
}

TArray<AActor*> UUnitSelectionComponent::GetCachedSelection() const
{
    TArray<AActor*> CachedSelection;
    CachedSelection.Reserve(CachedSelectedActors.Num());

    for (const TWeakObjectPtr<AActor>& WeakActor : CachedSelectedActors)
    {
        if (AActor* Actor = WeakActor.Get())
        {
            CachedSelection.Add(Actor);
        }
    }

    return CachedSelection;
}

void UUnitSelectionComponent::RefreshCachedSelection()
{
    CachedSelectedActors.Empty(SelectedActors.Num());
    for (AActor* Actor : SelectedActors)
    {
        CachedSelectedActors.Add(Actor);
    }
}

