#include "Widget/CustomButtonWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/TextBlock.h"
#include "Styling/SlateBrush.h"

namespace
{
	FSlateBrush CreateUpdatedBrush(const FSlateBrush& InBrush, const FLinearColor& TintColor, const FLinearColor& OutlineColor)
	{
		FSlateBrush Result = InBrush;
		Result.TintColor = FSlateColor(TintColor);
		Result.OutlineSettings.Color = OutlineColor;
		return Result;
	}
}

void UCustomButtonWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (!Button || !ButtonTextBlock || !ButtonBorder)
		return;

	if (!Button->OnClicked.IsAlreadyBound(this, &UCustomButtonWidget::OnCustomUIButtonClickedEvent))
		Button->OnClicked.AddDynamic(this, &UCustomButtonWidget::OnCustomUIButtonClickedEvent);

	if (!Button->OnHovered.IsAlreadyBound(this, &UCustomButtonWidget::OnCustomUIButtonHoveredEvent))
		Button->OnHovered.AddDynamic(this, &UCustomButtonWidget::OnCustomUIButtonHoveredEvent);

	if (!Button->OnUnhovered.IsAlreadyBound(this, &UCustomButtonWidget::OnCustomUIButtonUnHoveredEvent))
		Button->OnUnhovered.AddDynamic(this, &UCustomButtonWidget::OnCustomUIButtonUnHoveredEvent);

	UpdateButtonText(ButtonText);
	SetButtonSettings();
}

void UCustomButtonWidget::SetButtonText(const FText& InText)
{
	if (!ButtonTextBlock)
		return;

	bOverride_ButtonText = InText.IsEmpty();
	ButtonText = InText;
	UpdateButtonText(ButtonText);
}

void UCustomButtonWidget::ToggleButtonIsSelected(bool bNewValue)
{
	if (bIsSelected == bNewValue)
		return;

	bIsSelected = bNewValue;
	UpdateButtonVisuals(true);
}

void UCustomButtonWidget::SetButtonTexture(UTexture2D* NewTexture)
{
	ButtonTexture = NewTexture;
	SetButtonSettings();
}

//-------------------------- Events & Delegates -----------------------------
#pragma region Events & Delegates

void UCustomButtonWidget::OnCustomUIButtonClickedEvent()
{
	OnButtonClicked.Broadcast(this, ButtonIndex);
}

void UCustomButtonWidget::OnCustomUIButtonHoveredEvent()
{
	bIsHovered = true;
	UpdateButtonVisuals(false);
	OnButtonHovered.Broadcast(this, ButtonIndex);
}

void UCustomButtonWidget::OnCustomUIButtonUnHoveredEvent()
{
	bIsHovered = false;
	UpdateButtonVisuals(false);
	OnButtonUnHovered.Broadcast(this, ButtonIndex);
}

#pragma endregion

//-------------------------- Settings -----------------------------
#pragma region Settings

void UCustomButtonWidget::UpdateButtonText(const FText& InText)
{
	if (ButtonTextBlock)
	{
		ButtonTextBlock->SetText(InText);

		FSlateFontInfo NewFontInfo = ButtonTextBlock->GetFont();
		NewFontInfo.Size = TextScale;
		ButtonTextBlock->SetFont(NewFontInfo);
	}
}

void UCustomButtonWidget::SetButtonSettings()
{
	if (!ButtonBorder)
		return;

	bUseTexture = bEnableTexture && ButtonTexture != nullptr;
	bUseFill = bEnableFill && (!bUseTexture || bAllowFillWhenTextureIsSet);

	CachedFillColor = bOverride_FillColor ? FillColor : ButtonBorder->GetBrushColor();
	CachedFillHoverColor = bOverride_FillHoverColor ? FillHoverColor : CachedFillColor;
	
	CachedBorderColor = bOverride_BorderColor ? BorderColor : CachedFillColor;
	CachedBorderHoverColor = bOverride_BorderHoverColor ? BorderHoverColor : CachedBorderColor;

	CachedTextureAlpha = bOverride_Texture_Alpha ? TextureAlpha : 1.f;
	CachedTextureHoverAlpha = bOverride_Texture_Alpha ? TextureHoverAlpha : CachedTextureAlpha;
	
	CachedTextureScale = bOverride_Texture_Scale ? TextureScale : 1.f;
	CachedTextureHoverScale = bOverride_Texture_Scale ? TextureHoverScale : CachedTextureScale;
	
	CachedTextureShift.X = bOverride_Texture_Shift ? TextureShiftX : 0.f;
	CachedTextureShift.Y = bOverride_Texture_Shift ? TextureShiftY : 0.f;

	if (ButtonImage)
	{
		if (bUseTexture && ButtonTexture)
		{
			FSlateBrush Brush;
			Brush.SetResourceObject(ButtonTexture);
			Brush.ImageSize = FVector2D(ButtonTexture->GetSizeX(), ButtonTexture->GetSizeY());
			Brush.DrawAs = ESlateBrushDrawType::Image;
			
			ButtonImage->SetBrushFromTexture(ButtonTexture);
			ButtonImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			ButtonImage->SetBrush(FSlateBrush());
			ButtonImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else if (bUseTexture && ButtonTexture)
	{
		ButtonBorder->SetBrushFromTexture(ButtonTexture);
	}
	else
	{
		FSlateBrush BrushCopy = ButtonBorder->Background;
		BrushCopy.SetResourceObject(nullptr);
		ButtonBorder->SetBrush(BrushCopy);
	}

	UpdateButtonVisuals(true);

	if (ButtonTextBlock)
	{
		if (UScaleBoxSlot* ScaleBoxSlot = Cast<UScaleBoxSlot>(ButtonTextBlock->Slot))
		{
			ScaleBoxSlot->SetHorizontalAlignment(TextAlignmentHorizontal);
			ScaleBoxSlot->SetVerticalAlignment(TextAlignmentVertical);
		}
	}
}

void UCustomButtonWidget::UpdateButtonVisuals(const bool bForceStateUpdate)
{
	const bool bShouldUseHoverState = bIsHovered || bIsSelected;
	const bool bDisplayTextureOnBorder = (!ButtonImage || !ButtonImage->IsVisible()) && bUseTexture && ButtonTexture;

	if (ButtonBorder)
	{
		FLinearColor FillColorToApply = FLinearColor::Transparent;
		if (bDisplayTextureOnBorder)
		{
			const float TargetAlpha = bShouldUseHoverState ? CachedTextureHoverAlpha : CachedTextureAlpha;
			FillColorToApply = FLinearColor(1.f, 1.f, 1.f, TargetAlpha);
		}
		else if (bUseFill)
		{
			FillColorToApply = bShouldUseHoverState ? CachedFillHoverColor : CachedFillColor;
		}

		const FLinearColor BorderColorToApply = bShouldUseHoverState ? CachedBorderHoverColor : CachedBorderColor;
		
		FSlateBrush BrushCopy = ButtonBorder->Background;
		const FSlateBrush UpdatedBrush = CreateUpdatedBrush(BrushCopy, FillColorToApply, BorderColorToApply);
		ButtonBorder->SetBrush(UpdatedBrush);

		if (!ButtonImage || !bUseTexture)
		{
			const float TargetScale = bShouldUseHoverState ? CachedTextureHoverScale : CachedTextureScale;
			ButtonBorder->SetRenderScale(FVector2D(TargetScale, TargetScale));
			ButtonBorder->SetRenderTranslation(CachedTextureShift);
		}
	}

	if (ButtonImage)
	{
		if (bUseTexture && ButtonTexture)
		{
			const float TargetAlpha = bShouldUseHoverState ? CachedTextureHoverAlpha : CachedTextureAlpha;
			const float TargetScale = bShouldUseHoverState ? CachedTextureHoverScale : CachedTextureScale;

			FLinearColor CurrentColor = ButtonImage->GetColorAndOpacity();
			CurrentColor.A = TargetAlpha;
			ButtonImage->SetColorAndOpacity(CurrentColor);

			ButtonImage->SetRenderScale(FVector2D(TargetScale, TargetScale));
			ButtonImage->SetRenderTranslation(CachedTextureShift);
		}
		else if (bForceStateUpdate)
		{
			ButtonImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

#pragma endregion
