#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/AiData.h"
#include "SelectBehaviorWidget.generated.h"

class UComboBoxString;
namespace ESelectInfo
{
        enum Type;
}
class UUnitSelectionComponent;
class UUnitOrderComponent;

UCLASS()
class JUPITERPLUGIN_API USelectBehaviorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

        UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
        UComboBoxString* BehaviorDropdown;

protected:
        UFUNCTION()
        void OnBehaviorSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

        UFUNCTION()
        void OnNewUnitSelected();

        void InitializeBehaviorOptions();
        void ApplyBehaviorSelection(const FString& SelectedOption);
        void UpdateSelectedBehavior(ECombatBehavior Behavior);

        UPROPERTY()
        UUnitSelectionComponent* SelectionComponent;

        UPROPERTY()
        UUnitOrderComponent* OrderComponent;

        /** Mapping between dropdown labels and combat behavior values. */
        TMap<FString, ECombatBehavior> OptionToBehavior;

        /** Reverse mapping to quickly update the dropdown selection. */
        TMap<ECombatBehavior, FString> BehaviorToOption;

        /** Avoids propagating behavior updates when changing the selection programmatically. */
        bool bIsUpdatingSelection = false;
};
