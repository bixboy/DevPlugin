#include "Widgets/PromptWidget.h"
#include "Components/TextBlock.h"


void UPromptWidget::SetPromptText(const FText& InText)
{
	PromptTextBlock->SetText(InText);
}
