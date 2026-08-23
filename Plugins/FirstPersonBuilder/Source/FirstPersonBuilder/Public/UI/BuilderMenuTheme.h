// Copyright 2026

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "BuilderMenuTheme.generated.h"

/**
 * Defines a primary category displayed as a top tab in the catalog menu.
 */
USTRUCT(BlueprintType)
struct FIRSTPERSONBUILDER_API FBuilderPrimaryCategory
{
	GENERATED_BODY()

	/** GameplayTag matching recipes for this category (e.g. Category.Structures, Category.Infrastructure) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Category", meta = (Categories = "Category"))
	FGameplayTag CategoryTag;

	/** Display label for the top tab (leave empty to use tag name) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Category")
	FText DisplayName = FText::GetEmpty();
};

/**
 * Data Asset to specifically parameterize the Builder Catalog UI.
 */
UCLASS(BlueprintType, Category = "Builder UI")
class FIRSTPERSONBUILDER_API UBuilderMenuTheme : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Manually configured Primary Categories displayed along the top bar */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder UI|Categories")
	TArray<FBuilderPrimaryCategory> PrimaryCategories;

	/** The material applied to the retainer box of each catalog card (e.g. for cut corners) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder UI|Cards")
	TObjectPtr<class UMaterialInterface> CardRetainerMaterial;

	/** Thickness of the outline on the cards */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder UI|Cards")
	float OutlineThickness = 0.04f;

	// --- Theme Colors ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder UI|Colors")
	FLinearColor PrimaryHighlightColor = FLinearColor(0.35f, 0.85f, 1.0f, 1.0f); // Electric Cyan

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder UI|Colors")
	FLinearColor AccentOrangeColor = FLinearColor(1.0f, 0.5f, 0.1f, 1.0f); // High-Vis Orange

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder UI|Colors")
	FLinearColor WireframeColor = FLinearColor(0.18f, 0.28f, 0.38f, 0.45f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder UI|Colors")
	FLinearColor MainWindowBgColor = FLinearColor(0.015f, 0.020f, 0.028f, 0.94f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder UI|Colors")
	FLinearColor TopHeaderBgColor = FLinearColor(0.035f, 0.050f, 0.070f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder UI|Colors")
	FLinearColor SidebarBgColor = FLinearColor(0.020f, 0.030f, 0.040f, 0.80f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder UI|Colors")
	FLinearColor DetailPanelBgColor = FLinearColor(0.025f, 0.035f, 0.050f, 0.90f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder UI|Colors")
	FLinearColor TextPrimaryColor = FLinearColor(0.90f, 0.96f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder UI|Colors")
	FLinearColor TextSecondaryColor = FLinearColor(0.55f, 0.68f, 0.78f, 0.80f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder UI|Colors")
	FLinearColor TextMutedColor = FLinearColor(0.35f, 0.45f, 0.55f, 0.60f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder UI|Colors")
	FLinearColor EntryBgColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder UI|Colors")
	FLinearColor EntryHoverBgColor = FLinearColor(0.2f, 0.5f, 0.8f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder UI|Colors")
	FLinearColor EntrySelectedBgColor = FLinearColor(0.35f, 0.85f, 1.0f, 0.25f);

	// --- Icons ---
	/** Icon texture displayed for the unfavorited (outline) state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder UI|Icons")
	TObjectPtr<class UTexture2D> StarOutlineIcon;

	/** Icon texture displayed when an item is favorited (filled) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder UI|Icons")
	TObjectPtr<class UTexture2D> StarFilledIcon;

	/** Size of the favorite star icon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder UI|Icons")
	FVector2D FavoriteIconSize = FVector2D(16.0f, 16.0f);
};
