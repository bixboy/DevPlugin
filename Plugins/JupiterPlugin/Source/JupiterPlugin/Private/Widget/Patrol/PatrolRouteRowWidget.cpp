#include "Widget/Patrol/PatrolRouteRowWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "Widget/CustomButtonWidget.h"

void UPatrolRouteRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (NameTextBox)
		NameTextBox->OnTextCommitted.AddDynamic(this, &UPatrolRouteRowWidget::OnNameCommitted);

	if (WaitTimeTextBox)
		WaitTimeTextBox->OnTextCommitted.AddDynamic(this, &UPatrolRouteRowWidget::OnWaitTimeCommitted);

	if (TypeComboBox)
	{
		TypeComboBox->OnSelectionChanged.AddDynamic(this, &UPatrolRouteRowWidget::OnTypeChanged);
	}

	if (ReverseButton)
	{
		ReverseButton->OnButtonClicked.AddDynamic(this, &UPatrolRouteRowWidget::OnReverseClicked);
	}

	if (DeleteButton)
	{
		DeleteButton->OnButtonClicked.AddDynamic(this, &UPatrolRouteRowWidget::OnDeleteClicked);
	}

	if (ArrowsCheckBox)
		ArrowsCheckBox->OnCheckStateChanged.AddDynamic(this, &UPatrolRouteRowWidget::OnArrowsChanged);

	if (NumbersCheckBox)
		NumbersCheckBox->OnCheckStateChanged.AddDynamic(this, &UPatrolRouteRowWidget::OnNumbersChanged);
}

void UPatrolRouteRowWidget::NativeDestruct()
{
	Super::NativeDestruct();
	// Cleanup if needed
}

void UPatrolRouteRowWidget::Setup(UUnitPatrolComponent* InComponent, int32 InRouteIndex)
{
	PatrolComponent = InComponent;
	RouteIndex = InRouteIndex;
	TargetUnit = nullptr;

	if (PatrolComponent && PatrolComponent->GetActiveRoutes().IsValidIndex(RouteIndex))
	{
		CachedRoute = PatrolComponent->GetActiveRoutes()[RouteIndex];
		CreateColorButtons();
		RefreshDisplay();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UPatrolRouteRowWidget::Setup - Invalid route data (Component: %s, Index: %d)"), 
			PatrolComponent ? TEXT("Valid") : TEXT("Null"), RouteIndex);
	}
}

void UPatrolRouteRowWidget::Setup(UUnitPatrolComponent* InComponent, AActor* InTargetUnit)
{
	PatrolComponent = InComponent;
	TargetUnit = InTargetUnit;
	RouteIndex = -1;

	if (PatrolComponent && TargetUnit)
	{
		if (PatrolComponent->GetPatrolRouteForUnit(TargetUnit, CachedRoute))
		{
			CreateColorButtons();
			RefreshDisplay();
		}
		else
		{
			// Fallback if no route found (shouldn't happen if called correctly)
			UE_LOG(LogTemp, Warning, TEXT("[PatrolRouteRowWidget] No patrol route found for unit %s"), *TargetUnit->GetName());
		}
	}
}

void UPatrolRouteRowWidget::OnNameCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	CachedRoute.RouteName = FName(*Text.ToString());
	UpdateRoute();
}

void UPatrolRouteRowWidget::OnWaitTimeCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (Text.IsNumeric())
	{
		CachedRoute.WaitTime = FCString::Atof(*Text.ToString());
		UpdateRoute();
	}
}

void UPatrolRouteRowWidget::OnTypeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectedItem == "Loop")
	{
		CachedRoute.PatrolType = EPatrolType::Loop;
	}
	else if (SelectedItem == "PingPong")
	{
		CachedRoute.PatrolType = EPatrolType::PingPong;
	}
	else
	{
		CachedRoute.PatrolType = EPatrolType::Once;
	}
	UpdateRoute();
}

void UPatrolRouteRowWidget::OnReverseClicked(UCustomButtonWidget* Button, int Index)
{
	if (PatrolComponent)
	{
		if (TargetUnit)
		{
			PatrolComponent->Server_ReversePatrolRouteForUnit(TargetUnit);
		}
		else if (RouteIndex >= 0)
		{
			PatrolComponent->Server_ReversePatrolRoute(RouteIndex);
		}
	}
}

void UPatrolRouteRowWidget::OnDeleteClicked(UCustomButtonWidget* Button, int Index)
{
	if (PatrolComponent)
	{
		if (TargetUnit)
		{
			PatrolComponent->Server_RemovePatrolRouteForUnit(TargetUnit);
		}
		else if (RouteIndex >= 0)
		{
			PatrolComponent->Server_RemovePatrolRoute(RouteIndex);
		}
	}
}

void UPatrolRouteRowWidget::OnArrowsChanged(bool bIsChecked)
{
	CachedRoute.bShowArrows = bIsChecked;
	UpdateRoute();
}

void UPatrolRouteRowWidget::OnNumbersChanged(bool bIsChecked)
{
	CachedRoute.bShowNumbers = bIsChecked;
	UpdateRoute();
}

void UPatrolRouteRowWidget::OnColorButtonClicked(UCustomButtonWidget* Button, int Index)
{
	// Presets: 0=Blue, 1=Red, 2=Green, 3=Yellow, 4=Purple
	TArray<FLinearColor> Colors = {
		FLinearColor::Blue,
		FLinearColor::Red,
		FLinearColor::Green,
		FLinearColor::Yellow,
		FLinearColor(0.5f, 0.0f, 0.5f)
	};

	if (Colors.IsValidIndex(Index))
	{
		CachedRoute.RouteColor = Colors[Index];
		UpdateRoute();
	}
}

void UPatrolRouteRowWidget::UpdateRoute()
{
	if (PatrolComponent)
	{
		if (TargetUnit)
		{
			PatrolComponent->Server_UpdateUnitPatrol(TargetUnit, CachedRoute);
		}
		else if (RouteIndex >= 0)
		{
			PatrolComponent->Server_UpdatePatrolRoute(RouteIndex, CachedRoute);
		}
	}
}

void UPatrolRouteRowWidget::RefreshDisplay()
{
	if (NameTextBox)
		NameTextBox->SetText(FText::FromName(CachedRoute.RouteName));

	if (WaitTimeTextBox)
		WaitTimeTextBox->SetText(FText::AsNumber(CachedRoute.WaitTime));

	if (TypeComboBox)
	{
		if (TypeComboBox->GetOptionCount() == 0)
		{
			TypeComboBox->AddOption("Once");
			TypeComboBox->AddOption("Loop");
			TypeComboBox->AddOption("PingPong");
		}

		FString SelectedOption = "Once";
		switch (CachedRoute.PatrolType)
		{
		case EPatrolType::Loop: SelectedOption = "Loop"; break;
		case EPatrolType::PingPong: SelectedOption = "PingPong"; break;
		default: break;
		}
		TypeComboBox->SetSelectedOption(SelectedOption);
	}

	if (ArrowsCheckBox)
		ArrowsCheckBox->SetIsChecked(CachedRoute.bShowArrows);

	if (NumbersCheckBox)
		NumbersCheckBox->SetIsChecked(CachedRoute.bShowNumbers);

	if (AssignedUnitsText)
	{
		FString UnitListStr = FString::Printf(TEXT("Units: %d"), CachedRoute.AssignedUnits.Num());
		// Optional: List names if few
		if (CachedRoute.AssignedUnits.Num() > 0 && CachedRoute.AssignedUnits.Num() <= 3)
		{
			UnitListStr += TEXT(" (");
			for (int32 i = 0; i < CachedRoute.AssignedUnits.Num(); ++i)
			{
				if (AActor* Unit = CachedRoute.AssignedUnits[i].Get())
				{
					UnitListStr += Unit->GetName();
					if (i < CachedRoute.AssignedUnits.Num() - 1) UnitListStr += TEXT(", ");
				}
			}
			UnitListStr += TEXT(")");
		}
		AssignedUnitsText->SetText(FText::FromString(UnitListStr));
	}
}

void UPatrolRouteRowWidget::CreateColorButtons()
{
	if (!ColorContainer || ColorContainer->GetChildrenCount() > 0)
		return;

	TArray<FLinearColor> Colors = {
		FLinearColor::Blue,
		FLinearColor::Red,
		FLinearColor::Green,
		FLinearColor::Yellow,
		FLinearColor(0.5f, 0.0f, 0.5f)
	};

	for (int32 i = 0; i < Colors.Num(); ++i)
	{
		UCustomButtonWidget* Btn = CreateWidget<UCustomButtonWidget>(this, UCustomButtonWidget::StaticClass());
		if (Btn)
		{
			Btn->ButtonIndex = i;
			Btn->SetButtonColor(Colors[i]);
			Btn->OnButtonClicked.AddDynamic(this, &UPatrolRouteRowWidget::OnColorButtonClicked);
			ColorContainer->AddChildToHorizontalBox(Btn);
		}
	}
}
