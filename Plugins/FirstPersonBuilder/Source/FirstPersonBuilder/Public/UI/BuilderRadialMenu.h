// Copyright 2026

#pragma once

#include "CoreMinimal.h"
#include "PrismWidgetBase.h"
#include "Components/PrismRadialMenu.h"
#include "Interaction/Deployment/Data/PlacementPropData.h"
#include "BuilderRadialMenu.generated.h"

class UPlayerConstructionObject;

UCLASS(BlueprintType, Blueprintable)
class FIRSTPERSONBUILDER_API UBuilderRadialMenu : public UPrismRadialMenu
{
	GENERATED_BODY()

public:
	// The builder object this menu is currently controlling
	UPROPERTY(BlueprintReadOnly, Category = "Construction|UI")
	TWeakObjectPtr<UPlayerConstructionObject> OwningBuilder;

	// Recipes to display
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|UI")
	TArray<UPlacementPropData*> AvailableRecipes;

	UFUNCTION(BlueprintCallable, Category = "Construction|UI")
	void InitializeMenu(UPlayerConstructionObject* InBuilder, const TArray<UPlacementPropData*>& InRecipes);

	// Overriding custom open/close if needed, but PrismRadialMenu already has OpenMenu()
	// We might just want to hide the base class one or call it, but since we want custom input mode we can rename ours
	UFUNCTION(BlueprintCallable, Category = "Construction|UI")
	void OpenBuilderMenu();

	UFUNCTION(BlueprintCallable, Category = "Construction|UI")
	void CloseBuilderMenu();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleSegmentSelected(FName SegmentID);
};
