#include "Widget/Formations/FormationButtonWidget.h"
#include "Components/TextBlock.h"

void UFormationButtonWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (!Button) return;

	Button->ButtonIndex = static_cast<int8>(Formation.GetValue());
	Button->SetButtonText(UEnum::GetDisplayValueAsText(Formation));
}
