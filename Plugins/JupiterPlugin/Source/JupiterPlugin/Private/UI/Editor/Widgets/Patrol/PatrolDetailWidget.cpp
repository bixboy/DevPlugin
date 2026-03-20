#include "UI/Editor/Widgets/Patrol/PatrolDetailWidget.h"
#include "UI/Editor/Widgets/Patrol/PatrolRoutePreview.h"
#include "Components/Patrol/UnitPatrolComponent.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/Unit/UnitSelectionComponent.h"
#include "GameFramework/PlayerController.h"
#include "Player/PlayerCamera.h"
#include "Player/JupiterPlayerSystem/CameraMovementSystem.h"
#include "UI/CustomContextMenu.h"
#include "UI/ContextMenuData.h"
#include "UI/CustomButtonWidget.h"
#include "UI/JupiterToggleSwitch.h"
#include "Data/PatrolData.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "UI/Notification/NotificationSubsystem.h"
#include "UI/JupiterHudWidget.h"
#include "UI/Editor/JupiterEditorPanel.h"
#include "UI/Editor/Pages/Page_Patrol.h"
#include "Components/WidgetSwitcher.h"


void UPatrolDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (Btn_SelectUnits)
	{
		Btn_SelectUnits->OnButtonClicked.AddDynamic(this, &UPatrolDetailWidget::OnSelectUnitsClicked);
	}

	if (Btn_AddSelectedUnits)
	{
		Btn_AddSelectedUnits->OnButtonClicked.AddDynamic(this, &UPatrolDetailWidget::OnAddSelectedUnitsClicked);
	}
	
    if (Btn_LoopToggle)
	{
		Btn_LoopToggle->OnToggled.AddDynamic(this, &UPatrolDetailWidget::OnLoopToggled);
	}
	
    if (Btn_ReverseToggle)
	{
		Btn_ReverseToggle->OnToggled.AddDynamic(this, &UPatrolDetailWidget::OnReverseToggled);
	}

	if (Btn_PauseToggle)
	{
		Btn_PauseToggle->OnToggled.AddDynamic(this, &UPatrolDetailWidget::OnPauseToggled);
	}
	
	if (Btn_DeletePatrol)
	{
		Btn_DeletePatrol->OnButtonClicked.AddDynamic(this, &UPatrolDetailWidget::OnDeletePatrolClicked);
        Btn_DeletePatrol->OnButtonRightClicked.AddDynamic(this, &UPatrolDetailWidget::OnDeletePatrolRightClicked);
	}
	
	if (Input_Name)
	{
		Input_Name->OnTextCommitted.AddDynamic(this, &UPatrolDetailWidget::OnNameCommitted);
	}

    if (RoutePreviewWidget)
    {
        RoutePreviewWidget->OnPreviewClicked.AddDynamic(this, &UPatrolDetailWidget::FocusCameraHandler);
    }
}

void UPatrolDetailWidget::SetupDetailWidget(int32 Index, UUnitPatrolComponent* Comp)
{
	WeakPatrolComponent = Comp;
	CurrentPatrolIndex = Index;

	if (WeakPatrolComponent.IsValid() && WeakPatrolComponent->GetActiveRoutes().IsValidIndex(CurrentPatrolIndex))
	{
		const FPatrolRoute& Route = WeakPatrolComponent->GetActiveRoutes()[CurrentPatrolIndex].RouteData;
		BoundPatrolID = Route.PatrolID;
		// UE_LOG(LogTemp, Warning, TEXT("UPatrolDetailWidget::SetupDetailWidget - ID: %s"), *BoundPatrolID.ToString());
        
        if (Btn_ReverseToggle)
        {
            Btn_ReverseToggle->SetIsToggled(false);
        }

		RefreshUI();
        
        WeakPatrolComponent->SetUISelectedPatrol(BoundPatrolID);
	}
	else
	{
		ClearBinding();
	}
}

void UPatrolDetailWidget::ClearBinding()
{
	WeakPatrolComponent = nullptr;
	BoundPatrolID = FGuid();
	CurrentPatrolIndex = -1;
    
    SetVisibility(ESlateVisibility::Collapsed);

	if (ActiveContextMenu)
	{
		ActiveContextMenu->SetVisibility(ESlateVisibility::Collapsed);
	}

    if (WeakPatrolComponent.IsValid())
    {
        WeakPatrolComponent->SetUISelectedPatrol(FGuid());
    }
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
        FString Info = FString::Printf(TEXT("%d : Units"), Route.AssignedUnits.Num());
        Text_Info->SetText(FText::FromString(Info));
    }

    if (Text_WaypointCount)
    {
        FString CountStr = FString::Printf(TEXT("%d : Points"), Route.PatrolPoints.Num());
        Text_WaypointCount->SetText(FText::FromString(CountStr));
    }
    
    if (Btn_LoopToggle)
    {
        Btn_LoopToggle->SetIsToggled(Route.PatrolType == EPatrolType::Loop);
    }

    if (Btn_PauseToggle)
    {
        Btn_PauseToggle->SetIsToggled(Route.bPaused);
    }

    if (RoutePreviewWidget)
    {
        RoutePreviewWidget->SetPatrolPoints(Route.PatrolPoints, Route.PatrolType == EPatrolType::Loop);
    }

}

void UPatrolDetailWidget::OnSelectUnitsClicked(UCustomButtonWidget* Button, int Index)
{
	SelectAssignedUnits();
}

void UPatrolDetailWidget::OnAddSelectedUnitsClicked(UCustomButtonWidget* Button, int Index)
{
    if (!WeakPatrolComponent.IsValid())
    	return;

    APlayerController* PC = GetOwningPlayer();
    if (!PC) 
        PC = WeakPatrolComponent->GetWorld()->GetFirstPlayerController();

    if (PC && PC->GetPawn())
    {
        if (UUnitSelectionComponent* SelComp = PC->GetPawn()->FindComponentByClass<UUnitSelectionComponent>())
        {
            const TArray<AActor*>& Selected = SelComp->GetSelectedActors();
            if (Selected.Num() > 0)
            {
                WeakPatrolComponent->Server_AssignUnitsToPatrol(Selected, BoundPatrolID);
                
                if (UGameInstance* GI = GetGameInstance())
                {
                    if (UNotificationSubsystem* NotifSys = GI->GetSubsystem<UNotificationSubsystem>())
                    {
                        FPatrolRoute Route;
                        bool bFoundRoute = false;
                        if (WeakPatrolComponent.IsValid())
                        {
                             for (const FPatrolRouteItem& Item : WeakPatrolComponent->GetActiveRoutes())
                             {
                                 if (Item.RouteData.PatrolID == BoundPatrolID)
                                 {
                                     Route = Item.RouteData;
                                     bFoundRoute = true;
                                     break;
                                 }
                             }
                        }

                        if (bFoundRoute)
                        {
                            FNotificationData Data;
                            
                            FString TitleStr = FString::Printf(TEXT("Units assigned to %s"), *Route.RouteName.ToString());
                            Data.Title = FText::FromString(TitleStr);
                            
                            FString TypeStr = (Route.PatrolType == EPatrolType::Loop) ? TEXT("Loop") : TEXT("PingPong");
                            
                            FString MsgStr = FString::Printf(TEXT("%s | Added: %d"), *TypeStr, Selected.Num());
                            Data.SubTitle = FText::FromString(MsgStr);
                            Data.Duration = NotificationDuration;
                            
                            Data.AccentColor = Route.RouteColor;
                            
                            Data.NativeAction.BindLambda([WeakComp = WeakPatrolComponent, BoundID = BoundPatrolID]()
                            {
                                UE_LOG(LogTemp, Warning, TEXT("PatrolAction - Triggered for ID: %s"), *BoundID.ToString());

                                if (!WeakComp.IsValid())
                                {
                                     UE_LOG(LogTemp, Error, TEXT("PatrolAction - Captured PatrolComponent INVALID"));
                                     return;
                                }
                                
                                UWorld* World = WeakComp->GetWorld();
                                if (!World) 
                                	return;

                                APlayerController* PlayerController = World->GetFirstPlayerController();
                                if (!PlayerController) 
                                	return;
                                
                                APawn* PlayerPawn = PlayerController->GetPawn();
                                if (!PlayerPawn)
                                	return;
                                
                                if (UUnitSelectionComponent* SelComp = PlayerPawn->FindComponentByClass<UUnitSelectionComponent>())
                                {
                                    if (UJupiterHudWidget* HUD = SelComp->GetHudWidget())
                                    {
                                        if (UJupiterEditorPanel* Editor = HUD->EditorPanel)
                                        {
                                            if (UWidgetSwitcher* Switcher = Editor->GetContentSwitcher())
                                            {
                                                for (int32 i = 0; i < Switcher->GetChildrenCount(); ++i)
                                                {
                                                    if (UPage_Patrol* PatrolPage = Cast<UPage_Patrol>(Switcher->GetWidgetAtIndex(i)))
                                                    {
                                                        UE_LOG(LogTemp, Warning, TEXT("PatrolAction - Switching to Page %d and selecting ID"), i);
                                                        Editor->SwitchToPage(i);
                                                        PatrolPage->SelectPatrolByID(BoundID);
                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                        else
                                        {
                                             UE_LOG(LogTemp, Error, TEXT("PatrolAction - Editor Panel NULL"));
                                        }
                                    }
                                    else
                                    {
                                         UE_LOG(LogTemp, Error, TEXT("PatrolAction - HUD NULL"));
                                    }
                                }
                            });

                            NotifSys->AddNotification(Data);
                        }
                    }
                }
            }
        }
    }
}

void UPatrolDetailWidget::OnLoopToggled(bool bIsToggled)
{
    if (WeakPatrolComponent.IsValid())
    {
        EPatrolType NewType = bIsToggled ? EPatrolType::Loop : EPatrolType::PingPong;
        
        FPatrolModPayload Payload;
        Payload.NewType = NewType;
        WeakPatrolComponent->Server_ModifyPatrol(BoundPatrolID, EPatrolModAction::ChangeType, Payload);
    }
}

void UPatrolDetailWidget::OnReverseToggled(bool bIsToggled)
{
    if (WeakPatrolComponent.IsValid())
    {
        WeakPatrolComponent->Server_ModifyPatrol(BoundPatrolID, EPatrolModAction::Reverse, FPatrolModPayload());
    }
}

void UPatrolDetailWidget::OnPauseToggled(bool bIsToggled)
{
    if (WeakPatrolComponent.IsValid())
    {
        WeakPatrolComponent->Server_ModifyPatrol(BoundPatrolID, EPatrolModAction::TogglePause, FPatrolModPayload());
    }
}

void UPatrolDetailWidget::OnDeletePatrolClicked(UCustomButtonWidget* Button, int Index)
{
	if (WeakPatrolComponent.IsValid())
	{
		WeakPatrolComponent->Server_RemovePatrolWithOption(BoundPatrolID, CurrentDeleteOption);
	}
	
    ClearBinding();
}

void UPatrolDetailWidget::OnDeletePatrolRightClicked(UCustomButtonWidget* Button, int Index)
{
    if (!ContextMenuClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("UPatrolDetailWidget::OnDeletePatrolRightClicked - ContextMenuClass is NOT set!"));
        return;
    }

    if (!ActiveContextMenu)
    {
        ActiveContextMenu = CreateWidget<UCustomContextMenu>(this, ContextMenuClass);
    }

    if (ActiveContextMenu)
    {
        if (!ActiveContextMenu->IsInViewport())
        {
            ActiveContextMenu->AddToViewport(500);
        }
        TArray<FContextMenuItem> Items;

        // Option 1: Disband
        {
            FContextMenuItem Item;
            Item.Label = FText::FromString("Disband (Stop)");
            Item.Tooltip = FText::FromString("Default: Units will stop patrolling.");
            Item.Action.BindLambda([this]()
            {
                CurrentDeleteOption = EPatrolDeleteOption::Disband;
                OnDeletePatrolClicked(nullptr, 0);
            });
            Items.Add(Item);
        }

        // Option 2: Join Nearest
        {
            FContextMenuItem Item;
            Item.Label = FText::FromString("Join Nearest Patrol");
            Item.Tooltip = FText::FromString("Units will move to and join the nearest patrol.");
            Item.Action.BindLambda([this]()
            {
                CurrentDeleteOption = EPatrolDeleteOption::JoinNearest;
                OnDeletePatrolClicked(nullptr, 0);
            });
            Items.Add(Item);
        }

        ActiveContextMenu->BuildMenu(Items);
        ActiveContextMenu->SetVisibility(ESlateVisibility::Visible);
        ActiveContextMenu->SetUserFocus(GetOwningPlayer());
        
        FVector2D MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(this);
        ActiveContextMenu->SetPositionInViewport(MousePosition, false);
    }
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
