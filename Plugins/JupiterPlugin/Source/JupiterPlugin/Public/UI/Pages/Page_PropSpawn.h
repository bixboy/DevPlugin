#pragma once
#include "CoreMinimal.h"
#include "UI/Editor/JupiterPageBase.h"
#include "Page_PropSpawn.generated.h"

class USizeBox;
class UPlacementItemData;
class UPropsEntryWidget;
class UPresetEntryWidget;
class UPresetManagerComponent;
class UWrapBox;
class UScrollBox;
class UBorder;
class UEditableTextBox;
class UCustomButtonWidget;


UCLASS()
class JUPITERPLUGIN_API UPage_PropSpawn : public UJupiterPageBase
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	// --- UJupiterPageBase Interface ---
	virtual void InitPage(UCameraPlacementSystem* PlacementSys, UUnitPatrolComponent* PatrolComp, UUnitSelectionComponent* SelComp) override;
	virtual void OnPageOpened() override;

protected:
	// --- Setup ---

	UFUNCTION()
	void SetupItemsList();

	UFUNCTION()
	void SetupPresetList();

	UFUNCTION()
	void ApplyFilters();

	// --- Item Callbacks ---

	UFUNCTION()
	void OnItemSelected(UCustomButtonWidget* Button, int Index);

	UFUNCTION()
	void OnCategoryButtonClicked(UCustomButtonWidget* Button, int Index);

	UFUNCTION()
	void OnSearchTextChanged(const FText& Text);

	// --- Preset Callbacks ---
	
	UFUNCTION()
	void OnOpenPresetClicked(UCustomButtonWidget* Button, int Index);

	UFUNCTION()
	void OnSavePresetClicked(UCustomButtonWidget* Button, int Index);
	
	UFUNCTION()
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;

	UFUNCTION()
	void OnPresetSpawnClicked(UCustomButtonWidget* Button, int Index);

	UFUNCTION()
	void OnPresetDeleteClicked(UCustomButtonWidget* Button, int Index);

	UFUNCTION()
	void OnPresetsChanged();

	// --- Helpers ---
	void SetupCategoryButtons();
	void UpdateCategoryButtonSelection(UCustomButtonWidget* SelectedButton);

protected:
	// --- Config ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TSubclassOf<UPropsEntryWidget> PropsEntryClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TSubclassOf<UPresetEntryWidget> PresetEntryClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TSubclassOf<UCustomButtonWidget> CategoryButtonClass;

	UPROPERTY(EditAnywhere, Category = "Settings")
	TArray<UPlacementItemData*> PlacementProps;

	// --- Items Section ---

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWrapBox> WrapBox_Items;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> CategoryWrapBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> SearchTextBox;

	// --- Presets Section ---

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UScrollBox> ScrollBox_Presets;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> PresetContainer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> Input_PresetName;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCustomButtonWidget> Btn_OpenPreset;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCustomButtonWidget> Btn_SavePreset;
	
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* Anim_SavePreset;

	// --- State ---

	UPROPERTY()
	TArray<UPropsEntryWidget*> EntryList;

	UPROPERTY()
	TArray<UPresetEntryWidget*> PresetWidgets;

	UPROPERTY()
	TArray<UCustomButtonWidget*> CategoryButtons;

	UPROPERTY()
	TMap<UCustomButtonWidget*, FName> CategoryButtonTagMap;

	UPROPERTY()
	TArray<FName> CachedCategoryTags;

	UPROPERTY(Transient)
	FString CurrentSearchText;

	UPROPERTY(Transient)
	FName CurrentTagFilter = NAME_None;

	UPROPERTY()
	TWeakObjectPtr<UPresetManagerComponent> PresetManager;
	
	bool bPresetIsOpen = false;
};
