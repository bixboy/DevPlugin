#include "UI/Patrol/PatrolDetailWidget.h"
#include "Components/Patrol/UnitPatrolComponent.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/Unit/UnitSelectionComponent.h"
#include "GameFramework/PlayerController.h"
#include "Player/PlayerCamera.h"
#include "Player/JupiterPlayerSystem/CameraMovementSystem.h"
#include "UI/CustomButtonWidget.h"


void UPatrolDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Focus)
	{
		Btn_Focus->OnButtonClicked.AddDynamic(this, &UPatrolDetailWidget::OnFocusClicked);
	}
	
	if (Btn_SelectUnits)
	{
		Btn_SelectUnits->OnButtonClicked.AddDynamic(this, &UPatrolDetailWidget::OnSelectUnitsClicked);
	}
	
    if (Btn_Loop)
	{
		Btn_Loop->OnButtonClicked.AddDynamic(this, &UPatrolDetailWidget::OnLoopClicked);
	}
	
    if (Btn_Reverse)
	{
		Btn_Reverse->OnButtonClicked.AddDynamic(this, &UPatrolDetailWidget::OnReverseClicked);
	}
	
	if (Btn_Delete)
	{
		Btn_Delete->OnButtonClicked.AddDynamic(this, &UPatrolDetailWidget::OnDeleteClicked);
	}
	
	if (Input_Name)
	{
		Input_Name->OnTextCommitted.AddDynamic(this, &UPatrolDetailWidget::OnNameCommitted);
	}
}

void UPatrolDetailWidget::BindToPatrol(const FGuid& InPatrolID, UUnitPatrolComponent* Comp)
{
    UE_LOG(LogTemp, Warning, TEXT("UPatrolDetailWidget::BindToPatrol - ID: %s"), *InPatrolID.ToString());
	WeakPatrolComponent = Comp;
	BoundPatrolID = InPatrolID;

	RefreshUI();
}

void UPatrolDetailWidget::ClearBinding()
{
	WeakPatrolComponent = nullptr;
	BoundPatrolID = FGuid();
    
    SetVisibility(ESlateVisibility::Collapsed);
}

void UPatrolDetailWidget::RefreshUI()
{
	if (!WeakPatrolComponent.IsValid())
	{
        UE_LOG(LogTemp, Warning, TEXT("UPatrolDetailWidget::RefreshUI - PatrolComponent is INVALID! Clearing binding."));
        ClearBinding();
		return;
	}

    bool bFound = false;
    FPatrolRoute Route;
    
    const TArray<FPatrolRouteItem>& Routes = WeakPatrolComponent->GetActiveRoutes();
    for (const FPatrolRouteItem& Item : Routes)
    {
        if (Item.RouteData.PatrolID == BoundPatrolID)
        {
            Route = Item.RouteData;
            bFound = true;
            break;
        }
    }

    if (!bFound)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPatrolDetailWidget::RefreshUI - Route with ID %s NOT FOUND in component! Clearing binding."), *BoundPatrolID.ToString());
        ClearBinding();
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("UPatrolDetailWidget::RefreshUI - Route FOUND. Setting Visibility to VISIBLE."));
    SetVisibility(ESlateVisibility::Visible);

    if (Input_Name)
    {
        if (!Input_Name->HasKeyboardFocus())
        {
             Input_Name->SetText(FText::FromName(Route.RouteName));
        }
    }

    if (Text_Info)
    {
        FString Info = FString::Printf(TEXT("%d Units | %d Points"), Route.AssignedUnits.Num(), Route.PatrolPoints.Num());
        Text_Info->SetText(FText::FromString(Info));
    }
}

void UPatrolDetailWidget::OnFocusClicked(UCustomButtonWidget* Button, int Index)
{
	FocusCamera();
}

void UPatrolDetailWidget::OnSelectUnitsClicked(UCustomButtonWidget* Button, int Index)
{
	SelectAssignedUnits();
}

void UPatrolDetailWidget::OnLoopClicked(UCustomButtonWidget* Button, int Index)
{
    if (WeakPatrolComponent.IsValid())
    {

        for (const FPatrolRouteItem& Item : WeakPatrolComponent->GetActiveRoutes())
        {
            const FPatrolRoute& R = Item.RouteData;
            if (R.PatrolID == BoundPatrolID)
            {
                EPatrolType NewType;
                
                if (R.PatrolType == EPatrolType::Loop)
                {
	                NewType = EPatrolType::PingPong;
                }
                else if (R.PatrolType == EPatrolType::PingPong)
                {
	                NewType = EPatrolType::Once;
                }
                else
                {
	                NewType = EPatrolType::Loop;
                }

                FPatrolModPayload Payload;
                Payload.NewType = NewType;
                WeakPatrolComponent->Server_ModifyPatrol(BoundPatrolID, EPatrolModAction::ChangeType, Payload);
                break;
            }
        }
    }
}

void UPatrolDetailWidget::OnReverseClicked(UCustomButtonWidget* Button, int Index)
{
    if (WeakPatrolComponent.IsValid())
    {
        // Payload can be empty for Reverse
        WeakPatrolComponent->Server_ModifyPatrol(BoundPatrolID, EPatrolModAction::Reverse, FPatrolModPayload());
    }
}

void UPatrolDetailWidget::OnDeleteClicked(UCustomButtonWidget* Button, int Index)
{
	DeletePatrol();
    ClearBinding();
}

void UPatrolDetailWidget::OnNameCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (CommitMethod == ETextCommit::OnEnter || CommitMethod == ETextCommit::OnUserMovedFocus)
    {
        if (WeakPatrolComponent.IsValid())
        {
            FPatrolModPayload Payload;
            Payload.NewName = FName(*Text.ToString());
            WeakPatrolComponent->Server_ModifyPatrol(BoundPatrolID, EPatrolModAction::Rename, Payload);
        }
    }
}

void UPatrolDetailWidget::FocusCamera()
{
    if (!WeakPatrolComponent.IsValid())
    	return;

    FPatrolRoute Route;
    bool bFound = false;
    for (const FPatrolRouteItem& Item : WeakPatrolComponent->GetActiveRoutes())
    {
        if (Item.RouteData.PatrolID == BoundPatrolID)
        {
            Route = Item.RouteData;
            bFound = true;
            break;
        }
    }
	
    if (!bFound || Route.PatrolPoints.Num() == 0)
    	return;

    FVector Sum = FVector::ZeroVector;
    for (const FVector& Pt : Route.PatrolPoints)
    {
        Sum += Pt;
    }
	
    FVector FocusLocation = Sum / Route.PatrolPoints.Num();
    APlayerController* PC = WeakPatrolComponent->GetWorld()->GetFirstPlayerController();
	
    if (PC && PC->GetPawn())
    {
        if (APlayerCamera* CameraPawn = Cast<APlayerCamera>(PC->GetPawn()))
        {
            if (UCameraMovementSystem* MoveSys = CameraPawn->GetMovementSystem())
            {
                MoveSys->MoveToLocation(FocusLocation, 1.0f);
                return;
            }
        }
        PC->ClientSetLocation(FocusLocation, FRotator::ZeroRotator);
    }
}

void UPatrolDetailWidget::SelectAssignedUnits()
{
    if (!WeakPatrolComponent.IsValid())
    	return;

     FPatrolRoute Route;
    bool bFound = false;
    for (const FPatrolRouteItem& Item : WeakPatrolComponent->GetActiveRoutes())
    {
        if (Item.RouteData.PatrolID == BoundPatrolID)
        {
            Route = Item.RouteData;
            bFound = true;
            break;
        }
    }
	
    if (!bFound)
    	return;

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

void UPatrolDetailWidget::DeletePatrol()
{
    if (WeakPatrolComponent.IsValid())
    {
        WeakPatrolComponent->Server_RemovePatrolRouteByID(BoundPatrolID);
    }
}
