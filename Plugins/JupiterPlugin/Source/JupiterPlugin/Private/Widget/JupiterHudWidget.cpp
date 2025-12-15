#include "Widget/JupiterHudWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/UnitSelectionComponent.h"
#include "Components/WidgetSwitcher.h"
#include "Widget/Formations/FormationSelectorWidget.h"
#include "Widget/Behaviors/SelectBehaviorWidget.h"
#include "Widget/UnitsSelection/UnitsSelectionWidget.h"
#include "Widget/Patrol/PatrolListWidget.h"
#include "Components/UnitPatrolComponent.h"


void UJupiterHudWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	if(APawn* Owner = GetOwningPlayerPawn())
		InitializedJupiterHud(Owner);

	if (Btn_Switcher)
		Btn_Switcher->OnButtonClicked.AddDynamic(this, &UJupiterHudWidget::SwitchUnitAction);
}

void UJupiterHudWidget::NativeDestruct()
{
	Super::NativeDestruct();

	if (Btn_Switcher)
		Btn_Switcher->OnButtonClicked.RemoveDynamic(this, &UJupiterHudWidget::SwitchUnitAction);
}

void UJupiterHudWidget::SwitchUnitAction(UCustomButtonWidget* Button, int Index)
{
	if (SelectorSwitcher->GetActiveWidgetIndex() == 1)
	{
		SelectorSwitcher->SetActiveWidgetIndex(0);
		Switch("behaviors");
	}
	else
	{
		SelectorSwitcher->SetActiveWidgetIndex(1);
		Switch("formations");
	}
}

void UJupiterHudWidget::Switch(FString Type) const
{
	if (Type == "formations")
	{
		Btn_Switcher->SetButtonText(FText::FromString("Formations"));
		ActionTitle->SetText(FText::FromString("Formations"));
		return;
	}

	if (Type == "behaviors")
	{
		Btn_Switcher->SetButtonText(FText::FromString("Behaviors"));
		ActionTitle->SetText(FText::FromString("Behaviors"));
	}
}


void UJupiterHudWidget::SetFormationSelectionWidget(const bool bEnabled) const
{
	if (FormationSelector)
	{
		Switch("formations");
		FormationSelector->SetVisibility(bEnabled ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Btn_Switcher->SetIsEnabled(bEnabled);
		SelectorSwitcher->SetActiveWidgetIndex(1);

		if (!bEnabled)
		{
			Switch("Behaviors");
			SelectorSwitcher->SetActiveWidgetIndex(0);
		}
	}
}

void UJupiterHudWidget::SetBehaviorSelectionWidget(const bool bEnabled) const
{
	if (BehaviorSelector)
	{
		Switch("Behaviors");
		BehaviorSelector->SetVisibility(bEnabled ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		UnitActionBorder->SetVisibility(bEnabled ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UJupiterHudWidget::SetUnitsSelectionWidget(bool bEnabled) const
{
    if (UnitsSelector)
		UnitsSelector->SetVisibility(bEnabled ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UJupiterHudWidget::InitializedJupiterHud(APawn* PawnLinked)
{
	if(!PawnLinked) return;
        SelectionComponent = PawnLinked->FindComponentByClass<UUnitSelectionComponent>();
	
	if(!SelectionComponent)
		return;

	SetFormationSelectionWidget(false);
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
		if (SelectionComponent->GetSelectedActors().IsEmpty())
		{
			SetBehaviorSelectionWidget(false);
			return;
		}

		SetBehaviorSelectionWidget(true);
		SetFormationSelectionWidget(SelectionComponent->HasGroupSelection());

		// Update Patrol Editor in UnitsSelector
		if (SelectionComponent->GetSelectedActors().Num() == 1)
		{
			AActor* SelectedActor = SelectionComponent->GetSelectedActors()[0];
			if (SelectedActor)
			{
				UUnitPatrolComponent* PatrolComp = SelectedActor->FindComponentByClass<UUnitPatrolComponent>();
				if (PatrolComp && UnitsSelector)
				{
					UnitsSelector->SetPatrolComponent(PatrolComp);
				}
			}
		}
		else
		{
		    // Clear if multiple or none
		    if (UnitsSelector)
		    {
		        UnitsSelector->SetPatrolComponent(nullptr);
		    }
		}

		int Selected = SelectionComponent->GetSelectedActors().Num();
		FString Text = FString::FromInt(Selected);
		UnitsSelected->SetText(FText::FromString(Text));
	}
}
