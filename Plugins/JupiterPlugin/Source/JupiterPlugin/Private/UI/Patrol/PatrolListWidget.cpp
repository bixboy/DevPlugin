#include "UI/Patrol/PatrolListWidget.h"
#include "UI/Patrol/PatrolEntryWidget.h"
#include "UI/Patrol/PatrolDetailWidget.h"
#include "Components/Patrol/UnitPatrolComponent.h"
#include "Components/PanelWidget.h"

void UPatrolListWidget::InitializePatrolList(UUnitPatrolComponent* Comp)
{
	UE_LOG(LogTemp, Warning, TEXT("UPatrolListWidget::InitializePatrolList called with Comp: %s"), *GetNameSafe(Comp));
	
	if (PatrolComponent.Get() == Comp)
		return;

	if (PatrolComponent.IsValid())
	{
		PatrolComponent->OnPatrolRoutesChanged.RemoveDynamic(this, &UPatrolListWidget::HandleRoutesChanged);
	}

	PatrolComponent = Comp;

	if (PatrolComponent.IsValid())
	{
		PatrolComponent->OnPatrolRoutesChanged.AddDynamic(this, &UPatrolListWidget::HandleRoutesChanged);
		RefreshList();
	}

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
	if (!PatrolComponent.IsValid())
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

	const TArray<FPatrolRouteItem>& Routes = PatrolComponent->GetActiveRoutes();
	UE_LOG(LogTemp, Warning, TEXT("UPatrolListWidget::RefreshList - Found %d routes."), Routes.Num());

    bool bSelectedStillExists = false;

	for (const FPatrolRouteItem& RouteItem : Routes)
	{
		const FPatrolRoute& Route = RouteItem.RouteData;

        // Distance Filter for UI List
	    if (MaxDisplayDistance > 0.f && !Route.PatrolPoints.IsEmpty())
	    {
            if (APawn* MyPawn = GetOwningPlayerPawn())
            {
                const float DistSq = FVector::DistSquared(MyPawn->GetActorLocation(), Route.PatrolPoints[0]);
                if (DistSq > (MaxDisplayDistance * MaxDisplayDistance))
                {
                    continue; // Skip this route, it's too far
                }
            }
	    }

		if (APlayerController* PC = GetOwningPlayer())
		{
			UPatrolEntryWidget* NewEntry = CreateWidget<UPatrolEntryWidget>(PC, EntryWidgetClass);
			if (NewEntry)
			{
				NewEntry->Setup(Route);
                NewEntry->OnEntrySelected.AddDynamic(this, &UPatrolListWidget::OnEntrySelected);
                
                if (Route.PatrolID == SelectedPatrolID)
                {
                    NewEntry->SetIsSelected(true);
                    bSelectedStillExists = true;
                	
                    if (PatrolDetail)
                    {
                        PatrolDetail->BindToPatrol(SelectedPatrolID, PatrolComponent.Get());
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
        PatrolDetail->BindToPatrol(SelectedPatrolID, PatrolComponent.Get());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UPatrolListWidget: PatrolDetail is NULL when trying to select!"));
    }
}
