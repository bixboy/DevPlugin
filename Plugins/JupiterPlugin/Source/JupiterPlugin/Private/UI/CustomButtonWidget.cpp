#include "UI/CustomButtonWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Styling/SlateBrush.h"

namespace
{
    FSlateBrush CreateUpdatedBrush(const FSlateBrush& InBrush, const FLinearColor& TintColor)
    {
       FSlateBrush Result = InBrush;
       Result.TintColor = FSlateColor(TintColor);
       return Result;
    }
}

void UCustomButtonWidget::SetButtonColor(FLinearColor NewColor)
{
    FillColor = NewColor;
    bOverride_FillColor = true;
    bEnableFill = true;
    SetButtonSettings();
}

void UCustomButtonWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    if (!Button)
       return;

    // Bindings
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
    bUseBorderTexture = static_cast<bool>(bOverride_BorderTexture);
    bUseFill = bEnableFill;

    CachedFillColor = bOverride_FillColor ? FillColor : (ButtonBorder ? ButtonBorder->GetBrushColor() : FLinearColor::White);
    CachedFillHoverColor = bOverride_FillHoverColor ? FillHoverColor : CachedFillColor;
    
    CachedBorderColor = bOverride_BorderColor ? BorderColor : CachedFillColor;
    CachedBorderHoverColor = bOverride_BorderHoverColor ? BorderHoverColor : CachedBorderColor;

    CachedTextureAlpha = bOverride_Texture_Alpha ? TextureAlpha : 1.f;
    CachedTextureHoverAlpha = bOverride_Texture_Alpha ? TextureHoverAlpha : CachedTextureAlpha;
    
    CachedTextureScale = bOverride_Texture_Scale ? TextureScale : 1.f;
    CachedTextureHoverScale = bOverride_Texture_Scale ? TextureHoverScale : CachedTextureScale;
    
    CachedTextureShift.X = bOverride_Texture_Shift ? TextureShiftX : 0.f;
    CachedTextureShift.Y = bOverride_Texture_Shift ? TextureShiftY : 0.f;

    bUsingTextureSizeOverride = static_cast<bool>(bOverride_Texture_Size);
    CachedTextureSize = bOverride_Texture_Size ? TextureSize : FVector2D(32.f, 32.f);

    // --- 2. GESTION DE L'ICÔNE (ButtonImage) ---
    if (ButtonImage)
    {
       if (bUseTexture)
       {
          ButtonImage->SetBrushFromTexture(ButtonTexture);
          
          if (bUsingTextureSizeOverride)
          {
             ButtonImage->SetBrushSize(CachedTextureSize);
          }
           
          ButtonImage->SetVisibility(ESlateVisibility::HitTestInvisible);
       }
       else
       {
          ButtonImage->SetVisibility(ESlateVisibility::Collapsed);
       }
    }

    // --- 3. GESTION DU CADRE DÉCORATIF (BorderImage) ---
    if (BorderImage)
    {
        // On ne collapse plus si pas utilise, on laissera transparent dans UpdateButtonVisuals
        BorderImage->SetVisibility(ESlateVisibility::HitTestInvisible);
    }

    // --- 4. GESTION DU FOND (ButtonBorder) ---
    UpdateButtonVisuals(true);

    // --- 5. ALIGNEMENT TEXTE ---
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

    // --- A. COUCHE FOND (ButtonBorder) ---
    if (ButtonBorder)
    {
       if (bUseFill)
       {
           const FLinearColor TargetColor = bShouldUseHoverState ? CachedFillHoverColor : CachedFillColor;
           ButtonBorder->SetBrushColor(TargetColor);
       }
       else
       {
           ButtonBorder->SetBrushColor(FLinearColor::Transparent);
       }
    }

    // --- B. COUCHE CADRE (BorderImage) ---
    if (BorderImage)
    {
        if (bUseBorderTexture)
        {
            const FLinearColor TargetBorderColor = bShouldUseHoverState ? BorderHoverColor : BorderTextureColor;
            BorderImage->SetBrushColor(TargetBorderColor);
        }
        else
        {
            BorderImage->SetBrushColor(FLinearColor::Transparent);
        }
    }

    // --- C. COUCHE ICÔNE (ButtonImage) ---
    if (ButtonImage && bUseTexture)
    {
        const float TargetAlpha = bShouldUseHoverState ? CachedTextureHoverAlpha : CachedTextureAlpha;
        const float TargetScale = bShouldUseHoverState ? CachedTextureHoverScale : CachedTextureScale;

        FLinearColor CurrentColor = ButtonImage->GetColorAndOpacity();
        CurrentColor.A = TargetAlpha;
        ButtonImage->SetColorAndOpacity(CurrentColor);

        ButtonImage->SetRenderScale(FVector2D(TargetScale, TargetScale));
        ButtonImage->SetRenderTranslation(CachedTextureShift);
    }
}

#pragma endregion