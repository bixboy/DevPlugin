#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/AiData.h"
#include "RtsSelectionSystem.generated.h"

class USelectionComponent;
class URTSOrderSystem;

/**
 * Lightweight representation of a controller selection used by subsystems and Blueprints.
 * The struct intentionally stores weak references to keep garbage collection under control.
 */
USTRUCT(BlueprintType)
struct FRTSSelectionState
{
    GENERATED_BODY()

    /** Controller owning the selection (weak on purpose to avoid lifetime issues). */
    UPROPERTY(BlueprintReadOnly, Category = "RTS")
    TWeakObjectPtr<AController> OwningController;

    /** All currently selected actors. */
    UPROPERTY(BlueprintReadOnly, Category = "RTS")
    TArray<TWeakObjectPtr<AActor>> Units;

    /** Convenience flag telling if the selection contains more than one valid unit. */
    UPROPERTY(BlueprintReadOnly, Category = "RTS")
    bool bIsGroupSelection = false;

    /** Optional cached location used when dispatching formation orders. */
    UPROPERTY(BlueprintReadOnly, Category = "RTS")
    FVector CachedCentroid = FVector::ZeroVector;

    /** Optional cached rotation. */
    UPROPERTY(BlueprintReadOnly, Category = "RTS")
    FRotator CachedFacing = FRotator::ZeroRotator;

    FRTSSelectionState() = default;

    explicit FRTSSelectionState(const TWeakObjectPtr<AController>& InController)
        : OwningController(InController)
    {
    }

    /** Computes derived information such as centroid and group flag. */
    void RefreshDerivedData();
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRTSSelectionChangedSignature, AController*, OwningController, const FRTSSelectionState&, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FRTSSelectionCommandSignature, AController*, OwningController, const FRTSSelectionState&, Selection, const FCommandData&, CommandData, bool, bQueueCommand);

/**
 * Subsystem responsible for keeping the authoritative selection state for each player controller.
 * USelectionComponent instances forward their events to the subsystem which greatly simplifies the
 * logic when working in multiplayer games and in Blueprint scripts.
 */
UCLASS()
class JUPITERPLUGIN_API URTSUnitSelectionSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Registers a new component to the subsystem. */
    void RegisterSelectionComponent(USelectionComponent* Component, AController* OwningController);

    /** Removes a component from the subsystem map. */
    void UnregisterSelectionComponent(USelectionComponent* Component);

    /** Updates internal state when a component changes its local selection. */
    void UpdateSelection(USelectionComponent* Component, const TArray<AActor*>& SelectedActors);

    /** Issues a command for the current selection of the provided controller. */
    void IssueSelectionCommand(AController* Controller, const FCommandData& CommandData, bool bQueueCommand);

    /** Returns the last registered selection state for the controller. */
    FRTSSelectionState GetSelectionState(AController* Controller) const;

    /** Returns all registered controllers for debugging/UI purposes. */
    const TMap<TWeakObjectPtr<AController>, FRTSSelectionState>& GetSelectionStates() const { return SelectionStates; }

public:
    /** Broadcast every time a selection is updated on the owning machine. */
    UPROPERTY(BlueprintAssignable, Category = "RTS")
    FRTSSelectionChangedSignature OnSelectionChanged;

    /** Broadcast every time a command is requested on a selection. */
    UPROPERTY(BlueprintAssignable, Category = "RTS")
    FRTSSelectionCommandSignature OnSelectionCommandIssued;

private:
    /** Helper to rebuild internal state from the provided array. */
    static FRTSSelectionState BuildSelectionState(const TWeakObjectPtr<AController>& Controller, const TArray<AActor*>& Actors);

    /** Cache subsystem used to actually forward orders to the units. */
    TWeakObjectPtr<URTSOrderSystem> CachedOrderSystem;

    /** All components currently registered per controller. */
    TMap<TWeakObjectPtr<USelectionComponent>, TWeakObjectPtr<AController>> ComponentToController;

    /** Authoritative selection state for every controller. */
    TMap<TWeakObjectPtr<AController>, FRTSSelectionState> SelectionStates;
};

