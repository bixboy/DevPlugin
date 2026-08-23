// Copyright 2026

#pragma once

#include "CoreMinimal.h"
#include "PrismWidgetBase.h"
#include "GameplayTagContainer.h"
#include "UI/BuilderCatalogTypes.h"
#include "UI/Widgets/BuilderCategoryButton.h"
#include "BuilderCatalogMenu.generated.h"

class UPlacementPropData;
class UPlayerConstructionObject;
class UBuilderCatalogEntry;
class UBuilderMenuTheme;
class UTextBlock;
class UEditableTextBox;
class UTileView;
class UVerticalBox;
class UHorizontalBox;
class UBorder;
class UImage;
class UWrapBox;

/**
 * A sci-fi catalog menu for selecting props in First Person Builder.
 * Features: category sidebar, search, paginated grid, detail bar, favorites.
 */
UCLASS(BlueprintType, Blueprintable)
class FIRSTPERSONBUILDER_API UBuilderCatalogMenu : public UPrismWidgetBase
{
	GENERATED_BODY()

public:
	UBuilderCatalogMenu(const FObjectInitializer& ObjectInitializer);

	/** The builder object this menu controls */
	UPROPERTY(BlueprintReadOnly, Category = "Construction|UI")
	TWeakObjectPtr<UPlayerConstructionObject> OwningBuilder;

	/** All available recipes (set via InitializeMenu) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|UI")
	TArray<UPlacementPropData*> AvailableRecipes;

	/** Entry widget class for the UTileView */
	UPROPERTY(EditDefaultsOnly, Category = "Construction|UI")
	TSubclassOf<UBuilderCatalogEntry> EntryWidgetClass;

	/** Specific theme for the builder menu */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|UI")
	TObjectPtr<UBuilderMenuTheme> MenuTheme;

	/** Number of items displayed per page */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|UI")
	int32 ItemsPerPage = 8;

	// --- Lifecycle ---
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void BuildDefaultLayout() override;

	// --- Public API ---
	void InitializeMenu(UPlayerConstructionObject* InBuilder, const TArray<UPlacementPropData*>& InRecipes);
	void PopulateGridForCategory(const FGameplayTag& CategoryTag);
	void ShowFavorites();
	void ToggleFavorite();
	bool IsFavorite(UPlacementPropData* InProp) const;
	void NextPage();
	void PrevPage();

	void SelectPrimaryCategory(int32 InIndex);
	void UpdatePrimaryCategoryTabs();
	void UpdateSubcategoriesForPrimary(int32 InPrimaryIndex);

	UFUNCTION(BlueprintCallable, Category = "Builder|UI")
	void OpenBuilderMenu();

	UFUNCTION(BlueprintCallable, Category = "Builder|UI")
	void CloseBuilderMenu();

protected:
	// --- Modular Layout Builders ---
	void BuildWindowHeader(UVerticalBox* InRootVBox);
	void BuildLeftSidebar(UHorizontalBox* InContentHBox);
	void BuildRightArea(UHorizontalBox* InContentHBox);
	void BuildTopCategoryCapsule(UVerticalBox* InRightAreaVBox);
	void BuildPropGrid(UVerticalBox* InRightAreaVBox);
	void BuildBottomDetailPanel(UVerticalBox* InRightAreaVBox);
	void BuildWindowFooter(UVerticalBox* InRootVBox);

	// --- Event Handlers ---
	void HandleEntryClicked(UObject* Item);
	void HandleEntryGenerated(UUserWidget& Widget);

	UFUNCTION()
	void HandleEntryButtonClicked(UPrismButtonBase* Button);

	UFUNCTION()
	void HandlePlaceClicked(UPrismButtonBase* Button);

	UFUNCTION()
	void HandleFavoriteClicked(UPrismButtonBase* Button);

	UFUNCTION()
	void HandleFavoritesCategoryClicked(UPrismButtonBase* Button);

	UFUNCTION()
	void HandlePrevPageClicked(UPrismButtonBase* Button);

	UFUNCTION()
	void HandleNextPageClicked(UPrismButtonBase* Button);

	UFUNCTION()
	void HandleBackClicked(UPrismButtonBase* Button);

	UFUNCTION()
	void HandleSearchTextChanged(const FText& InText);

	void UpdateCategoryList();
	void FilterAndPopulate();
	void UpdateDetailBar(UPlacementPropData* InProp);
	void UpdateResourceCosts(UPlacementPropData* InProp);
	void UpdatePagination();
	void UpdateCategoryHighlight(UBuilderCategoryButton* InNewActive);
	void UpdatePrimaryTabHighlight(int32 InActiveIndex);
	void SelectItem(UPlacementPropData* InProp);

	// --- Sci-Fi Text Animation ---
	void StartScrambleText(UTextBlock* InTextBlock, const FString& InFinalText, float InDuration = 0.25f);
	void UpdateScrambleAnimation();

private:
	// --- Animation ---
	FTimerHandle ScrambleTimerHandle;
	TArray<FBuilderTextScrambleTarget> ActiveScrambleTargets;

	// --- UI Elements ---
	UPROPERTY(Transient)
	TObjectPtr<UBorder> MainBackground;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> TopCategoryBar;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> CategorySidebar;

	UPROPERTY(Transient)
	TObjectPtr<UTileView> PropGrid;

	TSubclassOf<UUserWidget> GetCatalogEntryClass(UObject* Item);

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CategoryHeaderText;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> SearchBox;

	// --- Bottom Detail Bar ---
	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> BottomDetailBar;

	UPROPERTY(Transient)
	TObjectPtr<UImage> DetailThumbnail;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailNameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailBadgeText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailDescriptionText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailStatsText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailCol1Text;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailCol2Text;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailCol3Text;

	UPROPERTY(Transient)
	TObjectPtr<UWrapBox> CostContainer;

	UPROPERTY(Transient)
	TArray<FBuilderResourceCostBadge> CostBadgePool;

	UPROPERTY(Transient)
	TObjectPtr<UBuilderCategoryButton> PlaceButton;

	UPROPERTY(Transient)
	TObjectPtr<UBuilderCategoryButton> FavoriteButton;

	UPROPERTY(Transient)
	TObjectPtr<UBuilderCategoryButton> BackButton;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> PaginationContainer;

	UPROPERTY(Transient)
	TObjectPtr<UBuilderCategoryButton> PrevPageButton;

	UPROPERTY(Transient)
	TObjectPtr<UBuilderCategoryButton> NextPageButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PageText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> SlidingIndicatorBorder;

	float CurrentIndicatorX = 0.0f;
	float TargetIndicatorX = 0.0f;
	float TabWidth = 160.0f;
	FTimerHandle TabSlideTimerHandle;

	void UpdateTabSlideAnimation();
	void AnimateTabTo(int32 InIndex);

	// --- State ---
	TMap<FGameplayTag, TArray<UPlacementPropData*>> PropsByCategory;
	int32 SelectedPrimaryCategoryIndex = 0;
	FGameplayTag SelectedPrimaryCategoryTag;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBuilderCategoryButton>> PrimaryCategoryButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBuilderPrimaryCategoryProxy>> PrimaryCategoryProxies;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBuilderCategoryProxy>> CategoryProxies;

	FGameplayTag ActiveCategoryTag;

	UPROPERTY(Transient)
	TObjectPtr<UPlacementPropData> SelectedPropData;

	TWeakObjectPtr<UBuilderCategoryButton> ActiveCategoryButton;

	UPROPERTY(Transient)
	TObjectPtr<UBuilderCategoryButton> FavoritesCategoryButton;

	TSet<FName> FavoriteIDs;
	bool bShowingFavorites = false;
	int32 CurrentPage = 0;
	FString SearchFilter;
	TArray<UPlacementPropData*> FilteredItems;
};
