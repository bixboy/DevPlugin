#include "Widget/Patrol/PatrolListWidget.h"
#include "Widget/Patrol/PatrolEntryWidget.h"
#include "Components/UnitPatrolComponent.h"
#include "Components/PanelWidget.h"

void UPatrolListWidget::InitializePatrolList(UUnitPatrolComponent* Comp)
{
	UE_LOG(LogTemp, Warning, TEXT("UPatrolListWidget::InitializePatrolList called with Comp: %s"), *GetNameSafe(Comp));
	
	if (PatrolComponent == Comp) return;

	// Unbind from old if necessary (though usually widgets are transient)
	if (PatrolComponent)
	{
		PatrolComponent->OnPatrolRoutesChanged.RemoveDynamic(this, &UPatrolListWidget::HandleRoutesChanged);
	}

	PatrolComponent = Comp;

	if (PatrolComponent)
	{
		PatrolComponent->OnPatrolRoutesChanged.AddDynamic(this, &UPatrolListWidget::HandleRoutesChanged);
		RefreshList();
	}
}

void UPatrolListWidget::HandleRoutesChanged()
{
	UE_LOG(LogTemp, Warning, TEXT("UPatrolListWidget::HandleRoutesChanged"));
	RefreshList();
}

void UPatrolListWidget::RefreshList()
{
	if (!EntryContainer)
	{
		UE_LOG(LogTemp, Error, TEXT("UPatrolListWidget::RefreshList - EntryContainer is NULL! Check BindWidget name 'EntryContainer' in WBP."));
		return;
	}
	if (!PatrolComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("UPatrolListWidget::RefreshList - PatrolComponent is NULL!"));
		return;
	}
	if (!EntryWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("UPatrolListWidget::RefreshList - EntryWidgetClass is NULL! Set it in WBP details."));
		return;
	}

	EntryContainer->ClearChildren();

	const TArray<FPatrolRoute>& Routes = PatrolComponent->GetActiveRoutes();
	UE_LOG(LogTemp, Warning, TEXT("UPatrolListWidget::RefreshList - Found %d routes."), Routes.Num());

	for (const FPatrolRoute& Route : Routes)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			UPatrolEntryWidget* NewEntry = CreateWidget<UPatrolEntryWidget>(PC, EntryWidgetClass);
			if (NewEntry)
			{
				NewEntry->Setup(Route, PatrolComponent);
				EntryContainer->AddChild(NewEntry);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("UPatrolListWidget::RefreshList - Failed to create widget for route %s"), *Route.RouteName.ToString());
			}
		}
	}
}
