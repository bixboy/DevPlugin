#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/AiData.h"
#include "Systems/RtsOrderSystem.h"
#include "Systems/RtsSelectionSystem.h"
#include "SlectionComponent.generated.h"

class ASoldierRts;
class UFormationDataAsset;
class UJupiterHudWidget;
class URTSOrderSystem;
class URTSUnitSelectionSubsystem;


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSelectedUpdatedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitUpdatedDelegate, TSubclassOf<ASoldierRts>, NewUnitClass);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRTSSelectionChangedSignature, const TArray<AActor*>&, NewSelection, bool, bFromReplication);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRTSOrderIssuedSignature, const FRTSOrderRequest&, Order, const TArray<AActor*>&, Units);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JUPITERPLUGIN_API USelectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USelectionComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const override;

	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	
	UFUNCTION()
	FHitResult GetMousePositionOnTerrain() const;

protected:
	UFUNCTION()
	virtual void BeginPlay() override;

	UFUNCTION(Server, Reliable)
	void Server_CommandSelected(FCommandData CommandData);

	UFUNCTION(NetMulticast, Unreliable)
	void NetMulticast_Unreliable_PlayWalkAnimation(const TArray<AActor*>& CurrentlySelectedActors);
	
        UPROPERTY()
        TWeakObjectPtr<APlayerController> OwnerController;

// ------------------- Selection ---------------------------
#pragma region Selection
	
public:

        UFUNCTION()
        void Handle_Selection(AActor* ActorToSelect);
        void Handle_Selection(const TArray<AActor*> ActorsToSelect);

        /** Selects a single actor. When bAppendSelection is false the existing selection is cleared first. */
        UFUNCTION(BlueprintCallable, Category = "RTS|Selection")
        void SelectActor(AActor* Actor, bool bAppendSelection = true);

        /** Selects a batch of actors. */
        UFUNCTION(BlueprintCallable, Category = "RTS|Selection")
        void SelectActors(const TArray<AActor*>& Actors, bool bAppendSelection = true);

        /** Toggles the selection state of an actor. */
        UFUNCTION(BlueprintCallable, Category = "RTS|Selection")
        void ToggleActorSelection(AActor* Actor);

        /** Clears the selection. */
        UFUNCTION(BlueprintCallable, Category = "RTS|Selection")
        void ClearSelection();

        UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RTS|Selection")
        const TArray<AActor*>& GetSelectedActors() const { return SelectedActors; }

        UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RTS|Selection")
        bool HasSelectedActors() const { return SelectedActors.Num() > 0; }

        /** Legacy helper kept for compatibility. */
        UFUNCTION()
        void CommandSelected(FCommandData CommandData);

        /** High level API to issue orders using the new request structure. */
        UFUNCTION(BlueprintCallable, Category = "RTS|Orders")
        void IssueOrderToSelection(const FRTSOrderRequest& OrderRequest);

        /** Convenience wrapper around IssueOrderToSelection. */
        UFUNCTION(BlueprintCallable, Category = "RTS|Orders")
        void IssueSimpleOrder(ERTSOrderType OrderType, FVector TargetLocation, AActor* TargetActor, float Radius = 0.f, bool bQueueCommand = false);

        /** Allows Blueprints to keep using FCommandData directly. */
        UFUNCTION(BlueprintCallable, Category = "RTS|Orders")
        void IssueCommandData(FCommandData CommandData, bool bQueueCommand);

        /** Fired whenever the selection changes. */
        UPROPERTY(BlueprintAssignable, Category = "RTS|Selection")
        FRTSSelectionChangedSignature OnSelectionChanged;

        /** Fired whenever the component successfully pushes an order to the order system. */
        UPROPERTY(BlueprintAssignable, Category = "RTS|Orders")
        FRTSOrderIssuedSignature OnOrderIssued;

        UPROPERTY()
        FSelectedUpdatedDelegate OnSelectedUpdate;

protected:

        UFUNCTION()
        bool ActorSelected(AActor* ActorToCheck) const;

        UFUNCTION()
        void OnRep_Selected();

        void RefreshSelectionSet();

        void BroadcastSelectionChanged(bool bFromReplication);

        bool CanSelectActor(AActor* Actor) const;

        bool AddActorToSelection(AActor* Actor, bool bBroadcast, bool bForce);

        bool RemoveActorFromSelection(AActor* Actor, bool bBroadcast);

        void ClearSelectionInternal(bool bBroadcast);

        void DispatchCommandToSelection(const FCommandData& CommandData);

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Selection")
        bool bAllowMultipleSelection = true;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Selection")
        bool bRestrictToSameTeam = true;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Selection")
        bool bAutoRegisterToSubsystem = true;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Orders")
        bool bQueueCommandsByDefault = false;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Orders")
        bool bForwardOrdersToSubsystem = true;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Orders")
        FGameplayTagContainer DefaultOrderTags;

        UPROPERTY(ReplicatedUsing = OnRep_Selected)
        TArray<AActor*> SelectedActors;

        /** Fast lookup structure derived from SelectedActors. */
        UPROPERTY(Transient)
        TSet<TWeakObjectPtr<AActor>> SelectedActorSet;

        /** Cached pointer to the selection subsystem. */
        UPROPERTY(Transient)
        TObjectPtr<URTSUnitSelectionSubsystem> SelectionSubsystem;

        /** Cached pointer to the order system. */
        UPROPERTY(Transient)
        TObjectPtr<URTSOrderSystem> OrderSystem;


	/*- Server Replication -*/

        UFUNCTION(Server, Reliable)
        void Server_Select(AActor* ActorToSelect, bool bAppendSelection);

        UFUNCTION(Server, Reliable)
        void Server_Select_Group(const TArray<AActor*>& ActorsToSelect, bool bAppendSelection);

        UFUNCTION(Server, Reliable)
        void Server_DeSelect(AActor* ActorToDeSelect);

        UFUNCTION(Server, Reliable)
	void Server_ClearSelected();

	/*- Client Replication -*/
	
	UFUNCTION(Client, Reliable)
	void Client_Select(AActor* ActorToSelect);
	UFUNCTION(Client, Reliable)
	void Client_Deselect(AActor* ActorToDeselect);

#pragma endregion	

// ------------------- Formation ---------------------------
#pragma region Formation
public:
	UFUNCTION()
	void UpdateSpacing(const float NewSpacing);
	
	UFUNCTION()
	bool HasGroupSelection() const;

	UFUNCTION()
	void CreateHud();

	UFUNCTION()
	void DestroyHud();
	

	
	UFUNCTION()
	void UpdateFormation(EFormation Formation);



protected:


	
	UFUNCTION()
	UFormationDataAsset* GetFormationData() const;
	
	UFUNCTION()
	void CalculateOffset(const int Index, FCommandData& CommandData);

	UFUNCTION()
	void RefreshFormation(bool bIsSpacing);


	/*- Variables -*/
public:
	UPROPERTY()
	TObjectPtr<UAssetManager> AssetManager;
	
protected:	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TArray<UFormationDataAsset*> FormationData;
	
	UPROPERTY(EditAnywhere, Category = "Settings|UI")
	TSubclassOf<UUserWidget> HudClass;

	UPROPERTY()
	TObjectPtr<UJupiterHudWidget> Hud;

	UPROPERTY(ReplicatedUsing = OnRep_FormationSpacing)
	float FormationSpacing;
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentFormation)
	TEnumAsByte<EFormation> CurrentFormation = EFormation::Square;

	UPROPERTY()
	const UFormationDataAsset* CurrentFormationData;

	UPROPERTY()
	FVector LastFormationLocation;
	UPROPERTY()
	AActor* LastFormationActor;

	
	
	/*- Server Replication -*/
	UFUNCTION(Server, Unreliable)
	void Server_Unreliable_UpdateSpacing(const float NewSpacing);
	UFUNCTION(Server, Reliable)
	void Server_Reliable_UpdateFormation(EFormation Formation);

	UFUNCTION()
	void OnRep_CurrentFormation();
	UFUNCTION()
	void OnRep_FormationSpacing();
	
#pragma endregion

// ------------------- Behavior ---------------------------
#pragma region Behavior
public:
	UFUNCTION()
	void UpdateBehavior(const ECombatBehavior NewBehavior);

protected:
	UFUNCTION(Server, Reliable)
	void Server_UpdateBehavior(const ECombatBehavior NewBehavior);
	
#pragma endregion

// ------------------- Spawn Units ---------------------------
#pragma region Spawn Units
	
public:
	UFUNCTION()
	void SpawnUnits();

	UFUNCTION(Server, Reliable)
	void Server_Reliable_ChangeUnitClass(TSubclassOf<ASoldierRts> UnitClass);
	
	UPROPERTY()
	FOnUnitUpdatedDelegate OnUnitUpdated;

protected:
	UFUNCTION(Server, Reliable)
	void Server_Reliable_SpawnUnits(FVector HitLocation);

	UFUNCTION()
	void OnRep_UnitClass();

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_UnitClass)
	TSubclassOf<ASoldierRts> UnitToSpawn;
	
#pragma endregion	

};
