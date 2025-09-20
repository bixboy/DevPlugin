#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/AiData.h"
#include "UnitSelectionComponent.generated.h"

class UFormationDataAsset;
class UJupiterHudWidget;
class UUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectionChangedSignature, const TArray<AActor*>&, SelectedActors);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSelectionDeltaSignature, const TArray<AActor*>&, AddedActors, const TArray<AActor*>&, RemovedActors);

UCLASS(ClassGroup = (RTS), meta = (BlueprintSpawnableComponent))
class JUPITERPLUGIN_API UUnitSelectionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UUnitSelectionComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

    /** Returns the current mouse hit on the navigation terrain. */
    UFUNCTION(BlueprintCallable, Category = "RTS|Selection")
    FHitResult GetMousePositionOnTerrain() const;

    /** Handles single unit selection from user input. */
    UFUNCTION(BlueprintCallable, Category = "RTS|Selection")
    void Handle_Selection(AActor* ActorToSelect);

    /** Handles group selection (typically from marquee selection). */
    UFUNCTION(BlueprintCallable, Category = "RTS|Selection")
    void Handle_Selection(const TArray<AActor*>& ActorsToSelect);

    /** Clears the current selection set. */
    UFUNCTION(BlueprintCallable, Category = "RTS|Selection")
    void ClearSelection();

    /** Returns true if more than one unit is currently selected. */
    UFUNCTION(BlueprintPure, Category = "RTS|Selection")
    bool HasGroupSelection() const;

    /** Returns the replicated list of selected actors. */
    UFUNCTION(BlueprintPure, Category = "RTS|Selection")
    TArray<AActor*> GetSelectedActors() const { return SelectedActors; }

    /** Returns true when the provided actor is currently selected. */
    UFUNCTION(BlueprintPure, Category = "RTS|Selection")
    bool ActorSelected(AActor* ActorToCheck) const;

    /** Ensures that invalid actors are pruned from the current selection. */
    UFUNCTION(BlueprintCallable, Category = "RTS|Selection")
    void RemoveInvalidSelections();

    /** Creates the HUD widget associated with the selection component. */
    UFUNCTION(BlueprintCallable, Category = "RTS|Selection")
    void CreateHud();

    /** Destroys the HUD widget created via CreateHud. */
    UFUNCTION(BlueprintCallable, Category = "RTS|Selection")
    void DestroyHud();

    /** Delegate fired whenever the replicated selection array changes. */
    UPROPERTY(BlueprintAssignable, Category = "RTS|Selection")
    FSelectedUpdatedDelegate OnSelectedUpdate;

    /** Delegate fired with the full selection payload for Blueprints. */
    UPROPERTY(BlueprintAssignable, Category = "RTS|Selection")
    FOnSelectionChangedSignature OnSelectionChanged;

    /** Delegate providing the delta between two selection states. */
    UPROPERTY(BlueprintAssignable, Category = "RTS|Selection")
    FOnSelectionDeltaSignature OnSelectionDelta;

    /** Asset manager reference exposed for UI driven asset loading. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Selection")
    TObjectPtr<UAssetManager> AssetManager;

protected:
    /** Server authoritative implementation of single selection behaviour. */
    UFUNCTION(Server, Reliable)
    void Server_SelectSingle(AActor* ActorToSelect, bool bToggleIfSelected);

    /** Server authoritative implementation of group selection behaviour. */
    UFUNCTION(Server, Reliable)
    void Server_SelectGroup(const TArray<AActor*>& ActorsToSelect, bool bAppendToSelection);

    /** Server RPC that clears the current selection list. */
    UFUNCTION(Server, Reliable)
    void Server_ClearSelection();

    /** Replication notification for the selected actors list. */
    UFUNCTION()
    void OnRep_SelectedActors();

    /** Applies selection delta and triggers visual feedback. */
    void HandleSelectionUpdate(const TArray<AActor*>& PreviousSelection);

    /** Helper returning whether the local controller is authoritative. */
    bool HasAuthorityOnSelection() const;

    /** Removes invalid entries from a selection array. */
    void SanitizeSelectionList(TArray<AActor*>& InOutSelection) const;

    /** Applies the visual feedback for selection deltas. */
    void ApplySelectionVisuals(const TArray<AActor*>& AddedActors, const TArray<AActor*>& RemovedActors) const;

    /** Converts the cached weak pointer selection to a plain array. */
    TArray<AActor*> GetCachedSelection() const;

    /** Updates the cached selection used on clients to compute deltas. */
    void RefreshCachedSelection();

protected:
    /** Replicated list of selected actors (owner only). */
    UPROPERTY(ReplicatedUsing = OnRep_SelectedActors)
    TArray<AActor*> SelectedActors;

    /** Cached weak references used to compute selection deltas locally. */
    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> CachedSelectedActors;

    /** Owning controller (retrieved on begin play). */
    UPROPERTY()
    TObjectPtr<APlayerController> OwnerController;

    /** HUD widget class that exposes selection controls. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Selection|UI")
    TSubclassOf<UUserWidget> HudClass;

    /** Spawned HUD widget instance. */
    UPROPERTY()
    TObjectPtr<UJupiterHudWidget> HudInstance;

    /** Whether a ground click should clear the current selection. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Selection")
    bool bClearSelectionOnEmptyClick = true;

    /** Whether selecting a new actor appends to the current selection. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Selection")
    bool bAllowAppendOnSingleSelection = false;

    /** Whether the selection should toggle when clicking a selected actor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Selection")
    bool bToggleIfAlreadySelected = true;

    /** Trace channel used to project the mouse cursor on the terrain. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Selection")
    TEnumAsByte<ECollisionChannel> TerrainTraceChannel = ECollisionChannel::ECC_GameTraceChannel1;

    /** Maximum trace distance when projecting the mouse cursor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Selection")
    float TerrainTraceLength = 100000.f;
};

