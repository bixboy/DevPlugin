#include "UI/CustomContextMenu.h"
#include "UI/CustomButtonWidget.h"
#include "Components/PanelWidget.h"


void UCustomContextMenu::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
}

void UCustomContextMenu::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusLost(InFocusEvent);
	
    if (!IsHovered())
    {
	    HideMenu();
    }
}

void UCustomContextMenu::BuildMenu(const TArray<FContextMenuItem>& Items)
{
    if (!MenuContainer)
    {
        UE_LOG(LogTemp, Warning, TEXT("UCustomContextMenu::BuildMenu - MenuContainer is null!"));
        return;
    }

    if (!ButtonClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("UCustomContextMenu::BuildMenu - ButtonClass is null!"));
        return;
    }

    MenuContainer->ClearChildren();
    CurrentItems = Items;

    for (int32 i = 0; i < CurrentItems.Num(); ++i)
    {
        UCustomButtonWidget* dynamicButton = CreateWidget<UCustomButtonWidget>(this, ButtonClass);
        if (dynamicButton)
        {
            const FContextMenuItem& Item = CurrentItems[i];
            
            dynamicButton->SetButtonText(Item.Label);
            if (Item.Icon)
            {
                dynamicButton->SetButtonTexture(Item.Icon);
            }

            dynamicButton->ButtonIndex = i;
            dynamicButton->OnButtonClicked.AddDynamic(this, &UCustomContextMenu::OnOptionClicked);
        	dynamicButton->SetPadding(FMargin(10.0f));
            dynamicButton->SetIsFocusable(false);
        	
            MenuContainer->AddChild(dynamicButton);
        }
    }
}

void UCustomContextMenu::HideMenu()
{
    RemoveFromParent();
}

void UCustomContextMenu::OnOptionClicked(UCustomButtonWidget* Button, int32 Index)
{
    if (CurrentItems.IsValidIndex(Index))
    {
        CurrentItems[Index].Action.ExecuteIfBound();
    }
	
    HideMenu();
}
