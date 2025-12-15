#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "Widget/CustomButtonWidget.h"
#include "UnitsSelectionWidget.generated.h"

class UUnitPatrolComponent;
class UUnitsEntryWidget;
class UWrapBox;
class UBorder;
class UButton;
class UUnitsSelectionDataAsset;
class UEditableTextBox;
class UUnitSpawnComponent;
class UUnitSpawnCountWidget;
class UUnitSpawnFormationWidget;
class UUnitSpawnAxisWidget;
class UWidgetSwitcher;
class UPatrolListWidget;

UCLASS()
class JUPITERPLUGIN_API UUnitsSelectionWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;

    UFUNCTION()
    void SetupUnitsList();

    void SetPatrolComponent(UUnitPatrolComponent* PatrolComp);

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
    void SetupCategoryButtons();

    UFUNCTION()
    void UpdateCategoryButtonSelection(UCustomButtonWidget* SelectedButton);

    UFUNCTION()
    void ApplyFilters();

    void InitializeSpawnOptionWidgets();

    UFUNCTION()
    void OnCountOptionSelected(UCustomButtonWidget* Button, int Index);

    UFUNCTION()
    void OnFormationOptionSelected(UCustomButtonWidget* Button, int Index);

    void UpdateSpawnOptionButtons(UCustomButtonWidget* SelectedButton);
    void UpdateSpawnOptionVisibility(bool bShowCountOptions);

    /* ---------- PROPRIÉTÉS ---------- */

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
    UEditableTextBox* SearchTextBox;

    /* -------- Sous-widgets d’options de spawn -------- */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UCustomButtonWidget* CountOptionButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UCustomButtonWidget* FormationOptionButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UUnitSpawnCountWidget* SpawnCountWidget;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UUnitSpawnFormationWidget* SpawnFormationWidget;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UUnitSpawnAxisWidget* SpawnAxisWidget;

    /* -------- Navigation & Pages -------- */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UWidgetSwitcher* ContentSwitcher;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UCustomButtonWidget* Btn_OpenSpawnPage;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UCustomButtonWidget* Btn_OpenPatrolPage;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UPatrolListWidget* PatrolEditorPage;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UWidget* SpawnPageContainer;

    UFUNCTION()
    void OnOpenSpawnPage(UCustomButtonWidget* Button, int Index);

    UFUNCTION()
    void OnOpenPatrolPage(UCustomButtonWidget* Button, int Index);

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

    UPROPERTY()
    UUnitSpawnComponent* SpawnComponent;
};
