// Copyright 2026

#pragma once

#include "CoreMinimal.h"
#include "Components/PrismButtonBase.h"
#include "BuilderCategoryButton.generated.h"

class UBorder;
class UTexture2D;

/**
 * Concrete button class used for all catalog buttons (categories, action buttons, tabs).
 * Implements silky-smooth color & indicator transitions for hover, press, and selection states.
 */
UCLASS()
class FIRSTPERSONBUILDER_API UBuilderCategoryButton : public UPrismButtonBase
{
	GENERATED_BODY()

public:
	UBuilderCategoryButton(const FObjectInitializer& ObjectInitializer);

	/** Base resting color for the button background */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
	FLinearColor NormalColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);

	/** Color when hovered (unselected) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
	FLinearColor HoverColor = FLinearColor(0.10f, 0.20f, 0.32f, 0.40f);

	/** Color when pressed (unselected) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
	FLinearColor PressedColor = FLinearColor(0.12f, 0.25f, 0.40f, 0.65f);

	/** Color when selected */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
	FLinearColor SelectedColor = FLinearColor(0.10f, 0.26f, 0.42f, 0.70f);

	/** Color when selected and hovered */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
	FLinearColor SelectedHoverColor = FLinearColor(0.14f, 0.34f, 0.54f, 0.85f);

	/** Color when selected and pressed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
	FLinearColor SelectedPressedColor = FLinearColor(0.18f, 0.42f, 0.65f, 0.95f);

	/** Custom stroke color for the border */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
	FLinearColor StrokeColor = FLinearColor::Transparent;

	/** Corner radius for the rounded box */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
	float CornerRadius = 0.0f;

	/** Left selection indicator line */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> IndicatorBorder;

	/** Outline border for stroked buttons */
	UPROPERTY(Transient)
	TObjectPtr<UBorder> OutlineBorder;

	/** If true, this button is a simple action button, not a sidebar category */
	UPROPERTY(EditAnywhere, Category = "Style")
	bool bIsActionButton = false;

	void SetAsActionButton();

	virtual void SetText(FText InText) override;
	virtual void SetIcon(UTexture2D* InIcon) override;
	virtual void SetContentPadding(FMargin InPadding) override;

	void SetFontSize(int32 InSize);
	void SetIconSize(FVector2D InSize);
	void RefreshStyle();

	/** Selection handling */
	void SetIsSelected(bool bInSelected);
	FORCEINLINE bool IsSelected() const { return bIsSelected; }

protected:
	virtual void NativePreConstruct() override;
	virtual void BuildDefaultLayout() override;
	virtual void OnStateChanged(EPrismWidgetState InNewState) override;
	virtual bool TickTransitions(float DeltaTime) override;

private:
	void UpdateTargetColors();

	bool bIsSelected = false;

	FLinearColor CurrentBgColor = FLinearColor::Transparent;
	FLinearColor TargetBgColor = FLinearColor::Transparent;
	FLinearColor CurrentIndicatorColor = FLinearColor::Transparent;
	FLinearColor TargetIndicatorColor = FLinearColor::Transparent;
};
