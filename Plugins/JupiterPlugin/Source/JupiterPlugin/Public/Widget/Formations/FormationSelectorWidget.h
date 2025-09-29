#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/AiData.h"
#include "FormationSelectorWidget.generated.h"

class USlider;
class UComboBoxString;
class UUnitFormationComponent;
namespace ESelectInfo { enum Type; }

UCLASS()
class JUPITERPLUGIN_API UFormationSelectorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UComboBoxString* FormationDropdown;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    USlider* SpacingSlider;

protected:
	UFUNCTION()
	void OnFormationSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnSpacingSliderValueChanged(const float Value);

	UFUNCTION()
	void OnFormationStateChanged();

	void InitializeFormationOptions();
	void ApplyFormationSelection(const FString& SelectedOption);
	void UpdateSelectedFormation(EFormation Formation);
	void UpdateSpacingFromComponent();

	UPROPERTY()
	UUnitFormationComponent* FormationComponent;

	TMap<FString, EFormation> OptionToFormation;

	TMap<EFormation, FString> FormationToOption;

	UPROPERTY()
	bool bIsUpdatingSelection = false;

	UPROPERTY()
	bool bIsUpdatingSpacing = false;
};
