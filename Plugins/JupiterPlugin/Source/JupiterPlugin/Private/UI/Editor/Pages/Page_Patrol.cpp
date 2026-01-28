#include "UI/Editor/Pages/Page_Patrol.h"
#include "UI/Editor/Widgets/Patrol/PatrolEntryWidget.h"
#include "UI/Editor/Widgets/Patrol/PatrolDetailWidget.h"
#include "Components/Patrol/UnitPatrolComponent.h"
#include "Components/PanelWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UPage_Patrol::InitPage(UUnitSpawnComponent* SpawnComp, UUnitPatrolComponent* PatrolComp, UUnitSelectionComponent* SelComp)
{
	Super::InitPage(SpawnComp, PatrolComp, SelComp);
}

void UPage_Patrol::OnPageOpened()
{
	Super::OnPageOpened();

	if (PatrolComponent.IsValid())
	{
        PatrolComponent->OnPatrolRoutesChanged.RemoveDynamic(this, &UPage_Patrol::HandleRoutesChanged);
		PatrolComponent->OnPatrolRoutesChanged.AddDynamic(this, &UPage_Patrol::HandleRoutesChanged);
		
		RefreshList();
	}
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UPage_Patrol::OnPageOpened - PatrolComponent is Invalid!"));
    }
}

void UPage_Patrol::OnPageClosed()
{
	if (PatrolComponent.IsValid())
	{
		PatrolComponent->OnPatrolRoutesChanged.RemoveDynamic(this, &UPage_Patrol::HandleRoutesChanged);
	}

    if (PatrolDetail)
    {
        PatrolDetail->ClearBinding();
    }
    
    SelectedPatrolID = FGuid();
    
	Super::OnPageClosed();
}

void UPage_Patrol::HandleRoutesChanged()
{
	RefreshList();
}

void UPage_Patrol::RefreshList()
{
	if (!EntryContainer || !PatrolComponent.IsValid() || !EntryWidgetClass)
		return;

	EntryContainer->ClearChildren();

    bool bSelectedStillExists = false;
	const TArray<FPatrolRouteItem>& Routes = PatrolComponent->GetActiveRoutes();

    APawn* MyPawn = GetOwningPlayerPawn();
    FVector MyLoc = MyPawn ? MyPawn->GetActorLocation() : FVector::ZeroVector;

	int32 RouteIndex = 0;
	for (const FPatrolRouteItem& RouteItem : Routes)
	{
		const FPatrolRoute& Route = RouteItem.RouteData;

	    if (MyPawn && MaxDisplayDistance > 0.f && !Route.PatrolPoints.IsEmpty())
	    {
            const float DistSq = FVector::DistSquared(MyLoc, Route.PatrolPoints[0]);
            if (DistSq > (MaxDisplayDistance * MaxDisplayDistance))
            {
            	RouteIndex++;
                continue; 
            }
	    }

        UPatrolEntryWidget* NewEntry = CreateWidget<UPatrolEntryWidget>(GetWorld(), EntryWidgetClass);
        if (NewEntry)
        {
        	float DistToRoute = -1.0f;
        	if (!Route.PatrolPoints.IsEmpty())
        	{
        		DistToRoute = FVector::Dist(MyLoc, Route.PatrolPoints[0]);
        	}
        	
            NewEntry->Setup(Route, RouteIndex, DistToRoute);
            NewEntry->OnEntrySelected.AddDynamic(this, &UPage_Patrol::OnEntrySelected);
            
            if (Route.PatrolID == SelectedPatrolID)
            {
                NewEntry->SetIsSelected(true);
                bSelectedStillExists = true;
                
                if (PatrolDetail)
                {
                    PatrolDetail->SetupDetailWidget(RouteIndex, PatrolComponent.Get());
                }
            }
            
            EntryContainer->AddChild(NewEntry);
        	NewEntry->SetPadding(FMargin(0.f, 2.5f, 0.f, 2.5f));
        }
        RouteIndex++;
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

void UPage_Patrol::OnEntrySelected(int32 LoopIndex, UPatrolEntryWidget* EntryWidget)
{
	if (!EntryWidget)
		return;
	
	SelectedPatrolID = EntryWidget->PatrolID;

	for (int32 i = 0; i < EntryContainer->GetChildrenCount(); ++i)
	{
		if (UPatrolEntryWidget* Entry = Cast<UPatrolEntryWidget>(EntryContainer->GetChildAt(i)))
		{
			Entry->SetIsSelected(Entry->EntryIndex == LoopIndex);
		}
	}

	if (PatrolDetail)
		PatrolDetail->SetupDetailWidget(LoopIndex, PatrolComponent.Get());
}
