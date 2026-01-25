#pragma once
#include "CoreMinimal.h"
#include "Components/SizeBox.h"
#include "UI/Editor/JupiterPageBase.h"
#include "Page_UnitSpawn.generated.h"

class UUnitSpawnComponent;
class UUnitsEntryWidget;
class UWrapBox;
class UBorder;
class UButton;
class UUnitsSelectionDataAsset;
class UEditableTextBox;
class UUnitSpawnCountWidget;
class UUnitSpawnFormationWidget;
class UUnitSpawnAxisWidget;
class UCustomButtonWidget;


UCLASS()
class JUPITERPLUGIN_API UPage_UnitSpawn : public UJupiterPageBase
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	// --- UJupiterPageBase Interface ---
	virtual void InitPage(UUnitSpawnComponent* SpawnComp, UUnitPatrolComponent* PatrolComp, UUnitSelectionComponent* SelComp) override;
	virtual void OnPageOpened() override;
	// ----------------------------------

protected:
	UFUNCTION()
	void SetupUnitsList();

	UFUNCTION()
	void ApplyFilters();

	UFUNCTION()
	void InitializeInspectorWidgets();

	// --- Callbacks ---

	UFUNCTION()
	void OnShowUnitSelectionPressed();

	UFUNCTION()
	void OnUnitSelected(UCustomButtonWidget* Button, int Index);

	UFUNCTION()
	void OnCategoryButtonClicked(UCustomButtonWidget* Button, int Index);

	UFUNCTION()
	void OnSearchTextChanged(const FText& Text);

	UFUNCTION()
	void OnInspectorToggleClicked(UCustomButtonWidget* Button, int Index);

	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;

	// --- Helpers ---
	
	void SetupCategoryButtons();
	void UpdateCategoryButtonSelection(UCustomButtonWidget* SelectedButton);

protected:
	// --- Config ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TSubclassOf<UUnitsEntryWidget> UnitsEntryClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TSubclassOf<UCustomButtonWidget> CategoryButtonClass;

	UPROPERTY(EditAnywhere, Category = "Settings")
	TArray<UUnitsSelectionDataAsset*> UnitsSelectionDataAssets;

	// --- UI Elements ---

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UButton* Btn_ShowUnitsSelection;
	

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UWrapBox* WrapBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UBorder* ListBorder;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UWrapBox* CategoryWrapBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UEditableTextBox* SearchTextBox;

	// --- Inspector ---

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	USizeBox* InspectorContainer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UCustomButtonWidget* InspectorToggleButton;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* Anim_ToggleInspector;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UUnitSpawnCountWidget* SpawnCountWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UUnitSpawnFormationWidget* SpawnFormationWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UUnitSpawnAxisWidget* SpawnAxisWidget;

	// --- State ---

	bool bIsInspectorOpen = true;

	UPROPERTY()
	TArray<UUnitsEntryWidget*> EntryList;

	UPROPERTY(Transient)
	FString CurrentSearchText;

	UPROPERTY(Transient)
	FName CurrentTagFilter = NAME_None;

	UPROPERTY()
	TArray<UCustomButtonWidget*> CategoryButtons;

	UPROPERTY()
	TMap<UCustomButtonWidget*, FName> CategoryButtonTagMap;

	UPROPERTY()
	TArray<FName> CachedCategoryTags;
};
