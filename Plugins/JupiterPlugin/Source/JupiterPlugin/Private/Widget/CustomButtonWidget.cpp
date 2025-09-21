#include "Widget/CustomButtonWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Styling/SlateBrush.h"

void UCustomButtonWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (!Button || !ButtonTextBlock || !ButtonBorder)
		return;
	
	Button->OnClicked.AddDynamic(this, &UCustomButtonWidget::OnCustomUIButtonClickedEvent);
	Button->OnHovered.AddDynamic(this, &UCustomButtonWidget::OnCustomUIButtonHoveredEvent);
	Button->OnUnhovered.AddDynamic(this, &UCustomButtonWidget::OnCustomUIButtonUnHoveredEvent);

	ApplyMaterial();
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
	if (ButtonBorder)
	{
		if (UMaterialInstanceDynamic* MaterialInstance = ButtonBorder->GetDynamicMaterial())
		{
			bIsSelected = bNewValue;
			
			if (bIsSelected )
			{
				MaterialInstance->SetScalarParameterValue("Hover_Animate", 10.f);
				MaterialInstance->SetScalarParameterValue("Size", 0.85f);
			}
			else
			{
				MaterialInstance->SetScalarParameterValue("Hover_Animate", 0.f);
				MaterialInstance->SetScalarParameterValue("Size", 0.75f);
			}
		}
	}
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
	if(ButtonBorder)
	{
		if(UMaterialInstanceDynamic* MaterialInstance = ButtonBorder->GetDynamicMaterial())
			MaterialInstance->SetScalarParameterValue("Hover_Animate", 1.f);
	}
	OnButtonHovered.Broadcast(this, ButtonIndex);
}

void UCustomButtonWidget::OnCustomUIButtonUnHoveredEvent()
{
	if(ButtonBorder)
	{
		if(UMaterialInstanceDynamic* MaterialInstance = ButtonBorder->GetDynamicMaterial())
		{
			if (!bIsSelected)
				MaterialInstance->SetScalarParameterValue("Hover_Animate", 0.f);
			else
				MaterialInstance->SetScalarParameterValue("Hover_Animate", 2.f);
		}
	}
	OnButtonUnHovered.Broadcast(this, ButtonIndex);
}

#pragma endregion

//-------------------------- Settings -----------------------------
#pragma region Settings

void UCustomButtonWidget::ApplyMaterial() const
{
	if (ButtonBorder && Material)
		ButtonBorder->SetBrushFromMaterial(Material);
}

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

void UCustomButtonWidget::SetButtonSettings() const
{
    if (ButtonBorder)
    {
        if (UMaterialInstanceDynamic* MaterialInstance = ButtonBorder->GetDynamicMaterial())
        {
            const bool bHasTexture = ButtonTexture != nullptr && bEnableTexture;
            const bool bShouldUseFill = bEnableFill && (!bHasTexture || bAllowFillWhenTextureIsSet);

            static const FName TextureParamName(TEXT("Texture"));
            static const FName UseTextureParamName(TEXT("UseTexture"));
            static const FName UseFillParamName(TEXT("UseFill"));
            static const FName FillColorParamName(TEXT("FillColor"));
            static const FName FillHoverColorParamName(TEXT("FillHoverColor"));
            static const FName BorderColorParamName(TEXT("BorderColor"));
            static const FName BorderHoverColorParamName(TEXT("BorderHoverColor"));

            MaterialInstance->SetScalarParameterValue(UseTextureParamName, bHasTexture ? 1.f : 0.f);
            MaterialInstance->SetScalarParameterValue(UseFillParamName, bShouldUseFill ? 1.f : 0.f);

            if (bHasTexture && ButtonTexture)
            {
                MaterialInstance->SetTextureParameterValue(TextureParamName, ButtonTexture);
            }
            else
            {
                MaterialInstance->SetTextureParameterValue(TextureParamName, nullptr);
            }

            const FLinearColor BrushColor = ButtonBorder->GetBrushColor();

            const FLinearColor EffectiveFillColor = bOverride_FillColor ? FillColor : BrushColor;
            const FLinearColor EffectiveFillHoverColor = bOverride_FillHoverColor ? FillHoverColor : EffectiveFillColor;
            const FLinearColor EffectiveBorderColor = bOverride_BorderColor ? BorderColor : EffectiveFillColor;
            const FLinearColor EffectiveBorderHoverColor = bOverride_BorderHoverColor ? BorderHoverColor : EffectiveBorderColor;

            MaterialInstance->SetVectorParameterValue(FillColorParamName, EffectiveFillColor);
            MaterialInstance->SetVectorParameterValue(FillHoverColorParamName, EffectiveFillHoverColor);
            MaterialInstance->SetVectorParameterValue(BorderColorParamName, EffectiveBorderColor);
            MaterialInstance->SetVectorParameterValue(BorderHoverColorParamName, EffectiveBorderHoverColor);

            if (bOverride_Texture_Alpha > 0)
            {
            	MaterialInstance->SetScalarParameterValue("TextAlpha", TextureAlpha);
            	MaterialInstance->SetScalarParameterValue("TextAlphaHover", TextureHoverAlpha);
            }

			if (bOverride_Texture_Scale > 0)
			{
				MaterialInstance->SetScalarParameterValue("TextScale", TextureScale);
				MaterialInstance->SetScalarParameterValue("TextScaleHover", TextureHoverScale);
			}

			if (bOverride_Texture_Shift > 0)
			{
				MaterialInstance->SetScalarParameterValue("Shift_X", TextureShiftX);
				MaterialInstance->SetScalarParameterValue("Shift_Y", TextureShiftY);
			}
        }
    }

    if (UScaleBoxSlot* ScaleBoxSlot = Cast<UScaleBoxSlot>(ButtonTextBlock->Slot))
    {
		ScaleBoxSlot->SetHorizontalAlignment(TextAlignmentHorizontal);
		ScaleBoxSlot->SetVerticalAlignment(TextAlignmentVertical);
	}
}

#pragma endregion