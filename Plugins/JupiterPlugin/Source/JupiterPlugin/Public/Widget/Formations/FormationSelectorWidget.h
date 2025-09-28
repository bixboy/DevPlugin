#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FormationSelectorWidget.generated.h"

class USlider;
class UComboBoxString;
namespace ESelectInfo
{
        enum Type;
}
class UUnitFormationComponent;

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

        /** Keeps the dropdown labels associated with their enum values. */
        TMap<FString, EFormation> OptionToFormation;

        /** Reverse lookup to quickly select the appropriate option. */
        TMap<EFormation, FString> FormationToOption;

        /** Prevents recursive updates while synchronizing with the component. */
        bool bIsUpdatingSelection = false;

        /** Prevents feedback loops when updating the spacing slider programmatically. */
        bool bIsUpdatingSpacing = false;
};
