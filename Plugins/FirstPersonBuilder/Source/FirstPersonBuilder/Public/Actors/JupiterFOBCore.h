// Copyright 2026

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/BuilderResourceInterface.h"
#include "JupiterFOBCore.generated.h"

class USphereComponent;

/**
 * Forward Operating Base (FOB) that provides shared resources for building within its radius.
 */
UCLASS()
class FIRSTPERSONBUILDER_API AJupiterFOBCore : public AActor, public IBuilderResourceInterface
{
	GENERATED_BODY()
	
public:	
	AJupiterFOBCore();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FOB")
	USphereComponent* BuildRadiusComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FOB")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FOB|Inventory")
	class UAC_StorageComponent* StorageComponent;

public:	
	// --- IBuilderResourceInterface Implementation ---
	virtual bool CanAffordResource_Implementation(const TSoftObjectPtr<class UPDA_ItemClass>& ResourceItem, int32 Amount) const override;
	virtual bool ConsumeResource_Implementation(const TSoftObjectPtr<class UPDA_ItemClass>& ResourceItem, int32 Amount) override;
	virtual void AddResource_Implementation(const TSoftObjectPtr<class UPDA_ItemClass>& ResourceItem, int32 Amount) override;

	// --- Utilities ---
	UFUNCTION(BlueprintPure, Category = "FOB")
	float GetBuildRadius() const;

	UFUNCTION(BlueprintCallable, Category = "FOB")
	void SetFOBMesh(UStaticMesh* InMesh);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_BuildingMesh)
	UStaticMesh* ReplicatedMesh;

	UFUNCTION()
	void OnRep_BuildingMesh();
};
