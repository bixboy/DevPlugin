#include "UI/CustomSliderWidget.h"
#include "Components/ProgressBar.h"
#include "Components/Slider.h"

void UCustomSliderWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (SliderProgressBar)
	{
		SliderProgressBar->SetFillColorAndOpacity(FillColor);
	}

	if (InputSlider)
	{
		InputSlider->SetMinValue(MinValue);
		InputSlider->SetMaxValue(MaxValue);
		InputSlider->SetValue(Value);
	}

	UpdateProgressBar();
}

void UCustomSliderWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (InputSlider)
	{
		InputSlider->OnValueChanged.AddDynamic(this, &UCustomSliderWidget::HandleSliderValueChanged);
	}
}

void UCustomSliderWidget::SetValue(float InValue)
{
	Value = FMath::Clamp(InValue, MinValue, MaxValue);
	
	if (InputSlider)
	{
		InputSlider->SetValue(Value);
	}

	UpdateProgressBar();
}

float UCustomSliderWidget::GetValue() const
{
	return Value;
}

void UCustomSliderWidget::SetMinMax(float InMin, float InMax)
{
	MinValue = InMin;
	MaxValue = InMax;

	if (InputSlider)
	{
		InputSlider->SetMinValue(MinValue);
		InputSlider->SetMaxValue(MaxValue);
	}

	SetValue(Value);
}

void UCustomSliderWidget::SetFillColor(FLinearColor InColor)
{
	FillColor = InColor;
	if (SliderProgressBar)
	{
		SliderProgressBar->SetFillColorAndOpacity(FillColor);
	}
}

void UCustomSliderWidget::HandleSliderValueChanged(float InValue)
{
	Value = InValue;
	UpdateProgressBar();
	OnValueChanged.Broadcast(Value);
}

void UCustomSliderWidget::UpdateProgressBar()
{
	if (!SliderProgressBar)
		return;
	
	float Range = MaxValue - MinValue;
	float Percent = (Range > KINDA_SMALL_NUMBER) ? (Value - MinValue) / Range : 0.0f;

	SliderProgressBar->SetPercent(Percent);
}