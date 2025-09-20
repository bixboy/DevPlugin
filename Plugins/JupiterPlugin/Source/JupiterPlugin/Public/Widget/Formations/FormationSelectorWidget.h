#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FormationSelectorWidget.generated.h"

class UCustomButtonWidget;
class USlider;
class UFormationButtonWidget;
class UUnitFormationComponent;

UCLASS()
class JUPITERPLUGIN_API UFormationSelectorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UFormationButtonWidget* LineButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UFormationButtonWidget* ColumnButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UFormationButtonWidget* WedgeButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UFormationButtonWidget* BlobButton;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UFormationButtonWidget* SquareButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	USlider* SpacingSlider;

protected:
	UFUNCTION()
	void OnFormationButtonClicked(UCustomButtonWidget* Button, int Index);

	UFUNCTION()
	void OnSpacingSliderValueChanged(const float Value);

        UPROPERTY()
        UUnitFormationComponent* FormationComponent;
};
