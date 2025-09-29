#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JupiterHudWidget.generated.h"

class UTextBlock;
class UBorder;
class UCustomButtonWidget;
class UUnitsSelectionWidget;
class UUnitSelectionComponent;
class UWidgetSwitcher;
class USelectBehaviorWidget;
class UFormationSelectorWidget;

UCLASS()
class JUPITERPLUGIN_API UJupiterHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UWidgetSwitcher* SelectorSwitcher;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UBorder* UnitActionBorder;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* UnitsSelected;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* ActionTitle;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UFormationSelectorWidget* FormationSelector;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	USelectBehaviorWidget* BehaviorSelector;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UUnitsSelectionWidget* UnitsSelector;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UCustomButtonWidget* Btn_Switcher;

	UFUNCTION(BlueprintCallable)
	void InitializedJupiterHud(APawn* PawnLinked);
	
	UFUNCTION()
	void OnSelectionUpdated();

protected:
	UFUNCTION()
	void SetFormationSelectionWidget(const bool bEnabled) const;
	
	UFUNCTION()
	void SetBehaviorSelectionWidget(bool bEnabled) const;
	
	UFUNCTION()
	void SetUnitsSelectionWidget(bool bEnabled) const;

	UFUNCTION()
	void SwitchUnitAction(UCustomButtonWidget* Button, int Index);

	UFUNCTION()
	void Switch(FString Type) const;

    UPROPERTY()
    UUnitSelectionComponent* SelectionComponent;
};
