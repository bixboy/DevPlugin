#include "UI/Editor/Widgets/SidebarButtonWidget.h"


void USidebarButtonWidget::UpdateButtonVisuals(const bool bForceStateUpdate)
{
	Super::UpdateButtonVisuals(bForceStateUpdate);

	FLinearColor CurrentContentColor = NormalContentColor;
	if (bIsSelected)
	{
		CurrentContentColor = SelectedContentColor;
	}
	else if (bIsHovered)
	{
		CurrentContentColor = HoverContentColor;
	}
	
	// Icon
	if (ButtonImage && bUseTexture)
	{
		FLinearColor ImageColor = CurrentContentColor;
		
		float CurrentAlpha = ButtonImage->GetColorAndOpacity().A;
		ImageColor.A = CurrentAlpha;
		
		ButtonImage->SetColorAndOpacity(ImageColor);
	}

	// Text
	if (ButtonTextBlock)
	{
		ButtonTextBlock->SetColorAndOpacity(FSlateColor(CurrentContentColor));
	}

	// Selection Indicator
	if (SelectionIndicator)
	{
		if (bIsSelected)
		{
			SelectionIndicator->SetVisibility(ESlateVisibility::HitTestInvisible);
			SelectionIndicator->SetColorAndOpacity(SelectionIndicatorColor);
		}
		else
		{
			SelectionIndicator->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}
