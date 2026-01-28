#include "UI/Editor/JupiterEditorPanel.h"
#include "UI/Editor/JupiterPageBase.h"
#include "Components/PanelWidget.h"
#include "Components/WidgetSwitcher.h"
#include "UI/CustomButtonWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

// Component Includes
#include "Components/Unit/UnitSpawnComponent.h"
#include "Components/Patrol/UnitPatrolComponent.h"
#include "Components/Unit/UnitSelectionComponent.h"

void UJupiterEditorPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	FindComponents();
	SetupSidebar();

	if (ContentSwitcher)
	{
		const int32 NumPages = ContentSwitcher->GetChildrenCount();
		for (int32 i = 0; i < NumPages; ++i)
		{
			if (UJupiterPageBase* Page = Cast<UJupiterPageBase>(ContentSwitcher->GetWidgetAtIndex(i)))
			{
				Page->InitPage(SpawnComponent, PatrolComponent, SelectionComponent);
			}
		}
	}

	SetupSidebar();
	SwitchToPage(0);
}

void UJupiterEditorPanel::FindComponents()
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("JupiterEditorPanel: No PlayerPawn found!"));
		return;
	}

	SpawnComponent = PlayerPawn->FindComponentByClass<UUnitSpawnComponent>();
	PatrolComponent = PlayerPawn->FindComponentByClass<UUnitPatrolComponent>();
	SelectionComponent = PlayerPawn->FindComponentByClass<UUnitSelectionComponent>();

	if (!SpawnComponent) 
		UE_LOG(LogTemp, Error, TEXT("JupiterEditorPanel: Missing UnitSpawnComponent on BoardPawn"));
	
	if (!PatrolComponent) 
		UE_LOG(LogTemp, Error, TEXT("JupiterEditorPanel: Missing UnitPatrolComponent on BoardPawn"));
	
	if (!SelectionComponent) 
		UE_LOG(LogTemp, Error, TEXT("JupiterEditorPanel: Missing UnitSelectionComponent on BoardPawn"));
}

void UJupiterEditorPanel::SetupSidebar()
{
	if (!SidebarContainer || !ContentSwitcher || !SidebarButtonClass)
		return;

	SidebarContainer->ClearChildren();
	PageButtons.Reset();

	const int32 NumPages = ContentSwitcher->GetChildrenCount();
	for (int32 i = 0; i < NumPages; ++i)
	{
		UJupiterPageBase* Page = Cast<UJupiterPageBase>(ContentSwitcher->GetWidgetAtIndex(i));
		if (!Page)
			continue;

		UCustomButtonWidget* NewButton = CreateWidget<UCustomButtonWidget>(this, SidebarButtonClass);
		if (NewButton)
		{
			NewButton->ButtonIndex = i;
			NewButton->SetButtonText(Page->PageTitle);
			
			if (Page->PageIcon)
			{
				NewButton->SetButtonTexture(Page->PageIcon);
			}

			NewButton->OnButtonClicked.AddDynamic(this, &UJupiterEditorPanel::OnSidebarButtonClicked);
			
			SidebarContainer->AddChild(NewButton);
			PageButtons.Add(i, NewButton);
		}
	}
}

void UJupiterEditorPanel::OnSidebarButtonClicked(UCustomButtonWidget* Button, int32 Index)
{
	if (!Button) 
		return;
	
	SwitchToPage(Index);
}

void UJupiterEditorPanel::SwitchToPage(int32 PageIndex)
{
	if (!ContentSwitcher)
		return;

	if (UJupiterPageBase* OldPage = Cast<UJupiterPageBase>(ContentSwitcher->GetActiveWidget()))
	{
		OldPage->OnPageClosed();
	}
	
	if (PageIndex >= 0 && PageIndex < ContentSwitcher->GetChildrenCount())
	{
		ContentSwitcher->SetActiveWidgetIndex(PageIndex);
		
		if (UJupiterPageBase* NewPage = Cast<UJupiterPageBase>(ContentSwitcher->GetWidgetAtIndex(PageIndex)))
		{
			NewPage->OnPageOpened();
		}
	}

	for (auto& Elem : PageButtons)
	{
		if (Elem.Value)
		{
			Elem.Value->ToggleButtonIsSelected(Elem.Key == PageIndex);
		}
	}
}
