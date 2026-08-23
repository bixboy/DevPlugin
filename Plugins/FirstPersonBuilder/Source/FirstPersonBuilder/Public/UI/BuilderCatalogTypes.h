// Copyright 2026

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "BuilderCatalogTypes.generated.h"

class UTextBlock;
class UHorizontalBox;
class UImage;
class UPrismButtonBase;
class UBuilderCatalogMenu;

/**
 * Text scramble animation target tracker.
 */
USTRUCT()
struct FIRSTPERSONBUILDER_API FBuilderTextScrambleTarget
{
	GENERATED_BODY()

	TWeakObjectPtr<UTextBlock> TargetTextBlock;
	FString FinalText;
	float ElapsedTime = 0.0f;
	float Duration = 0.25f;
	int32 TotalCharacters = 0;
	bool bIsComplete = false;
};

/**
 * Reusable pooled resource cost badge to eliminate runtime GC allocations.
 */
USTRUCT()
struct FIRSTPERSONBUILDER_API FBuilderResourceCostBadge
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> BadgeBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UImage> IconImage = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LabelText = nullptr;
};

/**
 * Proxy object used to route sidebar subcategory button clicks.
 */
UCLASS()
class FIRSTPERSONBUILDER_API UBuilderCategoryProxy : public UObject
{
	GENERATED_BODY()

public:
	FGameplayTag CategoryTag;
	TWeakObjectPtr<UBuilderCatalogMenu> Menu;

	UFUNCTION()
	void OnClicked(UPrismButtonBase* Button);
};

/**
 * Proxy object used to route top primary category tab clicks.
 */
UCLASS()
class FIRSTPERSONBUILDER_API UBuilderPrimaryCategoryProxy : public UObject
{
	GENERATED_BODY()

public:
	int32 PrimaryIndex = 0;
	FGameplayTag PrimaryTag;
	TWeakObjectPtr<UBuilderCatalogMenu> Menu;

	UFUNCTION()
	void OnClicked(UPrismButtonBase* Button);
};
