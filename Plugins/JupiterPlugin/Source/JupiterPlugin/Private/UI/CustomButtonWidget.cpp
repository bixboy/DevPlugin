#include "UI/CustomButtonWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Styling/SlateBrush.h"


void UCustomButtonWidget::SetButtonColor(FLinearColor NewColor)
{
    FillColor = NewColor;
    bOverride_FillColor = true;
    bEnableFill = true;
    SetButtonSettings();
}

UCustomButtonWidget::UCustomButtonWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    bHasScriptImplementedTick = false;
}

void UCustomButtonWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    if (!Button)
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

FReply UCustomButtonWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        return FReply::Handled();
    }
    
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UCustomButtonWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnButtonRightClicked.Broadcast(this, ButtonIndex);
		return FReply::Handled();
	}
	
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UCustomButtonWidget::SetButtonText(const FText& InText)
{
    ButtonText = InText;
    bOverride_ButtonText = !InText.IsEmpty(); 
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
    if (!ButtonTextBlock)
       return;
    
    const bool bShouldShow = bOverride_ButtonText && !InText.IsEmpty();
    
    if (bShouldShow)
    {
       if (TextScaleBox)
       {
           TextScaleBox->SetVisibility(ESlateVisibility::HitTestInvisible);
       }
       
       ButtonTextBlock->SetVisibility(ESlateVisibility::HitTestInvisible);
       ButtonTextBlock->SetText(InText);

       FSlateFontInfo NewFontInfo = ButtonTextBlock->GetFont();
       NewFontInfo.Size = TextScale;
       ButtonTextBlock->SetFont(NewFontInfo);
        
       if (UScaleBoxSlot* ScaleBoxSlot = Cast<UScaleBoxSlot>(ButtonTextBlock->Slot))
       {
          ScaleBoxSlot->SetHorizontalAlignment(TextAlignmentHorizontal);
          ScaleBoxSlot->SetVerticalAlignment(TextAlignmentVertical);
       }
    }
    else
    {
       ButtonTextBlock->SetVisibility(ESlateVisibility::Collapsed);
       if (TextScaleBox)
       {
           TextScaleBox->SetVisibility(ESlateVisibility::Collapsed);
       }
    }
}

void UCustomButtonWidget::SetButtonSettings()
{
    // --- 1. CALCULS DES ÉTATS ---
    bUseTexture = bEnableTexture && ButtonTexture != nullptr;
    bUseBorderTexture = bEnableBorder;
    bUseFill = bEnableFill;

    CachedFillColor = bOverride_FillColor ? FillColor : (ButtonBorder ? ButtonBorder->GetBrushColor() : FLinearColor::White);
    CachedFillHoverColor = bOverride_FillHoverColor ? FillHoverColor : CachedFillColor;
    
    CachedBorderColor = BorderColor;
    CachedBorderHoverColor = BorderHoverColor;

    CachedTextureAlpha = bOverride_Texture_Alpha ? TextureAlpha : 1.f;
    CachedTextureHoverAlpha = bOverride_Texture_Alpha ? TextureHoverAlpha : CachedTextureAlpha;
    
    CachedTextureScale = bOverride_Texture_Scale ? TextureScale : 1.f;
    CachedTextureHoverScale = bOverride_Texture_Scale ? TextureHoverScale : CachedTextureScale;
    
    CachedTextureShift.X = bOverride_Texture_Shift ? TextureShiftX : 0.f;
    CachedTextureShift.Y = bOverride_Texture_Shift ? TextureShiftY : 0.f;

    bUsingTextureSizeOverride = static_cast<bool>(bOverride_Texture_Size);
    CachedTextureSize = bOverride_Texture_Size ? TextureSize : FVector2D(32.f, 32.f);

    if (CurrentTextureScale == 0.0f) 
    {
         CurrentTextureScale = CachedTextureScale;
         CurrentTextureAlpha = CachedTextureAlpha;
         CurrentFillColor = CachedFillColor;
         CurrentBorderColor = CachedBorderColor;
    }

    // --- 2. (ButtonImage) ---
    if (ButtonImage)
    {
       if (bUseTexture)
       {
          ButtonImage->SetBrushFromTexture(ButtonTexture);
          
          if (bUsingTextureSizeOverride)
          {
              ButtonImage->SetDesiredSizeOverride(CachedTextureSize);
          }
           
          ButtonImage->SetVisibility(ESlateVisibility::HitTestInvisible);
       }
       else
       {
          ButtonImage->SetVisibility(ESlateVisibility::Collapsed);
       }
    }

    // --- 3. (BorderImage) ---
    if (BorderImage)
    {
        BorderImage->SetVisibility(ESlateVisibility::HitTestInvisible);
    }

    // --- 4. (ButtonBorder) ---
    UpdateButtonVisuals(true);

    // --- 5. Text ---
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

    // Target Values
    const FLinearColor TargetFillColor = (bUseFill) ? (bShouldUseHoverState ? CachedFillHoverColor : CachedFillColor) : FLinearColor::Transparent;
    const FLinearColor TargetBorderColor = (bUseBorderTexture && BorderImage) ? (bShouldUseHoverState ? CachedBorderHoverColor : CachedBorderColor) : FLinearColor::Transparent;
    
    // Texture Targets
    const float TargetAlpha = bShouldUseHoverState ? CachedTextureHoverAlpha : CachedTextureAlpha;
    const float TargetScale = bShouldUseHoverState ? CachedTextureHoverScale : CachedTextureScale;
    
    if (!bEnableTransition || bForceStateUpdate)
    {
        CurrentFillColor = TargetFillColor;
        CurrentBorderColor = TargetBorderColor;
        CurrentTextureAlpha = TargetAlpha;
        CurrentTextureScale = TargetScale;
    }
        
    if (!bEnableTransition || bForceStateUpdate)
    {
         if (ButtonBorder) 
            ButtonBorder->SetBrushColor(CurrentFillColor);

         if (BorderImage) 
            BorderImage->SetBrushColor(CurrentBorderColor);

         if (ButtonImage && bUseTexture)
         {
             FLinearColor CurrentColor = ButtonImage->GetColorAndOpacity();
             CurrentColor.A = CurrentTextureAlpha;
             ButtonImage->SetColorAndOpacity(CurrentColor);
             ButtonImage->SetRenderScale(FVector2D(CurrentTextureScale, CurrentTextureScale));
             ButtonImage->SetRenderTranslation(CachedTextureShift);
         }
    }
    
    if (bEnableTransition && !bForceStateUpdate)
    {
         bIsAnimating = true;
    }
}

void UCustomButtonWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!bEnableTransition || !bIsAnimating)
        return;

    const bool bShouldUseHoverState = bIsHovered || bIsSelected;

    // --- Calculate Targets ---
    const FLinearColor TargetFillColor = (bUseFill) ? (bShouldUseHoverState ? CachedFillHoverColor : CachedFillColor) : FLinearColor::Transparent;
    
    // Border
    FLinearColor TargetBorderColor = FLinearColor::Transparent;
    if (BorderImage && bUseBorderTexture)
    {
         TargetBorderColor = bShouldUseHoverState ? CachedBorderHoverColor : CachedBorderColor;
    }

    // Texture
    const float TargetAlpha = bShouldUseHoverState ? CachedTextureHoverAlpha : CachedTextureAlpha;
    const float TargetScale = bShouldUseHoverState ? CachedTextureHoverScale : CachedTextureScale;

    // --- INTERPOLATE ---
    CurrentFillColor = FMath::CInterpTo(CurrentFillColor, TargetFillColor, InDeltaTime, TransitionSpeed);
    CurrentBorderColor = FMath::CInterpTo(CurrentBorderColor, TargetBorderColor, InDeltaTime, TransitionSpeed);
    CurrentTextureAlpha = FMath::FInterpTo(CurrentTextureAlpha, TargetAlpha, InDeltaTime, TransitionSpeed);
    CurrentTextureScale = FMath::FInterpTo(CurrentTextureScale, TargetScale, InDeltaTime, TransitionSpeed);

    // --- APPLY ---
    if (ButtonBorder) 
    {
        ButtonBorder->SetBrushColor(CurrentFillColor);
    }
    
    if (BorderImage)
    {
        BorderImage->SetBrushColor(CurrentBorderColor);
    }

    if (ButtonImage && bUseTexture)
    {
        FLinearColor CurrentColor = ButtonImage->GetColorAndOpacity();
        CurrentColor.A = CurrentTextureAlpha;
        ButtonImage->SetColorAndOpacity(CurrentColor);
        ButtonImage->SetRenderTranslation(CachedTextureShift);
    }

    // --- CHECK COMPLETION ---
    bool bComplete = true;
    const float Tolerance = 0.001f;
    
    if (!CurrentFillColor.Equals(TargetFillColor, Tolerance)) bComplete = false;
    else if (!CurrentBorderColor.Equals(TargetBorderColor, Tolerance)) bComplete = false;
    else if (!FMath::IsNearlyEqual(CurrentTextureAlpha, TargetAlpha, Tolerance)) bComplete = false;
    else if (!FMath::IsNearlyEqual(CurrentTextureScale, TargetScale, Tolerance)) bComplete = false;

    if (bComplete)
    {
        bIsAnimating = false;
    }
}

#pragma endregion