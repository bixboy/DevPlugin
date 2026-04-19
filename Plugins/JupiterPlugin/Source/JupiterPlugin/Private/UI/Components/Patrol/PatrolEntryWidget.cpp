#include "UI/Editor/Widgets/Patrol/PatrolEntryWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/CustomButtonWidget.h"


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

void UPatrolEntryWidget::Setup(const FPatrolRoute& Route, int32 Index, float Distance)
{
	PatrolID = Route.PatrolID;
	EntryIndex = Index;
	
	if (Text_RouteName)
	{
		Text_RouteName->SetText(FText::FromName(Route.RouteName));
	}
	if (Text_UnitCount)
	{
		FText MyText = FText::Format(NSLOCTEXT("MyNamespace", "UnitCountKey", "{0} Units"), FText::AsNumber(Route.AssignedUnits.Num()));
		Text_UnitCount->SetText(MyText);
	}
	if (Text_Distance)
	{
		if (Distance >= 0.0f)
		{
			FString DistStr = (Distance > 100000.0f) ? FString::Printf(TEXT("%.1f km"), Distance / 100000.0f) : FString::Printf(TEXT("%d m"), FMath::RoundToInt(Distance / 100.0f));
			Text_Distance->SetText(FText::FromString(DistStr));
			Text_Distance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			Text_Distance->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (Image_RouteColor)
	{
		Image_RouteColor->SetColorAndOpacity(Route.RouteColor);
	}

	if (Img_TypeIcon)
	{
		UTexture2D* TargetIcon = nullptr;
		if (Route.PatrolType == EPatrolType::Loop)
		{
			TargetIcon = Icon_Loop;
		}
		else
		{
			TargetIcon = Icon_PingPong;
		}

		if (TargetIcon)
		{
			Img_TypeIcon->SetBrushFromTexture(TargetIcon);
			Img_TypeIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			Img_TypeIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	SetIsSelected(false);
}

void UPatrolEntryWidget::SetIsSelected(bool bIsSelected)
{
	if (SelectionBackground)
	{
		SelectionBackground->SetColorAndOpacity(bIsSelected ? SelectedColor : NormalColor);
	}
}

void UPatrolEntryWidget::OnSelectClicked(UCustomButtonWidget* Button, int Index)
{
    // UE_LOG(LogTemp, Warning, TEXT("UPatrolEntryWidget::OnSelectClicked called for PatrolID: %s"), *PatrolID.ToString());
	OnEntrySelected.Broadcast(EntryIndex, this);
}
