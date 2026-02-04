#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/TextBlock.h"
#include "Data/Placement/PlacementTypes.h"
#include "UnitSpawnFormationWidget.generated.h"

class UComboBoxString;
class UCameraPlacementSystem;


UCLASS()
class JUPITERPLUGIN_API UUnitSpawnFormationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="Jupiter|UI")
	void SetupWithSystem(UCameraPlacementSystem* InPlacementSystem);

protected:
	UFUNCTION()
	void OnFormationChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	void InitializeFormationOptions();

	UFUNCTION()
	void UpdateSelectionFromSystem(ESpawnFormation NewFormation);
	void UpdateSelectionFromSystem();

	UFUNCTION()
	UWidget* HandleGenerateWidget(FString Item);

	void UpdateMainDisplay(const FString& SelectedItem);
    
	UPROPERTY(EditAnywhere, Category = "Jupiter|UI")
	TMap<ESpawnFormation, UTexture2D*> FormationIcons;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> FormationDropdown;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> SelectedIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedLabel;

	UPROPERTY()
	TWeakObjectPtr<UCameraPlacementSystem> PlacementSystem;

	TMap<FString, ESpawnFormation> OptionToFormation;
	TMap<ESpawnFormation, FString> FormationToOption;
    
	bool bUpdatingSelection = false;
};