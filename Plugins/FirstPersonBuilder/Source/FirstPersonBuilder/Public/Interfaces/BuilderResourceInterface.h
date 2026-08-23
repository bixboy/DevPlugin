// Copyright 2026

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BuilderResourceInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UBuilderResourceInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface to allow any Player or Inventory Component to supply resources to the Construction System.
 */
class FIRSTPERSONBUILDER_API IBuilderResourceInterface
{
	GENERATED_BODY()

public:
	/**
	 * Checks if the entity can afford the specified resource cost.
	 * @param ResourceItem The identifier for the resource type
	 * @param Amount The amount required
	 * @return True if affordable
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Builder|Resources")
	bool CanAffordResource(const TSoftObjectPtr<class UPDA_ItemClass>& ResourceItem, int32 Amount) const;
	virtual bool CanAffordResource_Implementation(const TSoftObjectPtr<class UPDA_ItemClass>& ResourceItem, int32 Amount) const { return true; }

	/**
	 * Consumes the specified resource cost.
	 * @param ResourceItem The identifier for the resource type
	 * @param Amount The amount to consume
	 * @return True if consumption was successful
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Builder|Resources")
	bool ConsumeResource(const TSoftObjectPtr<class UPDA_ItemClass>& ResourceItem, int32 Amount);
	virtual bool ConsumeResource_Implementation(const TSoftObjectPtr<class UPDA_ItemClass>& ResourceItem, int32 Amount) { return true; }

	/**
	 * Adds the specified resource amount.
	 * @param ResourceItem The identifier for the resource type
	 * @param Amount The amount to add
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Builder|Resources")
	void AddResource(const TSoftObjectPtr<class UPDA_ItemClass>& ResourceItem, int32 Amount);
	virtual void AddResource_Implementation(const TSoftObjectPtr<class UPDA_ItemClass>& ResourceItem, int32 Amount) {}
};
