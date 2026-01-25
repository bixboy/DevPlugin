#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CustomSliderWidget.generated.h"

class UProgressBar;
class USlider;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCustomSliderValueChanged, float, Value);

UCLASS()
class JUPITERPLUGIN_API UCustomSliderWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	virtual void NativeOnInitialized() override;

	// --- Commandes Publiques ---

	UFUNCTION(BlueprintCallable, Category = "Custom Slider")
	void SetValue(float InValue);

	UFUNCTION(BlueprintCallable, Category = "Custom Slider")
	float GetValue() const;

	UFUNCTION(BlueprintCallable, Category = "Custom Slider")
	void SetMinMax(float InMin, float InMax);

	UFUNCTION(BlueprintCallable, Category = "Custom Slider")
	void SetFillColor(FLinearColor InColor);

	// --- Event ---
	
	UPROPERTY(BlueprintAssignable, Category = "Custom Slider")
	FCustomSliderValueChanged OnValueChanged;

protected:
	
	UFUNCTION()
	void HandleSliderValueChanged(float InValue);

	void UpdateProgressBar();

	// --- UI Bindings ---

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UProgressBar* SliderProgressBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	USlider* InputSlider;

	// --- Settings ---

	UPROPERTY(EditAnywhere, Category = "Custom Slider")
	float Value = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Custom Slider")
	float MinValue = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Custom Slider")
	float MaxValue = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Custom Slider")
	FLinearColor FillColor = FLinearColor(0.0f, 0.45f, 1.0f, 1.0f);
};