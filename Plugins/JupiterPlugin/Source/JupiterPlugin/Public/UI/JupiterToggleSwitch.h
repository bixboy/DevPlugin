#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Styling/SlateTypes.h"
#include "Components/Widget.h"
#include "Styling/CoreStyle.h"
#include "JupiterToggleSwitch.generated.h"

DECLARE_DELEGATE_OneParam(FOnToggleSwitchChangedNative, bool);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnToggleSwitchChanged, bool, bIsToggled);

/**
 * Slate Widget for the Toggle Switch
 */
class JUPITERPLUGIN_API SJupiterToggleSwitch : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SJupiterToggleSwitch)
		: _IsToggled(false)
		, _TintColor(FLinearColor(0.0f, 0.45f, 1.0f, 1.0f))
		, _InactiveColor(FLinearColor(0.2f, 0.2f, 0.2f, 1.0f))
		, _ThumbColor(FLinearColor::White)
		, _SwitchSize(FVector2D(50.0f, 28.0f))
	{}
		SLATE_ATTRIBUTE(bool, IsToggled)
		SLATE_ATTRIBUTE(FLinearColor, TintColor)
		SLATE_ATTRIBUTE(FLinearColor, InactiveColor)
		SLATE_ATTRIBUTE(FLinearColor, ThumbColor)
		SLATE_ATTRIBUTE(FVector2D, SwitchSize)
		SLATE_EVENT(FOnToggleSwitchChangedNative, OnToggled)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// SWidget interface
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	// End SWidget interface

	void SetIsToggled(bool bToggled);
	void SetTintColor(FLinearColor InColor);
	void SetInactiveColor(FLinearColor InColor);
	void SetThumbColor(FLinearColor InColor);
	void SetSwitchSize(FVector2D InSize);

private:
	TAttribute<bool> IsToggled;
	TAttribute<FLinearColor> TintColor;
	TAttribute<FLinearColor> InactiveColor;
	TAttribute<FLinearColor> ThumbColor;
	TAttribute<FVector2D> SwitchSize;
	
	FOnToggleSwitchChangedNative OnToggled;

	// Animation state
	float CurrentThumbPosition; // 0.0f to 1.0f
	float TargetThumbPosition;

	// Brushes
	FSlateBrush BackgroundBrush;
	FSlateBrush ThumbBrush;
};

/**
 * UMG Wrapper for the Toggle Switch
 */
UCLASS()
class JUPITERPLUGIN_API UJupiterToggleSwitch : public UWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	bool bIsToggled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor TintColor = FLinearColor(0.0f, 0.45f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor InactiveColor = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor ThumbColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FVector2D SwitchSize = FVector2D(50.0f, 28.0f);

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnToggleSwitchChanged OnToggled;

	UFUNCTION(BlueprintCallable, Category = "Jupiter Toggle Switch")
	void SetIsToggled(bool bNewState);

	// UWidget interface
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<SJupiterToggleSwitch> MyToggleSwitch;

	void HandleOnToggled(bool bNewState);
};
