// Copyright 2026

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Interaction/Deployment/Data/PlacementPropData.h"
#include "Interaction/Deployment/JupiterInstancedPropManager.h"
#include "Components/PrismRadialMenu.h"
#include "PlayerConstructionObject.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class IBuilderResourceInterface;
class UConstructionSystemSettings;

UCLASS(BlueprintType, Blueprintable)
class FIRSTPERSONBUILDER_API UPlayerConstructionObject : public UObject
{
	GENERATED_BODY()

public:
	UPlayerConstructionObject();

	// Initialize the object, must be called by the owning Actor
	UFUNCTION(BlueprintCallable, Category = "Construction")
	void InitializeConstruction(AActor* InOwner);

	// Register this object for network replication on the owning Actor (Use this in Blueprint!)
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Construction")
	void RegisterForReplication(AActor* OwnerActor);

	// Clean up components, call on destruction
	UFUNCTION(BlueprintCallable, Category = "Construction")
	void CleanupConstruction();

	// Ticks the construction logic (ghost updating, validity check), must be called by the owning Actor's Tick
	UFUNCTION(BlueprintCallable, Category = "Construction")
	void TickConstruction(float DeltaTime);

	// --- Network Replication Requirements ---
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;
	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, struct FOutParmRec* OutParms, FFrame* Stack) override;

	// --- Core Settings ---
	UPROPERTY(BlueprintReadWrite, Category = "Construction|Settings")
	TObjectPtr<UConstructionSystemSettings> Settings;

	// --- State ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Construction|Debug")
	bool bHasActiveSweetSpot;

	UPROPERTY(BlueprintReadOnly, Category = "Construction|State")
	bool bIsBuildModeActive;

	UPROPERTY(BlueprintReadOnly, Category = "Construction|State")
	TObjectPtr<UPlacementPropData> SelectedRecipe;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> GhostComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Construction|State")
	bool bIsValidPlacement = false;

	UPROPERTY(BlueprintReadOnly, Category = "Construction|State")
	bool bIsAimingAtSky = false;

	bool bLastValidState = true;
	TWeakObjectPtr<AActor> AimedActor;

	UPROPERTY(BlueprintReadOnly, Category = "Construction|State")
	int32 CurrentRotationStep;

	FVector GhostTargetLocation;
	FRotator GhostTargetRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction")
	bool bIsSnappingEnabled = true;

protected:
	bool bHasCachedSnappableClasses = false;
	TSet<TObjectPtr<UClass>> CachedSnappableClasses;

	// --- Internal State ---

public:
	// --- Actions ---

	UFUNCTION(BlueprintCallable, Category = "Construction")
	void EnterBuildMode(UPlacementPropData* InRecipe);

	UFUNCTION(BlueprintCallable, Category = "Construction")
	void ExitBuildMode();

	UFUNCTION(BlueprintCallable, Category = "Construction")
	void RotateGhost(int32 StepDirection);

	UFUNCTION(BlueprintCallable, Category = "Construction")
	void ToggleSnapping();

	UFUNCTION(BlueprintCallable, Category = "Construction")
	void RequestPlacement();

	UFUNCTION(BlueprintCallable, Category = "Construction")
	void RequestHammerHit();

	UFUNCTION(BlueprintCallable, Category = "Construction")
	void RequestDismantle();

	// --- Internal Logic ---
protected:
	UCameraComponent* GetPlayerCamera() const;

	void UpdateGhostTransform();
	void CheckPlacementValidity();
	
	UFUNCTION(Server, Reliable)
	void Server_RequestPlacement(UPlacementPropData* Recipe, FTransform SpawnTransform);

	UFUNCTION(Server, Reliable)
	void Server_RequestHammerHit(int32 TargetInstanceID, UPlacementPropData* TargetRecipe, bool bIsPerfectHit);

	UFUNCTION(Server, Reliable)
	void Server_RequestDismantle(int32 TargetInstanceID);

private:
	UPROPERTY()
	UDecalComponent* SweetSpotDecal;

	FVector CurrentSweetSpotWorldLocation;
	TWeakObjectPtr<class AJupiterInstancedPropManager> SweetSpotManager;
	int32 SweetSpotInstanceID = INDEX_NONE;
	
	void UpdateSweetSpot(UHierarchicalInstancedStaticMeshComponent* HISM, int32 InstanceIndex, class AJupiterInstancedPropManager* Manager, int32 InstanceID);

	// --- Helper Methods ---
	bool PerformPlacementTrace(FHitResult& OutHit) const;
	bool TrySnapGhost(const FHitResult& Hit, FVector& OutLoc, FRotator& OutRot);
	void CalculateFreePlacement(const FHitResult& Hit, float BottomOffset, FVector& OutLoc, FRotator& OutRot) const;
	
	bool CheckAndConsumeResources(UPlacementPropData* Recipe, bool bIsHit, bool bConsume);
	void PlayConstructionFeedback(UPlacementPropData* Recipe, bool bIsPerfectHit, const FVector& Location) const;

	class AJupiterFOBCore* FindNearbyFOB();

	/** Helpers for common lookups */
	UObject* GetResourceProvider();
	class AJupiterInstancedPropManager* GetPropManager();
	
	// --- Caching ---
	UPROPERTY(Transient)
	TWeakObjectPtr<class AJupiterFOBCore> CachedFOB;
	float FOBUpdateTimer = 0.0f;
	
	UPROPERTY(Transient)
	TWeakObjectPtr<class AJupiterInstancedPropManager> CachedPropManager;

public:
	// --- Catalog Menu ---
	UPROPERTY(Transient)
	TObjectPtr<class UBuilderCatalogMenu> ActiveUI;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|UI")
	TArray<UPlacementPropData*> AvailableRecipes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|UI")
	TObjectPtr<class UBuilderMenuTheme> MenuTheme;

	UFUNCTION(BlueprintCallable, Category = "Construction|UI")
	void OpenBuilderMenu();

	UFUNCTION(BlueprintCallable, Category = "Construction|UI")
	void CloseBuilderMenu();

	UFUNCTION()
	void HandleRadialMenuSelection(FName SegmentID);
};
