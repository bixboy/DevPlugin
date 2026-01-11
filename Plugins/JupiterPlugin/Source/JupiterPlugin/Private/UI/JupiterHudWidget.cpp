#include "UI/JupiterHudWidget.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Unit/UnitSelectionComponent.h"
#include "Components/Unit/UnitSpawnComponent.h"
#include "Components/Patrol/UnitPatrolComponent.h"
#include "Components/WidgetSwitcher.h"
#include "UI/CustomButtonWidget.h"
#include "UI/Behaviors/SelectBehaviorWidget.h"
#include "UI/Formations/FormationSelectorWidget.h"
#include "UI/UnitsSelection/UnitsSelectionWidget.h"


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
		SwitchTab(EControlPanelTab::Behaviors);
	}
	else
	{
		SelectorSwitcher->SetActiveWidgetIndex(1);
		SwitchTab(EControlPanelTab::Formations);
	}
}

void UJupiterHudWidget::SwitchTab(EControlPanelTab NewTab) const
{
	if (NewTab == EControlPanelTab::Formations)
	{
		Btn_Switcher->SetButtonText(FText::FromString("Formations"));
		ActionTitle->SetText(FText::FromString("Formations"));
		return;
	}

	if (NewTab == EControlPanelTab::Behaviors)
	{
		Btn_Switcher->SetButtonText(FText::FromString("Behaviors"));
		ActionTitle->SetText(FText::FromString("Behaviors"));
	}
}


void UJupiterHudWidget::SetFormationSelectionWidget(const bool bEnabled) const
{
	if (FormationSelector)
	{
		SwitchTab(EControlPanelTab::Formations);
		FormationSelector->SetVisibility(bEnabled ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Btn_Switcher->SetIsEnabled(bEnabled);
		SelectorSwitcher->SetActiveWidgetIndex(1);

		if (!bEnabled)
		{
			SwitchTab(EControlPanelTab::Behaviors);
			SelectorSwitcher->SetActiveWidgetIndex(0);
		}
	}
}

void UJupiterHudWidget::SetBehaviorSelectionWidget(const bool bEnabled) const
{
	if (BehaviorSelector)
	{
		SwitchTab(EControlPanelTab::Behaviors);
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
	if(!PawnLinked)
		return;
	
	if (SelectionComponent.IsValid())
	{
		SelectionComponent->OnSelectedUpdate.RemoveDynamic(this, &UJupiterHudWidget::OnSelectionUpdated);
	}

	SelectionComponent = PawnLinked->FindComponentByClass<UUnitSelectionComponent>();
	
	if(!SelectionComponent.IsValid())
		return;

	SetFormationSelectionWidget(false);
	SetBehaviorSelectionWidget(false);
	SetBehaviorSelectionWidget(false);
	SetUnitsSelectionWidget(true);

    if (UnitsSelector)
    {
    if (UnitsSelector)
    {
    	// Retrieve Dependencies
        UUnitSpawnComponent* SpawnComp = PawnLinked->FindComponentByClass<UUnitSpawnComponent>();
        UUnitPatrolComponent* PatrolComp = PawnLinked->FindComponentByClass<UUnitPatrolComponent>();
        
        // Inject Dependencies (even if null, to reset)
        UnitsSelector->Init(SpawnComp, PatrolComp);
    }
    }

    if (SelectionComponent.IsValid())
    {
        SelectionComponent->OnSelectedUpdate.AddUniqueDynamic(this, &UJupiterHudWidget::OnSelectionUpdated);
        OnSelectionUpdated();
    }
}

void UJupiterHudWidget::OnSelectionUpdated()
{
    UpdateSubWidgetsContext();
}

void UJupiterHudWidget::UpdateSubWidgetsContext()
{
	if(SelectionComponent.IsValid())
	{
		const TArray<AActor*>& SelectedActors = SelectionComponent->GetSelectedActors();
		
		if (SelectedActors.IsEmpty())
		{
			SetBehaviorSelectionWidget(false);
			if (UnitsSelector)
			{
			    UnitsSelector->SetSelectionContext(SelectedActors);
			}
			
			return;
		}

		SetBehaviorSelectionWidget(true);
		SetFormationSelectionWidget(SelectionComponent->HasGroupSelection());
        if (UnitsSelector)
        {
            UnitsSelector->SetSelectionContext(SelectedActors);
        }

		int Selected = SelectedActors.Num();
		FString Text = FString::FromInt(Selected);
		UnitsSelected->SetText(FText::FromString(Text));
	}
}
