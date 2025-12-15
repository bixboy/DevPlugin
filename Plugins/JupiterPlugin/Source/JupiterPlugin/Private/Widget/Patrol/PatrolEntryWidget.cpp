#include "Widget/Patrol/PatrolEntryWidget.h"
#include "Components/UnitPatrolComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Player/PlayerCamera.h"
#include "Player/JupiterPlayerSystem/CameraMovementSystem.h"
#include "Components/UnitSelectionComponent.h"
#include "Interfaces/Selectable.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Widget/CustomButtonWidget.h"
// #include "Components/Button.h" // Not needed unless we used standard button elsewhere

void UPatrolEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Focus)
	{
		Btn_Focus->OnButtonClicked.AddDynamic(this, &UPatrolEntryWidget::OnFocusClicked);
	}
	if (Btn_SelectUnits)
	{
		Btn_SelectUnits->OnButtonClicked.AddDynamic(this, &UPatrolEntryWidget::OnSelectUnitsClicked);
	}
	if (Btn_Delete)
	{
		Btn_Delete->OnButtonClicked.AddDynamic(this, &UPatrolEntryWidget::OnDeleteClicked);
	}
}

void UPatrolEntryWidget::OnFocusClicked(UCustomButtonWidget* Button, int Index)
{
	FocusCamera();
}

void UPatrolEntryWidget::OnSelectUnitsClicked(UCustomButtonWidget* Button, int Index)
{
	SelectAssignedUnits();
}

void UPatrolEntryWidget::OnDeleteClicked(UCustomButtonWidget* Button, int Index)
{
	DeletePatrol();
}

void UPatrolEntryWidget::Setup(const FPatrolRoute& Route, UUnitPatrolComponent* PatrolComp)
{
	WeakPatrolComponent = PatrolComp;
	PatrolID = Route.PatrolID;
	
	UnitCount = Route.AssignedUnits.Num();
	PointCount = Route.PatrolPoints.Num();
	RouteName = Route.RouteName;
	RouteColor = Route.RouteColor;

	// Set Description based on type
	switch (Route.PatrolType)
	{
	case EPatrolType::Loop:
		Description = NSLOCTEXT("PatrolUI", "Loop", "Loop");
		break;
	case EPatrolType::PingPong:
		Description = NSLOCTEXT("PatrolUI", "PingPong", "Ping Pong");
		break;
	case EPatrolType::Once:
		Description = NSLOCTEXT("PatrolUI", "Once", "Once");
		break;
	default:
		Description = NSLOCTEXT("PatrolUI", "Unknown", "Unknown");
		break;
	}

	// Update Visuals
	if (Text_RouteName)
	{
		Text_RouteName->SetText(FText::FromName(RouteName));
	}
	if (Text_UnitCount)
	{
		Text_UnitCount->SetText(FText::AsNumber(UnitCount));
	}
	if (Text_PointCount)
	{
		Text_PointCount->SetText(FText::AsNumber(PointCount));
	}
	if (Text_Description)
	{
		Text_Description->SetText(Description);
	}
	if (Image_RouteColor)
	{
		Image_RouteColor->SetColorAndOpacity(RouteColor);
	}

	// Calculate focus location (average of points or first point)
	if (Route.PatrolPoints.Num() > 0)
	{
		FVector Sum = FVector::ZeroVector;
		for (const FVector& Pt : Route.PatrolPoints)
		{
			Sum += Pt;
		}
		FocusLocation = Sum / Route.PatrolPoints.Num();
	}
	else
	{
		FocusLocation = FVector::ZeroVector;
	}
}

void UPatrolEntryWidget::FocusCamera()
{
	if (!WeakPatrolComponent.IsValid())
	{
		return;
	}

	APlayerController* PC = WeakPatrolComponent->GetWorld()->GetFirstPlayerController();
	if (PC && PC->GetPawn())
	{
		if (APlayerCamera* CameraPawn = Cast<APlayerCamera>(PC->GetPawn()))
		{
			if (UCameraMovementSystem* MoveSys = CameraPawn->GetMovementSystem())
			{
				MoveSys->MoveToLocation(FocusLocation, 1.0f); // 1 second smooth transition
				return;
			}
		}

		// Fallback if no CameraMovementSystem
		PC->ClientSetLocation(FocusLocation, FRotator::ZeroRotator); 
	}
}

void UPatrolEntryWidget::SelectAssignedUnits()
{
	if (!WeakPatrolComponent.IsValid())
		return;

	FPatrolRoute Route;
	// We need to fetch the fresh route data because assigned units might have changed?
	// But getting by ID is tricky without exposing a helper. 
	// The Setup data might be stale.
	// Ideally we query the comp.
	
	// Let's iterate comp routes to find ours.
	bool bFound = false;
	for (const FPatrolRoute& R : WeakPatrolComponent->GetActiveRoutes())
	{
		if (R.PatrolID == PatrolID)
		{
			Route = R;
			bFound = true;
			break;
		}
	}

	if (!bFound) return;

	APlayerController* PC = WeakPatrolComponent->GetWorld()->GetFirstPlayerController();
	if (PC && PC->GetPawn())
	{
		if (UUnitSelectionComponent* SelComp = PC->GetPawn()->FindComponentByClass<UUnitSelectionComponent>())
		{
			TArray<AActor*> ValidUnits;
			for (const auto& UnitPtr : Route.AssignedUnits)
			{
				if (AActor* Unit = UnitPtr.Get())
				{
					ValidUnits.Add(Unit);
				}
			}
			
			if (ValidUnits.Num() > 0)
			{
				SelComp->Handle_Selection(ValidUnits);
			}
		}
	}
}

void UPatrolEntryWidget::DeletePatrol()
{
	if (WeakPatrolComponent.IsValid())
	{
		WeakPatrolComponent->Server_RemovePatrolRouteByID(PatrolID);
	}
}
