#include "Subsystems/ContextMenuSubsystem.h"
#include "UI/CustomContextMenu.h"
#include "Blueprint/UserWidget.h"


void UContextMenuSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UContextMenuSubsystem::ShowContextMenu(const TArray<FContextMenuItem>& Options, const FVector2D& ScreenPosition)
{
    if (!MenuWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("ContextMenuSubsystem: MenuWidgetClass not set!"));
        return;
    }

    if (!ActiveMenuWidget)
    {
        ActiveMenuWidget = CreateWidget<UCustomContextMenu>(GetWorld(), MenuWidgetClass);
    }

    if (ActiveMenuWidget)
    {
        ActiveMenuWidget->AddToViewport(100);
        ActiveMenuWidget->SetPositionInViewport(ScreenPosition);
        ActiveMenuWidget->BuildMenu(Options);
    }
}

void UContextMenuSubsystem::HideContextMenu()
{
    if (ActiveMenuWidget)
    {
	    ActiveMenuWidget->RemoveFromParent();
    	ActiveMenuWidget->HideMenu();
    }
}
