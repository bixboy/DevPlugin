#include "UI/Editor/Pages/Page_PropSpawn.h"

#include "Animation/WidgetAnimation.h"
#include "Data/Placement/PlacementItemData.h"
#include "Data/Placement/PresetData.h"
#include "UI/Editor/Widgets/Spawn/PropsEntryWidget.h"
#include "UI/Editor/Widgets/Spawn/PresetEntryWidget.h"
#include "UI/CustomButtonWidget.h"
#include "Components/Placement/PresetManagerComponent.h"
#include "Components/WrapBox.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "Components/SizeBox.h"

#define LOCTEXT_NAMESPACE "Page_PropSpawn"


// ------------------------------------------------------------------
// Lifecycle
// ------------------------------------------------------------------

void UPage_PropSpawn::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (SearchTextBox)
		SearchTextBox->OnTextChanged.AddDynamic(this, &UPage_PropSpawn::OnSearchTextChanged);
	
	if (Btn_OpenPreset)
		Btn_OpenPreset->OnButtonClicked.AddDynamic(this, &UPage_PropSpawn::OnOpenPresetClicked);

	if (Btn_SavePreset)
		Btn_SavePreset->OnButtonClicked.AddDynamic(this, &UPage_PropSpawn::OnSavePresetClicked);
}

void UPage_PropSpawn::NativeDestruct()
{
	if (SearchTextBox)
		SearchTextBox->OnTextChanged.RemoveDynamic(this, &UPage_PropSpawn::OnSearchTextChanged);
	
	if (Btn_OpenPreset)
		Btn_OpenPreset->OnButtonClicked.RemoveDynamic(this, &UPage_PropSpawn::OnOpenPresetClicked);

	if (Btn_SavePreset)
		Btn_SavePreset->OnButtonClicked.RemoveDynamic(this, &UPage_PropSpawn::OnSavePresetClicked);

	for (UCustomButtonWidget* Btn : CategoryButtons)
	{
		if (Btn) 
			Btn->OnButtonClicked.RemoveDynamic(this, &UPage_PropSpawn::OnCategoryButtonClicked);
	}

	if (PresetManager.IsValid())
		PresetManager->OnPresetsChanged.RemoveDynamic(this, &UPage_PropSpawn::OnPresetsChanged);

	Super::NativeDestruct();
}

void UPage_PropSpawn::InitPage(UCameraPlacementSystem* PlacementSys, UUnitPatrolComponent* PatrolComp, UUnitSelectionComponent* SelComp)
{
	Super::InitPage(PlacementSys, PatrolComp, SelComp);

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			if (UPresetManagerComponent* PMC = Pawn->FindComponentByClass<UPresetManagerComponent>())
			{
				PresetManager = PMC;
				if (PresetManager.IsValid())
				{
					PresetManager->OnPresetsChanged.RemoveDynamic(this, &UPage_PropSpawn::OnPresetsChanged);
					PresetManager->OnPresetsChanged.AddDynamic(this, &UPage_PropSpawn::OnPresetsChanged);
				}
			}
		}
	}

	SetupItemsList();
	SetupPresetList();
	
	if (Anim_SavePreset)
	{
		PlayAnimation(Anim_SavePreset, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
		SetAnimationCurrentTime(Anim_SavePreset, Anim_SavePreset->GetEndTime());
	}
}

void UPage_PropSpawn::OnPageOpened()
{
	Super::OnPageOpened();
	SetupPresetList();
}

// ------------------------------------------------------------------
// Items Catalogue
// ------------------------------------------------------------------

void UPage_PropSpawn::SetupItemsList()
{
	if (!WrapBox_Items)
		return;

	if (!PropsEntryClass)
	{
		UE_LOG(LogTemp, Error, TEXT("SetupItemsList: PropsEntryClass is NULL"));
		return;
	}
	
	WrapBox_Items->ClearChildren();
	EntryList.Reset();
	CachedCategoryTags.Reset();

	for (UPlacementItemData* Data : PlacementProps)
	{
		if (!Data)
			continue;

		UPropsEntryWidget* Widget = CreateWidget<UPropsEntryWidget>(GetWorld(), PropsEntryClass);
		if (!Widget)
		{
			UE_LOG(LogTemp, Error, TEXT("SetupItemsList: Failed to create widget for %s"), *Data->GetName());
			continue;
		}

		Widget->InitEntry(Data);
		Widget->SetPlacementSystem(PlacementSystem.Get());
		WrapBox_Items->AddChild(Widget);
		EntryList.Add(Widget);

		if (Widget->ItemButton)
			Widget->ItemButton->OnButtonClicked.AddDynamic(this, &UPage_PropSpawn::OnItemSelected);

		for (const FName& Tag : Widget->GetItemTags())
		{
			if (!Tag.IsNone())
				CachedCategoryTags.AddUnique(Tag);
		}
	}

	SetupCategoryButtons();
	ApplyFilters();
}

void UPage_PropSpawn::SetupCategoryButtons()
{
	if (!CategoryWrapBox || !CategoryButtonClass)
		return;

	for (UCustomButtonWidget* Btn : CategoryButtons)
	{
		if (Btn) 
			Btn->OnButtonClicked.RemoveDynamic(this, &UPage_PropSpawn::OnCategoryButtonClicked);
	}

	CategoryButtons.Reset();
	CategoryButtonTagMap.Reset();
	CategoryWrapBox->ClearChildren();

	auto CreateBtn = [&](const FText& Label, FName Tag)
	{
		UCustomButtonWidget* Btn = CreateWidget<UCustomButtonWidget>(GetWorld(), CategoryButtonClass);
		if (!Btn) 
			return;

		Btn->SetButtonText(Label);
		Btn->ButtonIndex = CategoryButtons.Num();
		Btn->OnButtonClicked.AddDynamic(this, &UPage_PropSpawn::OnCategoryButtonClicked);

		CategoryWrapBox->AddChild(Btn);
		CategoryButtons.Add(Btn);
		CategoryButtonTagMap.Add(Btn, Tag);
	};

	CreateBtn(LOCTEXT("Cat_All", "All"), NAME_None);

	TArray<FName> SortedTags = CachedCategoryTags;
	SortedTags.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });

	for (const FName& Tag : SortedTags)
	{
		CreateBtn(FText::FromName(Tag), Tag);
	}

	if (CategoryButtons.Num() > 0)
		UpdateCategoryButtonSelection(CategoryButtons[0]);
}

void UPage_PropSpawn::UpdateCategoryButtonSelection(UCustomButtonWidget* SelectedButton)
{
	for (UCustomButtonWidget* Btn : CategoryButtons)
	{
		if (Btn)
			Btn->ToggleButtonIsSelected(Btn == SelectedButton);
	}
}

void UPage_PropSpawn::ApplyFilters()
{
	const bool bFilterByTag = !CurrentTagFilter.IsNone();

	for (UPropsEntryWidget* Entry : EntryList)
	{
		if (!Entry)
			continue;

		const bool bMatchesSearch = CurrentSearchText.IsEmpty() || Entry->MatchesSearch(CurrentSearchText);
		const bool bMatchesTag = !bFilterByTag || Entry->HasTag(CurrentTagFilter);

		const bool bVisible = (bMatchesSearch && bMatchesTag);
		Entry->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

		UE_LOG(LogTemp, Log, TEXT("ApplyFilters: Item '%s' -> Search=%d Tag=%d -> Visible=%d"), 
			*Entry->GetDisplayName().ToString(), bMatchesSearch, bMatchesTag, bVisible);
	}
}

// ------------------------------------------------------------------
// Item Callbacks
// ------------------------------------------------------------------

void UPage_PropSpawn::OnItemSelected(UCustomButtonWidget* Button, int Index)
{
	for (UPropsEntryWidget* Entry : EntryList)
	{
		if (Entry && Entry->ItemButton)
			Entry->ItemButton->ToggleButtonIsSelected(false);
	}
}

void UPage_PropSpawn::OnCategoryButtonClicked(UCustomButtonWidget* Button, int Index)
{
	if (FName* Tag = CategoryButtonTagMap.Find(Button))
	{
		CurrentTagFilter = *Tag;
		UpdateCategoryButtonSelection(Button);
		ApplyFilters();
	}
}

void UPage_PropSpawn::OnSearchTextChanged(const FText& Text)
{
	CurrentSearchText = Text.ToString();
	CurrentSearchText.TrimStartAndEndInline();
	CurrentSearchText.ToLowerInline();
	ApplyFilters();
}

// ------------------------------------------------------------------
// Presets
// ------------------------------------------------------------------

void UPage_PropSpawn::SetupPresetList()
{
	if (!ScrollBox_Presets || !PresetEntryClass)
		return;

	for (UPresetEntryWidget* PW : PresetWidgets)
	{
		if (!PW) 
			continue;

		PW->OnButtonClicked.RemoveDynamic(this, &UPage_PropSpawn::OnPresetSpawnClicked);

		if (PW->Btn_Delete)
			PW->Btn_Delete->OnButtonClicked.RemoveDynamic(this, &UPage_PropSpawn::OnPresetDeleteClicked);
	}

	ScrollBox_Presets->ClearChildren();
	PresetWidgets.Reset();

	if (!PresetManager.IsValid())
		return;

	int32 Idx = 0;
	for (const FPlacementPreset& Preset : PresetManager->GetAllPresets())
	{
		UPresetEntryWidget* PW = CreateWidget<UPresetEntryWidget>(GetWorld(), PresetEntryClass);
		if (!PW)
			continue;

		PW->InitFromPreset(Preset);
		ScrollBox_Presets->AddChild(PW);
		PresetWidgets.Add(PW);

		PW->ButtonIndex = Idx;
		PW->OnButtonClicked.AddDynamic(this, &UPage_PropSpawn::OnPresetSpawnClicked);

		if (PW->Btn_Delete)
		{
			PW->Btn_Delete->ButtonIndex = Idx;
			PW->Btn_Delete->OnButtonClicked.AddDynamic(this, &UPage_PropSpawn::OnPresetDeleteClicked);
		}

		++Idx;
	}
}

void UPage_PropSpawn::OnOpenPresetClicked(UCustomButtonWidget* Button, int Index)
{
	if (bPresetIsOpen)
	{
		PlayAnimation(Anim_SavePreset, 0.0f, 1, EUMGSequencePlayMode::Reverse);
		Btn_OpenPreset->SetButtonText(FText::FromString("Save Preset"));
		bPresetIsOpen = false;
	}
	else
	{
		PresetContainer->SetVisibility(ESlateVisibility::Visible);
		
		PlayAnimation(Anim_SavePreset, 0.0f, 1, EUMGSequencePlayMode::Forward);
		Btn_OpenPreset->SetButtonText(FText::FromString("Cancel Preset"));
		bPresetIsOpen = true;
	}
}

void UPage_PropSpawn::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	Super::OnAnimationFinished_Implementation(Animation);

	if (Animation == Anim_SavePreset)
	{
		if (!bPresetIsOpen && PresetContainer)
		{
			PresetContainer->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UPage_PropSpawn::OnSavePresetClicked(UCustomButtonWidget* Button, int Index)
{
	if (!PresetManager.IsValid() || !SelectionComponent.IsValid())
		return;

	// Determine preset name
	FName PresetName;
	if (Input_PresetName && !Input_PresetName->GetText().IsEmpty())
	{
		PresetName = FName(*Input_PresetName->GetText().ToString());
	}
	else
	{
		PresetName = FName(TEXT("Preset"));
	}

	const TArray<AActor*> Selected = SelectionComponent->GetSelectedActors();
	if (Selected.Num() == 0)
		return;

	PresetManager->CreatePresetFromActors(Selected, PresetName);

	if (Input_PresetName)
		Input_PresetName->SetText(FText::GetEmpty());
}

void UPage_PropSpawn::OnPresetSpawnClicked(UCustomButtonWidget* Button, int Index)
{
	if (!PresetManager.IsValid() || !PlacementSystem.IsValid())
		return;

	if (!PresetWidgets.IsValidIndex(Index))
		return;

	const FGuid PresetID = PresetWidgets[Index]->GetPresetID();
	const FPlacementPreset* Preset = PresetManager->FindPreset(PresetID);
	
	if (!Preset || Preset->Entries.Num() == 0)
		return;

	UPlacementPresetData* PresetData = NewObject<UPlacementPresetData>(this, NAME_None, RF_Transient);
	PresetData->Preset = *Preset;
	PresetData->DisplayName = FText::FromName(Preset->PresetName);
	
	PlacementSystem->StartPlacement(PresetData);
}

void UPage_PropSpawn::OnPresetDeleteClicked(UCustomButtonWidget* Button, int Index)
{
	if (!PresetManager.IsValid())
		return;

	if (!PresetWidgets.IsValidIndex(Index))
		return;

	PresetManager->DeletePreset(PresetWidgets[Index]->GetPresetID());
}

void UPage_PropSpawn::OnPresetsChanged()
{
	SetupPresetList();
}

#undef LOCTEXT_NAMESPACE
