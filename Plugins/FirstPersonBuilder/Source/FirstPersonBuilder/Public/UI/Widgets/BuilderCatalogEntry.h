// Copyright 2026

#pragma once

#include "CoreMinimal.h"
#include "Components/PrismButtonBase.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "BuilderCatalogEntry.generated.h"

class UPlacementPropData;
class UTextBlock;

/**
 * An entry widget used in the Builder Catalog TileView.
 * Shows a thumbnail, item name (uppercase), and category tag.
 * Inherits from PrismButtonBase for DataAsset styling + click handling.
 */
UCLASS()
class FIRSTPERSONBUILDER_API UBuilderCatalogEntry : public UPrismButtonBase, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	UBuilderCatalogEntry(const FObjectInitializer& ObjectInitializer);

	// --- Theme ---
	void ApplyTheme(class UBuilderMenuTheme* InTheme);

	// --- IUserObjectListEntry ---
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;

	FORCEINLINE UPlacementPropData* GetPropData() const { return PropData; }

protected:
	virtual void BuildDefaultLayout() override;
	virtual bool TickTransitions(float DeltaTime) override;
	virtual void OnStateChanged(EPrismWidgetState InNewState) override;


private:
	bool bWasHovered = false;

	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInterface> CardMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UPlacementPropData> PropData;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CategoryTagText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EntryNameText;

	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInstanceDynamic> CardMID;

	UPROPERTY(Transient)
	TObjectPtr<class UBuilderMenuTheme> CachedTheme;

	/** Tracks whether this entry is currently selected in the TileView */
	bool bEntrySelected = false;

	/** Hover/Press interpolation state */
	float CurrentHoverInterp = 0.0f;
	float TargetHoverInterp = 0.0f;
	float CurrentPressInterp = 0.0f;
	float TargetPressInterp = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<class UImage> CardBackground;

	UPROPERTY(Transient)
	TObjectPtr<class UOverlay> CardOverlayRoot;

};
