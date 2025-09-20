#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JupiterHudWidget.generated.h"

class UUnitsSelectionWidget;
class USelectionComponent;
class UButton;
class UWidgetSwitcher;
class USelectBehaviorWidget;
class UFormationSelectorWidget;

UCLASS()
class JUPITERPLUGIN_API UJupiterHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UWidgetSwitcher* SelectorSwitcher;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UFormationSelectorWidget* FormationSelector;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	USelectBehaviorWidget* BehaviorSelector;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UUnitsSelectionWidget* UnitsSelector;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* Btn_SwitchBehavior;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* Btn_SwitchFormation;

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

	UPROPERTY()
	USelectionComponent* SelectionComponent;
};
