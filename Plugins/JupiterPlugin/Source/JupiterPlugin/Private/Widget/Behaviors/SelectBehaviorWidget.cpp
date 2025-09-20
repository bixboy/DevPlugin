#include "Widget/Behaviors/SelectBehaviorWidget.h"
#include "Player/PlayerControllerRts.h"
#include "Components/SlectionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Units/SoldierRts.h"
#include "Widget/Behaviors/BehaviorButtonWidget.h"

void USelectBehaviorWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	verify((SelectionComponent = UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetPawn()->GetComponentByClass<USelectionComponent>()) != nullptr);

	if (SelectionComponent)
		SelectionComponent->OnSelectedUpdate.AddDynamic(this, &USelectBehaviorWidget::OnNewUnitSelected);

	if (NeutralButton && PassiveButton && AggressiveButton)
	{
		OnBehaviorButtonClicked(NeutralButton->Button, 0);
		NeutralButton->Button->OnButtonClicked.AddDynamic(this, &USelectBehaviorWidget::OnBehaviorButtonClicked);
		PassiveButton->Button->OnButtonClicked.AddDynamic(this, &USelectBehaviorWidget::OnBehaviorButtonClicked);
		AggressiveButton->Button->OnButtonClicked.AddDynamic(this, &USelectBehaviorWidget::OnBehaviorButtonClicked);
	}
}

void USelectBehaviorWidget::OnBehaviorButtonClicked(UCustomButtonWidget* Button, int Index)
{
	if (SelectionComponent)
	{
		SelectionComponent->UpdateBehavior(static_cast<ECombatBehavior>(Index));

		UpdateSelectedButton(NeutralButton->Button, false);
		UpdateSelectedButton(PassiveButton->Button, false);
		UpdateSelectedButton(AggressiveButton->Button, false);

		UpdateSelectedButton(Button, true);
	}
}

void USelectBehaviorWidget::UpdateSelectedButton(UCustomButtonWidget* Button, bool IsSelected)
{
	Button->ToggleButtonIsSelected(IsSelected);
}

void USelectBehaviorWidget::OnNewUnitSelected()
{
	UpdateSelectedButton(NeutralButton->Button, false);
	UpdateSelectedButton(PassiveButton->Button, false);
	UpdateSelectedButton(AggressiveButton->Button, false);
	
	if (SelectionComponent->GetSelectedActors().Num() == 1)
	{
		if (ASoldierRts* Unit = Cast<ASoldierRts>(SelectionComponent->GetSelectedActors()[0]))
		{
			
			ECombatBehavior Beavior = Unit->GetCombatBehavior();
			switch (Beavior)
			{
			case ECombatBehavior::Neutral:
				UpdateSelectedButton(NeutralButton->Button, true);
				break;

			case ECombatBehavior::Passive:
				UpdateSelectedButton(PassiveButton->Button, true);
				break;
				
			case ECombatBehavior::Aggressive:
				UpdateSelectedButton(AggressiveButton->Button, true);
				break;
				
			}
		}
	}
}

