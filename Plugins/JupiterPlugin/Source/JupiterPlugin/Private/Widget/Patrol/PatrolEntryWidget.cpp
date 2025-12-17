#include "Widget/Patrol/PatrolEntryWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Widget/CustomButtonWidget.h"

void UPatrolEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Select)
	{
		Btn_Select->OnButtonClicked.AddDynamic(this, &UPatrolEntryWidget::OnSelectClicked);
	}
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UPatrolEntryWidget: Btn_Select is NULL!"));
    }
}

void UPatrolEntryWidget::Setup(const FPatrolRoute& Route)
{
	PatrolID = Route.PatrolID;
	
	if (Text_RouteName)
	{
		Text_RouteName->SetText(FText::FromName(Route.RouteName));
	}
	if (Text_UnitCount)
	{
		Text_UnitCount->SetText(FText::AsNumber(Route.AssignedUnits.Num()));
	}
	if (Image_RouteColor)
	{
		Image_RouteColor->SetColorAndOpacity(Route.RouteColor);
	}

	SetIsSelected(false);
}

void UPatrolEntryWidget::SetIsSelected(bool bIsSelected)
{
	if (Image_SelectionHighlight)
	{
		Image_SelectionHighlight->SetColorAndOpacity(bIsSelected ? SelectedColor : NormalColor);
	}
}

void UPatrolEntryWidget::OnSelectClicked(UCustomButtonWidget* Button, int Index)
{
    UE_LOG(LogTemp, Warning, TEXT("UPatrolEntryWidget::OnSelectClicked called for PatrolID: %s"), *PatrolID.ToString());
	OnEntrySelected.Broadcast(PatrolID);
}
