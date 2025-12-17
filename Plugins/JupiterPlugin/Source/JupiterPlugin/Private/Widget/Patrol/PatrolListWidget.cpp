#include "Widget/Patrol/PatrolListWidget.h"
#include "Widget/Patrol/PatrolEntryWidget.h"
#include "Widget/Patrol/PatrolDetailWidget.h"
#include "Components/UnitPatrolComponent.h"
#include "Components/PanelWidget.h"

void UPatrolListWidget::InitializePatrolList(UUnitPatrolComponent* Comp)
{
	UE_LOG(LogTemp, Warning, TEXT("UPatrolListWidget::InitializePatrolList called with Comp: %s"), *GetNameSafe(Comp));
	
	if (PatrolComponent == Comp) return;

	// Unbind from old if necessary
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

    // Clear detail view on init
    if (PatrolDetail)
    {
        PatrolDetail->ClearBinding();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UPatrolListWidget: PatrolDetail is NULL! Check WBP BindWidget."));
    }
    SelectedPatrolID = FGuid();
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

    bool bSelectedStillExists = false;

	for (const FPatrolRoute& Route : Routes)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			UPatrolEntryWidget* NewEntry = CreateWidget<UPatrolEntryWidget>(PC, EntryWidgetClass);
			if (NewEntry)
			{
				NewEntry->Setup(Route);
                NewEntry->OnEntrySelected.AddDynamic(this, &UPatrolListWidget::OnEntrySelected);
                
                // Maintain selection state
                if (Route.PatrolID == SelectedPatrolID)
                {
                    NewEntry->SetIsSelected(true);
                    bSelectedStillExists = true;
                    // Update detail view just in case data changed (name renamed etc)
                    if (PatrolDetail)
                    {
                        PatrolDetail->BindToPatrol(SelectedPatrolID, PatrolComponent);
                    }
                }
                
				EntryContainer->AddChild(NewEntry);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("UPatrolListWidget::RefreshList - Failed to create widget for route %s"), *Route.RouteName.ToString());
			}
		}
	}

    // specific logic: if selected patrol is gone, clear details
    if (!bSelectedStillExists && SelectedPatrolID.IsValid())
    {
        SelectedPatrolID = FGuid();
        if (PatrolDetail)
        {
            PatrolDetail->ClearBinding();
        }
    }
}

void UPatrolListWidget::OnEntrySelected(FGuid PatrolID)
{
    UE_LOG(LogTemp, Warning, TEXT("UPatrolListWidget::OnEntrySelected called with PatrolID: %s"), *PatrolID.ToString());
    SelectedPatrolID = PatrolID;
    
    // Update visuals
    for (int32 i = 0; i < EntryContainer->GetChildrenCount(); ++i)
    {
        if (UPatrolEntryWidget* Entry = Cast<UPatrolEntryWidget>(EntryContainer->GetChildAt(i)))
        {
            Entry->SetIsSelected(Entry->PatrolID == SelectedPatrolID);
        }
    }

    if (PatrolDetail)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPatrolListWidget: Binding PatrolDetail..."));
        PatrolDetail->BindToPatrol(SelectedPatrolID, PatrolComponent);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UPatrolListWidget: PatrolDetail is NULL when trying to select!"));
    }
}
