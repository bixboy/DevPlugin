#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "Widget/CustomButtonWidget.h"
#include "UnitsSelectionWidget.generated.h"

class UUnitsEntryWidget;
class UWrapBox;
class UBorder;
class UButton;
class UUnitsSelectionDataAsset;
class UEditableTextBox;
class UUnitSpawnComponent;
class UComboBoxString;
enum class ESpawnFormation : uint8;


UCLASS()
class JUPITERPLUGIN_API UUnitsSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;
	
	UFUNCTION()
	void SetupUnitsList();
	
protected:

	UFUNCTION()
	void OnShowUnitSelectionPressed();

    UFUNCTION()
    void OnUnitSelected(UCustomButtonWidget* Button, int Index);

    UFUNCTION()
    void OnCategoryButtonClicked(UCustomButtonWidget* Button, int Index);

    UFUNCTION()
    void OnSearchTextChanged(const FText& Text);

    UFUNCTION()
    void OnSpawnCountTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void OnIncreaseSpawnCount(UCustomButtonWidget* Button, int Index);

    UFUNCTION()
    void OnDecreaseSpawnCount(UCustomButtonWidget* Button, int Index);

    UFUNCTION()
    void HandleSpawnCountChanged(int32 NewCount);

    UFUNCTION()
    void HandleSpawnFormationChanged(ESpawnFormation NewFormation);

    UFUNCTION()
    void HandleCustomFormationDimensionsChanged(FIntPoint NewDimensions);

    UFUNCTION()
    void OnFormationOptionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnCustomFormationXCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void OnCustomFormationYCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    void ApplySpawnCount(int32 NewCount);
    void RefreshSpawnCountDisplay() const;
    void SetupCategoryButtons();
    void UpdateCategoryButtonSelection(UCustomButtonWidget* SelectedButton);
    void ApplyFilters();
    void InitializeFormationOptions();
    void UpdateFormationSelection() const;
    void RefreshCustomFormationInputs() const;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
        TSubclassOf<UUnitsEntryWidget> UnitsEntryClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
    TSubclassOf<UCustomButtonWidget> CategoryButtonClass;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* Btn_ShowUnitsSelection;
	
    UPROPERTY(EditAnywhere, Category = "Settings")
    TArray<UUnitsSelectionDataAsset*> UnitsSelectionDataAssets;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UWrapBox* WrapBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UBorder* ListBorder;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UWrapBox* CategoryWrapBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UEditableTextBox* SpawnCountTextBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UEditableTextBox* SearchTextBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UCustomButtonWidget* Btn_IncreaseSpawnCount;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UCustomButtonWidget* Btn_DecreaseSpawnCount;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UComboBoxString* FormationDropdown;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UEditableTextBox* CustomFormationXTextBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UEditableTextBox* CustomFormationYTextBox;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (ClampMin = "1"))
    int32 MaxSpawnCount = 50;

    UPROPERTY()
    TArray<UUnitsEntryWidget*> EntryList;

    UPROPERTY()
    UUnitSpawnComponent* SpawnComponent = nullptr;

    UPROPERTY(Transient)
    int32 CachedSpawnCount = 1;

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

    UPROPERTY(Transient)
    TMap<FString, ESpawnFormation> OptionToFormation;

    UPROPERTY(Transient)
    TMap<ESpawnFormation, FString> FormationToOption;

    bool bUpdatingFormationSelection = false;
};
