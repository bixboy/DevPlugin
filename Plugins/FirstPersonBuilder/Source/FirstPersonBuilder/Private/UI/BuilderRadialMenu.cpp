// Copyright 2026

#include "UI/BuilderRadialMenu.h"
#include "Objects/PlayerConstructionObject.h"
#include "GameFramework/PlayerController.h"

void UBuilderRadialMenu::NativeConstruct()
{
	Super::NativeConstruct();
	OnSegmentSelected.AddUniqueDynamic(this, &UBuilderRadialMenu::HandleSegmentSelected);
}

void UBuilderRadialMenu::NativeDestruct()
{
	OnSegmentSelected.RemoveDynamic(this, &UBuilderRadialMenu::HandleSegmentSelected);
	Super::NativeDestruct();
}

void UBuilderRadialMenu::InitializeMenu(UPlayerConstructionObject* InBuilder, const TArray<UPlacementPropData*>& InRecipes)
{
	OwningBuilder = InBuilder;
	AvailableRecipes = InRecipes;

	ClearSegments();

	for (UPlacementPropData* Recipe : AvailableRecipes)
	{
		if (Recipe)
		{
			UTexture2D* LoadedIcon = Recipe->EntityThumbnail.LoadSynchronous();
			FText Label = FText::FromName(Recipe->EntityID);
			
			FLinearColor SegColor = Recipe->Color;
			if (SegColor.A <= 0.01f)
			{
				SegColor = FLinearColor::White;
			}

			AddSegment(Recipe->EntityID, Label, LoadedIcon, SegColor);
		}
	}
}

void UBuilderRadialMenu::OpenBuilderMenu()
{
	OpenMenu(); // Call base PrismRadialMenu function
	
	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
		PC->SetIgnoreLookInput(true);
	}
}

void UBuilderRadialMenu::CloseBuilderMenu()
{
	CloseMenuAndGetSelection(); // Call base function to run animation/logic
	
	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
		PC->SetIgnoreLookInput(false);
	}

	RemoveFromParent();
}

void UBuilderRadialMenu::HandleSegmentSelected(FName SegmentID)
{
	if (OwningBuilder.IsValid())
	{
		OwningBuilder->HandleRadialMenuSelection(SegmentID);
	}
}
