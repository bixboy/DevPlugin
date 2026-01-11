#include "UI/Behaviors/BehaviorButtonWidget.h"
#include "Blueprint/UserWidget.h"


void UBehaviorButtonWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (!Button) return;
	
	const UEnum* EnumPtr = StaticEnum<ECombatBehavior>();
	Button->ButtonIndex = static_cast<int32>(CombatBehavior);
	Button->SetButtonText(EnumPtr->GetDisplayNameTextByValue(Button->ButtonIndex));
}
