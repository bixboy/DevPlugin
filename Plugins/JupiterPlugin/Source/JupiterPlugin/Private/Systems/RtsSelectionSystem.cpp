#include "Systems/RtsSelectionSystem.h"

#include "Components/SlectionComponent.h"
#include "Engine/World.h"
#include "Systems/RtsOrderSystem.h"

void FRTSSelectionState::RefreshDerivedData()
{
    bIsGroupSelection = false;
    CachedCentroid = FVector::ZeroVector;
    CachedFacing = FRotator::ZeroRotator;

    TArray<AActor*> ValidActors;
    ValidActors.Reserve(Units.Num());

    for (const TWeakObjectPtr<AActor>& WeakActor : Units)
    {
        if (AActor* Actor = WeakActor.Get())
        {
            ValidActors.Add(Actor);
            CachedCentroid += Actor->GetActorLocation();
        }
    }

    if (ValidActors.Num() > 0)
    {
        CachedCentroid /= ValidActors.Num();
        bIsGroupSelection = ValidActors.Num() > 1;

        FVector AccumulatedForward = FVector::ZeroVector;
        for (AActor* Actor : ValidActors)
        {
            AccumulatedForward += Actor->GetActorForwardVector();
        }

        if (!AccumulatedForward.IsNearlyZero())
        {
            CachedFacing = AccumulatedForward.GetSafeNormal().Rotation();
        }
        else if (ValidActors.Num() == 1)
        {
            CachedFacing = ValidActors[0]->GetActorRotation();
        }
    }
}

void URTSUnitSelectionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CachedOrderSystem = GetWorld()->GetSubsystem<URTSOrderSystem>();
}

void URTSUnitSelectionSubsystem::Deinitialize()
{
    ComponentToController.Empty();
    SelectionStates.Empty();
    CachedOrderSystem.Reset();
    Super::Deinitialize();
}

void URTSUnitSelectionSubsystem::RegisterSelectionComponent(USelectionComponent* Component, AController* OwningController)
{
    if (!Component || !OwningController)
    {
        return;
    }

    ComponentToController.Add(Component, OwningController);

    const TArray<AActor*> CurrentSelection = Component->GetSelectedActors();
    SelectionStates.Add(OwningController, BuildSelectionState(OwningController, CurrentSelection));
}

void URTSUnitSelectionSubsystem::UnregisterSelectionComponent(USelectionComponent* Component)
{
    if (!Component)
    {
        return;
    }

    const TWeakObjectPtr<AController>* ControllerPtr = ComponentToController.Find(Component);
    if (!ControllerPtr)
    {
        return;
    }

    SelectionStates.Remove(*ControllerPtr);
    ComponentToController.Remove(Component);
}

void URTSUnitSelectionSubsystem::UpdateSelection(USelectionComponent* Component, const TArray<AActor*>& SelectedActors)
{
    if (!Component)
    {
        return;
    }

    const TWeakObjectPtr<AController>* ControllerPtr = ComponentToController.Find(Component);
    if (!ControllerPtr)
    {
        return;
    }

    FRTSSelectionState NewState = BuildSelectionState(*ControllerPtr, SelectedActors);
    SelectionStates.Add(*ControllerPtr, NewState);

    if (AController* Controller = ControllerPtr->Get())
    {
        OnSelectionChanged.Broadcast(Controller, NewState);
    }
}

void URTSUnitSelectionSubsystem::IssueSelectionCommand(AController* Controller, const FCommandData& CommandData, bool bQueueCommand)
{
    if (!Controller)
    {
        return;
    }

    FRTSSelectionState* State = SelectionStates.Find(Controller);
    if (!State)
    {
        return;
    }

    State->RefreshDerivedData();

    OnSelectionCommandIssued.Broadcast(Controller, *State, CommandData, bQueueCommand);

    if (!CachedOrderSystem.IsValid())
    {
        CachedOrderSystem = GetWorld()->GetSubsystem<URTSOrderSystem>();
    }

    if (!CachedOrderSystem.IsValid())
    {
        return;
    }

    TArray<AActor*> Actors;
    Actors.Reserve(State->Units.Num());
    for (const TWeakObjectPtr<AActor>& WeakActor : State->Units)
    {
        if (AActor* Actor = WeakActor.Get())
        {
            Actors.Add(Actor);
        }
    }

    CachedOrderSystem->IssueOrder(Controller, Actors, CommandData, bQueueCommand);
}

FRTSSelectionState URTSUnitSelectionSubsystem::GetSelectionState(AController* Controller) const
{
    if (!Controller)
    {
        return FRTSSelectionState();
    }

    if (const FRTSSelectionState* FoundState = SelectionStates.Find(Controller))
    {
        return *FoundState;
    }

    return FRTSSelectionState(Controller);
}

FRTSSelectionState URTSUnitSelectionSubsystem::BuildSelectionState(const TWeakObjectPtr<AController>& Controller, const TArray<AActor*>& Actors)
{
    FRTSSelectionState NewState(Controller);
    NewState.Units.Reserve(Actors.Num());
    for (AActor* Actor : Actors)
    {
        NewState.Units.Add(Actor);
    }

    NewState.RefreshDerivedData();
    return NewState;
}

