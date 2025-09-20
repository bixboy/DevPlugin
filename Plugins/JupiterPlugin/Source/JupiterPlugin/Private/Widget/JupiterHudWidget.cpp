#include "Widget/JupiterHudWidget.h"

#include "Player/PlayerControllerRts.h"
#include "Components/Button.h"
#include "Components/UnitSelectionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Widget/Formations/FormationSelectorWidget.h"
#include "Widget/Behaviors/SelectBehaviorWidget.h"

void UJupiterHudWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	APawn* Owner = GetOwningPlayerPawn();
	if(Owner)
	{
		InitializedJupiterHud(Owner);
	}
	
}

void UJupiterHudWidget::SetFormationSelectionWidget(const bool bEnabled) const
{
	if (FormationSelector)
	{
		FormationSelector->SetVisibility(bEnabled ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Btn_SwitchFormation->SetVisibility(bEnabled ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UJupiterHudWidget::SetBehaviorSelectionWidget(const bool bEnabled) const
{
	if (BehaviorSelector)
	{
		BehaviorSelector->SetVisibility(bEnabled ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Btn_SwitchBehavior->SetVisibility(bEnabled ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UJupiterHudWidget::SetUnitsSelectionWidget(bool bEnabled) const
{
        if (UnitsSelector)
        {
                UnitsSelector->SetVisibility(bEnabled ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        }
}

void UJupiterHudWidget::InitializedJupiterHud(APawn* PawnLinked)
{
	if(!PawnLinked) return;
	
        SelectionComponent = PawnLinked->FindComponentByClass<UUnitSelectionComponent>();
	if(!SelectionComponent) return;

	SetFormationSelectionWidget(false);
	SetBehaviorSelectionWidget(false);
	SetUnitsSelectionWidget(true);


        if (SelectionComponent)
        {
                SelectionComponent->OnSelectedUpdate.AddUniqueDynamic(this, &UJupiterHudWidget::OnSelectionUpdated);
                OnSelectionUpdated();
        }
}

void UJupiterHudWidget::OnSelectionUpdated()
{

	if(SelectionComponent)
	{
		
		SetFormationSelectionWidget(SelectionComponent->HasGroupSelection());
		if (!SelectionComponent->GetSelectedActors().IsEmpty())
		{
			SetBehaviorSelectionWidget(true);
		}
		else
		{
			SetBehaviorSelectionWidget(false);
		}
	}
}
