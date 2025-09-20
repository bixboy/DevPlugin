#include "Components/SlectionComponent.h"

#include "Blueprint/UserWidget.h"
#include "Data/FormationDataAsset.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Interfaces/Selectable.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Units/SoldierRts.h"
#include "Widget/JupiterHudWidget.h"
#include "Systems/RtsOrderSystem.h"
#include "Systems/RtsSelectionSystem.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"

USelectionComponent::USelectionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

void USelectionComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!OwnerController.IsValid())
    {
        OwnerController = Cast<APlayerController>(GetOwner());
        if (!OwnerController.IsValid())
        {
            OwnerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
        }
    }

    SelectionSubsystem = GetWorld()->GetSubsystem<URTSUnitSelectionSubsystem>();
    OrderSystem = GetWorld()->GetSubsystem<URTSOrderSystem>();

    if (bAutoRegisterToSubsystem && SelectionSubsystem.IsValid() && OwnerController.IsValid())
    {
        SelectionSubsystem->RegisterSelectionComponent(this, OwnerController.Get());
    }

    RefreshSelectionSet();
    BroadcastSelectionChanged(true);
}

void USelectionComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    Super::OnComponentDestroyed(bDestroyingHierarchy);

    DestroyHud();

    if (SelectionSubsystem.IsValid())
    {
        SelectionSubsystem->UnregisterSelectionComponent(this);
    }
}

void USelectionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(USelectionComponent, SelectedActors, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(USelectionComponent, CurrentFormation, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(USelectionComponent, FormationSpacing, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(USelectionComponent, UnitToSpawn, COND_OwnerOnly);
}

FHitResult USelectionComponent::GetMousePositionOnTerrain() const
{
    if (!OwnerController.IsValid())
    {
        GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("SelectionComponent missing owner controller"));
        return FHitResult();
    }

    FVector WorldLocation = FVector::ZeroVector;
    FVector WorldDirection = FVector::ForwardVector;
    OwnerController.Get()->DeprojectMousePositionToWorld(WorldLocation, WorldDirection);

    FHitResult OutHit;
    if (GetWorld()->LineTraceSingleByChannel(OutHit, WorldLocation, WorldLocation + (WorldDirection * 100000.f), ECollisionChannel::ECC_GameTraceChannel1) && OutHit.bBlockingHit)
    {
        return OutHit;
    }
    return FHitResult();
}

void USelectionComponent::CommandSelected(FCommandData CommandData)
{
    IssueCommandData(CommandData, bQueueCommandsByDefault);
}

// ------------------- Selection   ---------------------
#pragma region Selection

void USelectionComponent::Handle_Selection(AActor* ActorToSelect)
{
    if (!ActorToSelect)
    {
        return;
    }

    if (!ActorToSelect->Implements<USelectable>())
    {
        ClearSelection();
        return;
    }

    ToggleActorSelection(ActorToSelect);
}

void USelectionComponent::Handle_Selection(TArray<AActor*> ActorsToSelect)
{
    SelectActors(ActorsToSelect, false);
}

void USelectionComponent::SelectActor(AActor* Actor, bool bAppendSelection)
{
    if (!Actor)
    {
        return;
    }

    if (!GetOwner()->HasAuthority())
    {
        Server_Select(Actor, bAppendSelection);
        return;
    }

    if (!bAppendSelection)
    {
        ClearSelectionInternal(false);
    }

    if (AddActorToSelection(Actor, false, false))
    {
        BroadcastSelectionChanged(false);
    }
}

void USelectionComponent::SelectActors(const TArray<AActor*>& Actors, bool bAppendSelection)
{
    if (!GetOwner()->HasAuthority())
    {
        Server_Select_Group(Actors, bAppendSelection);
        return;
    }

    if (!bAppendSelection)
    {
        ClearSelectionInternal(false);
    }

    bool bSelectionChanged = false;
    for (AActor* Actor : Actors)
    {
        bSelectionChanged |= AddActorToSelection(Actor, false, false);
    }

    if (bSelectionChanged)
    {
        BroadcastSelectionChanged(false);
    }
}

void USelectionComponent::ToggleActorSelection(AActor* Actor)
{
    if (!Actor)
    {
        return;
    }

    if (!GetOwner()->HasAuthority())
    {
        if (ActorSelected(Actor))
        {
            Server_DeSelect(Actor);
        }
        else
        {
            Server_Select(Actor, true);
        }
        return;
    }

    if (ActorSelected(Actor))
    {
        RemoveActorFromSelection(Actor, true);
    }
    else
    {
        AddActorToSelection(Actor, true, false);
    }
}

void USelectionComponent::ClearSelection()
{
    if (!GetOwner()->HasAuthority())
    {
        Server_ClearSelected();
        return;
    }

    ClearSelectionInternal(true);
}

bool USelectionComponent::ActorSelected(AActor* ActorToCheck) const
{
    return SelectedActorSet.Contains(ActorToCheck);
}

void USelectionComponent::OnRep_Selected()
{
    RefreshSelectionSet();
    BroadcastSelectionChanged(true);
}

void USelectionComponent::RefreshSelectionSet()
{
    TSet<TWeakObjectPtr<AActor>> PreviousSelection = SelectedActorSet;
    TArray<AActor*> PreviousActors = SelectedActors;

    SelectedActorSet.Reset();
    for (AActor* Actor : SelectedActors)
    {
        SelectedActorSet.Add(Actor);
    }

    if (!GetOwner()->HasAuthority())
    {
        for (AActor* Actor : PreviousActors)
        {
            if (Actor && !SelectedActorSet.Contains(Actor) && Actor->Implements<USelectable>())
            {
                ISelectable::Execute_Deselect(Actor);
            }
        }

        for (AActor* Actor : SelectedActors)
        {
            if (Actor && !PreviousSelection.Contains(Actor) && Actor->Implements<USelectable>())
            {
                ISelectable::Execute_Select(Actor);
            }
        }
    }
}

void USelectionComponent::BroadcastSelectionChanged(bool bFromReplication)
{
    OnSelectedUpdate.Broadcast();
    OnSelectionChanged.Broadcast(SelectedActors, bFromReplication);

    if (SelectionSubsystem.IsValid() && OwnerController.IsValid())
    {
        SelectionSubsystem->UpdateSelection(this, SelectedActors);
    }
}

bool USelectionComponent::CanSelectActor(AActor* Actor) const
{
    if (!Actor || !Actor->Implements<USelectable>())
    {
        return false;
    }

    if (!bRestrictToSameTeam || SelectedActors.IsEmpty())
    {
        return true;
    }

    AActor* FirstActor = SelectedActors[0];
    if (!FirstActor || !FirstActor->Implements<USelectable>())
    {
        return true;
    }

    return ISelectable::Execute_GetCurrentTeam(FirstActor) == ISelectable::Execute_GetCurrentTeam(Actor);
}

bool USelectionComponent::AddActorToSelection(AActor* Actor, bool bBroadcast, bool bForce)
{
    if (!Actor)
    {
        return false;
    }

    if (!bForce && !CanSelectActor(Actor))
    {
        return false;
    }

    if (!bAllowMultipleSelection && !SelectedActors.IsEmpty())
    {
        ClearSelectionInternal(false);
    }

    if (SelectedActorSet.Contains(Actor))
    {
        return false;
    }

    SelectedActors.Add(Actor);
    SelectedActorSet.Add(Actor);

    if (Actor->Implements<USelectable>())
    {
        ISelectable::Execute_Select(Actor);
    }

    if (bBroadcast)
    {
        BroadcastSelectionChanged(false);
    }

    return true;
}

bool USelectionComponent::RemoveActorFromSelection(AActor* Actor, bool bBroadcast)
{
    if (!Actor)
    {
        return false;
    }

    if (!SelectedActorSet.Remove(Actor))
    {
        return false;
    }

    SelectedActors.Remove(Actor);

    if (Actor->Implements<USelectable>())
    {
        ISelectable::Execute_Deselect(Actor);
    }

    if (bBroadcast)
    {
        BroadcastSelectionChanged(false);
    }

    return true;
}

void USelectionComponent::ClearSelectionInternal(bool bBroadcast)
{
    for (AActor* Actor : SelectedActors)
    {
        if (Actor && Actor->Implements<USelectable>())
        {
            ISelectable::Execute_Deselect(Actor);
        }
    }

    SelectedActors.Empty();
    SelectedActorSet.Reset();

    if (bBroadcast)
    {
        BroadcastSelectionChanged(false);
    }
}

#pragma endregion

// ------------------- Orders ---------------------
#pragma region Orders

void USelectionComponent::IssueOrderToSelection(const FRTSOrderRequest& OrderRequest)
{
    FRTSOrderRequest RequestCopy = OrderRequest;
    RequestCopy.OrderTags.AppendTags(DefaultOrderTags);
    FCommandData CommandData = RequestCopy.ToCommandData(OwnerController.Get());
    IssueCommandData(CommandData, RequestCopy.bQueue);
}

void USelectionComponent::IssueSimpleOrder(ERTSOrderType OrderType, FVector TargetLocation, AActor* TargetActor, float Radius, bool bQueueCommand)
{
    FRTSOrderRequest Request;
    Request.OrderType = OrderType;
    Request.TargetLocation = TargetLocation;
    Request.TargetActor = TargetActor;
    Request.Radius = Radius;
    Request.bQueue = bQueueCommand;
    IssueOrderToSelection(Request);
}

void USelectionComponent::IssueCommandData(FCommandData CommandData, bool bQueueCommand)
{
    if (SelectedActors.IsEmpty())
    {
        return;
    }

    CommandData.RequestingController = OwnerController.Get();

    if (!GetOwner()->HasAuthority())
    {
        Server_CommandSelected(CommandData);
        return;
    }

    if (SelectionSubsystem.IsValid() && OwnerController.IsValid())
    {
        SelectionSubsystem->UpdateSelection(this, SelectedActors);
        SelectionSubsystem->OnSelectionCommandIssued.Broadcast(OwnerController.Get(), SelectionSubsystem->GetSelectionState(OwnerController.Get()), CommandData, bQueueCommand);
    }

    FRTSOrderRequest BaseRequest(CommandData);
    BaseRequest.bQueue = bQueueCommand;
    BaseRequest.OrderTags.AppendTags(DefaultOrderTags);

    if (OrderSystem)
    {

        for (int32 Index = 0; Index < SelectedActors.Num(); ++Index)
        {
            FCommandData LocalCommand = CommandData;
            if (!LocalCommand.Target)
            {
                CalculateOffset(Index, LocalCommand);
            }

            FRTSOrderRequest LocalRequest = BaseRequest;
            LocalRequest.TargetLocation = LocalCommand.Location;
            LocalRequest.LegacyCommand = LocalCommand;
            LocalRequest.TargetActor = LocalCommand.Target;

            TArray<AActor*> SingleUnit;
            SingleUnit.Add(SelectedActors[Index]);
            OrderSystem->IssueOrder(OwnerController.Get(), SingleUnit, LocalRequest);
        }

        OnOrderIssued.Broadcast(BaseRequest, SelectedActors);
        return;
    }

    DispatchCommandToSelection(CommandData);
    OnOrderIssued.Broadcast(BaseRequest, SelectedActors);
}

void USelectionComponent::DispatchCommandToSelection(const FCommandData& CommandData)
{
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    for (int32 Index = 0; Index < SelectedActors.Num(); ++Index)
    {
        AActor* Soldier = SelectedActors[Index];
        if (!Soldier || !Soldier->Implements<USelectable>())
        {
            continue;
        }

        if (CommandData.Target && ISelectable::Execute_GetCurrentTeam(Soldier) == ISelectable::Execute_GetCurrentTeam(CommandData.Target))
        {
            continue;
        }

        FCommandData LocalCommand = CommandData;
        if (!LocalCommand.Target)
        {
            CalculateOffset(Index, LocalCommand);
        }

        ISelectable::Execute_CommandMove(Soldier, LocalCommand);
    }
}

#pragma endregion

// ------------------- Server Replication   ---------------------
#pragma region Server Replication

void USelectionComponent::Server_CommandSelected_Implementation(FCommandData CommandData)
{
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    IssueCommandData(CommandData, bQueueCommandsByDefault);
}

void USelectionComponent::NetMulticast_Unreliable_PlayWalkAnimation_Implementation(const TArray<AActor*>& CurrentlySelectedActors)
{
    for (AActor* Soldier : CurrentlySelectedActors)
    {
        if (ASoldierRts* RtsSoldier = Cast<ASoldierRts>(Soldier))
        {
            RtsSoldier->StartWalkingEvent_Delegate.Broadcast();
        }
    }
}

void USelectionComponent::Server_Select_Group_Implementation(const TArray<AActor*>& ActorsToSelect, bool bAppendSelection)
{
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    if (!bAppendSelection)
    {
        ClearSelectionInternal(false);
    }

    bool bSelectionChanged = false;
    for (AActor* Actor : ActorsToSelect)
    {
        bSelectionChanged |= AddActorToSelection(Actor, false, false);
    }

    if (bSelectionChanged)
    {
        BroadcastSelectionChanged(false);
    }
}

void USelectionComponent::Server_Select_Implementation(AActor* ActorToSelect, bool bAppendSelection)
{
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    if (!bAppendSelection)
    {
        ClearSelectionInternal(false);
    }

    if (AddActorToSelection(ActorToSelect, false, false))
    {
        BroadcastSelectionChanged(false);
    }
}

void USelectionComponent::Server_DeSelect_Implementation(AActor* ActorToDeSelect)
{
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    if (RemoveActorFromSelection(ActorToDeSelect, false))
    {
        BroadcastSelectionChanged(false);
    }
}

void USelectionComponent::Server_ClearSelected_Implementation()
{
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    ClearSelectionInternal(true);
}

void USelectionComponent::OnRep_CurrentFormation()
{
    RefreshFormation(false);
}

void USelectionComponent::OnRep_FormationSpacing()
{
    RefreshFormation(true);
}

#pragma endregion

// ------------------- Formation ---------------------------
#pragma region Formation

bool USelectionComponent::HasGroupSelection() const
{
    return SelectedActors.Num() > 1;
}

void USelectionComponent::CreateHud()
{
    if (HudClass)
    {
        Hud = CreateWidget<UJupiterHudWidget>(GetWorld(), HudClass);
        if (Hud)
        {
            Hud->AddToViewport();
        }
    }
}

void USelectionComponent::DestroyHud()
{
    if (Hud)
    {
        Hud->RemoveFromParent();
        Hud = nullptr;
    }
}

UFormationDataAsset* USelectionComponent::GetFormationData() const
{
    for (UFormationDataAsset* Data : FormationData)
    {
        if (Data && Data->FormationType == CurrentFormation)
        {
            return Data;
        }
    }
    return nullptr;
}

void USelectionComponent::CalculateOffset(int Index, FCommandData& CommandData)
{
    if (FormationData.IsEmpty())
    {
        return;
    }

    CurrentFormationData = GetFormationData();
    if (!CurrentFormationData)
    {
        return;
    }

    FVector Offset = CurrentFormationData->Offset;
    const int32 NumActors = SelectedActors.Num();

    switch (CurrentFormationData->FormationType)
    {
    case EFormation::Square:
        {
            const int GridSize = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(NumActors)));
            Offset.X = (Index / GridSize) * FormationSpacing - ((GridSize - 1) * FormationSpacing * 0.5f);
            Offset.Y = (Index % GridSize) * FormationSpacing - ((GridSize - 1) * FormationSpacing * 0.5f);
            break;
        }
    case EFormation::Blob:
        {
            if (Index != 0)
            {
                const float Angle = (Index / static_cast<float>(NumActors)) * TWO_PI;
                const float Radius = FMath::RandRange(FormationSpacing * -0.5f, FormationSpacing * 0.5f);
                Offset.X += Radius * FMath::Cos(Angle);
                Offset.Y += Radius * FMath::Sin(Angle);
            }
            break;
        }
    default:
        {
            const float Multiplier = FMath::Floor((Index + 1) / 2) * FormationSpacing;
            Offset.Y = CurrentFormationData->Alternate && Index % 2 == 0 ? -Offset.Y : Offset.Y;
            Offset *= CurrentFormationData->Alternate ? Multiplier : Index * FormationSpacing;
            break;
        }
    }

    if (!CommandData.Rotation.IsZero())
    {
        Offset = CommandData.Rotation.RotateVector(Offset);
    }

    CommandData.Location = CommandData.SourceLocation + Offset;
}

void USelectionComponent::UpdateFormation(EFormation Formation)
{
    if (GetOwner()->HasAuthority())
    {
        CurrentFormation = Formation;
        OnRep_CurrentFormation();
    }
    else
    {
        Server_Reliable_UpdateFormation(Formation);
    }
}

void USelectionComponent::UpdateSpacing(float NewSpacing)
{
    Server_Unreliable_UpdateSpacing(NewSpacing);
}

void USelectionComponent::Server_Reliable_UpdateFormation_Implementation(EFormation Formation)
{
    CurrentFormation = Formation;
    OnRep_CurrentFormation();
}

void USelectionComponent::Server_Unreliable_UpdateSpacing_Implementation(float NewSpacing)
{
    FormationSpacing = NewSpacing;
    OnRep_FormationSpacing();
}

void USelectionComponent::RefreshFormation(bool bIsSpacing)
{
    if (HasGroupSelection() && SelectedActors.IsValidIndex(0) && OwnerController.IsValid())
    {
        FVector Centroid = FVector::ZeroVector;
        for (AActor* Actor : SelectedActors)
        {
            Centroid += Actor->GetActorLocation();
        }
        Centroid /= SelectedActors.Num();

        LastFormationLocation = Centroid;

        const FRotator PlayerRotation = OwnerController.Get()->GetPawn() ? OwnerController.Get()->GetPawn()->GetActorRotation() : FRotator::ZeroRotator;
        const FRotator CommandRotation(PlayerRotation.Pitch, PlayerRotation.Yaw, 0.f);

        IssueCommandData(FCommandData(OwnerController.Get(), LastFormationLocation, CommandRotation, ECommandType::CommandMove), false);
    }
}

#pragma endregion

// ------------------- Behavior ---------------------------
#pragma region Behavior

void USelectionComponent::UpdateBehavior(const ECombatBehavior NewBehavior)
{
    if (!GetOwner()->HasAuthority())
    {
        Server_UpdateBehavior(NewBehavior);
        return;
    }

    for (AActor* Soldier : SelectedActors)
    {
        if (Soldier && Soldier->Implements<USelectable>())
        {
            ISelectable::Execute_SetBehavior(Soldier, NewBehavior);
        }
    }
}

void USelectionComponent::Server_UpdateBehavior_Implementation(const ECombatBehavior NewBehavior)
{
    for (AActor* Soldier : SelectedActors)
    {
        if (Soldier && Soldier->Implements<USelectable>())
        {
            ISelectable::Execute_SetBehavior(Soldier, NewBehavior);
        }
    }
}

#pragma endregion

// ------------------- Spawn units ---------------------
#pragma region Spawn Units

void USelectionComponent::SpawnUnits()
{
    const FHitResult HitResult = GetMousePositionOnTerrain();

    if (!HitResult.bBlockingHit)
    {
        return;
    }

    Server_Reliable_SpawnUnits(HitResult.Location);
}

void USelectionComponent::Server_Reliable_ChangeUnitClass_Implementation(TSubclassOf<ASoldierRts> UnitClass)
{
    UnitToSpawn = UnitClass;
}

void USelectionComponent::OnRep_UnitClass()
{
    OnUnitUpdated.Broadcast(UnitToSpawn);
}

void USelectionComponent::Server_Reliable_SpawnUnits_Implementation(FVector HitLocation)
{
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    if (ASoldierRts* Unit = GetWorld()->SpawnActor<ASoldierRts>(UnitToSpawn, HitLocation, FRotator::ZeroRotator, SpawnParams))
    {
        if (SelectionSubsystem.IsValid() && OwnerController.IsValid())
        {
            SelectionSubsystem->UpdateSelection(this, SelectedActors);
        }
    }
}

#pragma endregion

// ------------------- Client Replication   ---------------------
#pragma region Client Replication

void USelectionComponent::Client_Select_Implementation(AActor* ActorToSelect)
{
    if (ActorToSelect && ActorToSelect->Implements<USelectable>())
    {
        ISelectable::Execute_Select(ActorToSelect);
    }
}

void USelectionComponent::Client_Deselect_Implementation(AActor* ActorToDeselect)
{
    if (ActorToDeselect && ActorToDeselect->Implements<USelectable>())
    {
        ISelectable::Execute_Deselect(ActorToDeselect);
    }
}

#pragma endregion

