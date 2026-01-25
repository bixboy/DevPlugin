#include "UI/Editor/Pages/Page_UnitSpawn.h"
#include "Components/Unit/UnitSpawnComponent.h"
#include "UI/Editor/Widgets/Spawn/UnitSpawnCountWidget.h"
#include "UI/Editor/Widgets/Spawn/UnitSpawnFormationWidget.h"
#include "UI/Editor/Widgets/Spawn/UnitSpawnAxisWidget.h"
#include "UI/Editor/Widgets/Spawn/UnitsEntryWidget.h"
#include "UI/CustomButtonWidget.h"
#include "Data/UnitsSelectionDataAsset.h"
#include "Animation/WidgetAnimation.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/WrapBox.h"

#define LOCTEXT_NAMESPACE "Page_UnitSpawn"

void UPage_UnitSpawn::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Btn_ShowUnitsSelection)
		Btn_ShowUnitsSelection->OnPressed.AddDynamic(this, &UPage_UnitSpawn::OnShowUnitSelectionPressed);

	if (SearchTextBox)
		SearchTextBox->OnTextChanged.AddDynamic(this, &UPage_UnitSpawn::OnSearchTextChanged);

	if (InspectorToggleButton)
		InspectorToggleButton->OnButtonClicked.AddDynamic(this, &UPage_UnitSpawn::OnInspectorToggleClicked);
}

void UPage_UnitSpawn::NativeDestruct()
{
	if (Btn_ShowUnitsSelection)
		Btn_ShowUnitsSelection->OnPressed.RemoveDynamic(this, &UPage_UnitSpawn::OnShowUnitSelectionPressed);

	if (SearchTextBox)
		SearchTextBox->OnTextChanged.RemoveDynamic(this, &UPage_UnitSpawn::OnSearchTextChanged);

	if (InspectorToggleButton)
		InspectorToggleButton->OnButtonClicked.RemoveDynamic(this, &UPage_UnitSpawn::OnInspectorToggleClicked);

	for (UCustomButtonWidget* Btn : CategoryButtons)
	{
		if (Btn) Btn->OnButtonClicked.RemoveDynamic(this, &UPage_UnitSpawn::OnCategoryButtonClicked);
	}

	Super::NativeDestruct();
}

void UPage_UnitSpawn::InitPage(UUnitSpawnComponent* SpawnComp, UUnitPatrolComponent* PatrolComp, UUnitSelectionComponent* SelComp)
{
	Super::InitPage(SpawnComp, PatrolComp, SelComp);

	if (SpawnCountWidget)
		SpawnCountWidget->SetupWithComponent(SpawnComp);
	
	if (SpawnFormationWidget)
		SpawnFormationWidget->SetupWithComponent(SpawnComp);
	
	if (SpawnAxisWidget)
		SpawnAxisWidget->SetupWithComponent(SpawnComp);

	SetupUnitsList();
    InitializeInspectorWidgets();
}

void UPage_UnitSpawn::OnPageOpened()
{
	Super::OnPageOpened();
}

void UPage_UnitSpawn::SetupUnitsList()
{
    if (!WrapBox)
    	return;

    WrapBox->ClearChildren();
    EntryList.Reset();
    CachedCategoryTags.Reset();

    for (UUnitsSelectionDataAsset* Data : UnitsSelectionDataAssets)
    {
        if (!Data)
        	continue;

        UUnitsEntryWidget* UnitWidget = CreateWidget<UUnitsEntryWidget>(GetWorld(), UnitsEntryClass);
        if (!UnitWidget)
        	continue;

        UnitWidget->InitEntry(Data);
        UnitWidget->SetSpawnComponent(SpawnComponent);
        WrapBox->AddChild(UnitWidget);
        EntryList.Add(UnitWidget);

        if (UnitWidget->UnitButton)
            UnitWidget->UnitButton->OnButtonClicked.AddDynamic(this, &UPage_UnitSpawn::OnUnitSelected);

        for (const FName& Tag : UnitWidget->GetUnitTags())
        {
            if (!Tag.IsNone())
                CachedCategoryTags.AddUnique(Tag);
        }
    }

    SetupCategoryButtons();
    ApplyFilters();
}

void UPage_UnitSpawn::SetupCategoryButtons()
{
    if (!CategoryWrapBox || !CategoryButtonClass)
    	return;

    // Cleanup old bindings
    for (UCustomButtonWidget* Btn : CategoryButtons)
    {
        if (Btn)
        	Btn->OnButtonClicked.RemoveDynamic(this, &UPage_UnitSpawn::OnCategoryButtonClicked);
    }
	
    CategoryButtons.Reset();
    CategoryButtonTagMap.Reset();
    CategoryWrapBox->ClearChildren();

    auto CreateBtn = [&](const FText& Label, FName Tag)
    {
        UCustomButtonWidget* Btn = CreateWidget<UCustomButtonWidget>(GetWorld(), CategoryButtonClass);
        if (!Btn) return;

        Btn->SetButtonText(Label);
        Btn->ButtonIndex = CategoryButtons.Num();
        Btn->OnButtonClicked.AddDynamic(this, &UPage_UnitSpawn::OnCategoryButtonClicked);
        
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

    // Default Selection
    if (CategoryButtons.Num() > 0)
    {
        UpdateCategoryButtonSelection(CategoryButtons[0]);
    }
}

void UPage_UnitSpawn::InitializeInspectorWidgets()
{
	if (SpawnCountWidget)
		SpawnCountWidget->SetVisibility(ESlateVisibility::Visible);
	
	if (SpawnFormationWidget)
		SpawnFormationWidget->SetVisibility(ESlateVisibility::Visible);
	
	if (SpawnAxisWidget)
		SpawnAxisWidget->SetVisibility(ESlateVisibility::Visible);

	bIsInspectorOpen = true;
	if (Anim_ToggleInspector)
	{
		PlayAnimation(Anim_ToggleInspector, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
		SetAnimationCurrentTime(Anim_ToggleInspector, Anim_ToggleInspector->GetEndTime());
	}
}

void UPage_UnitSpawn::OnUnitSelected(UCustomButtonWidget* Button, int Index)
{
    for (UUnitsEntryWidget* Entry : EntryList)
    {
        if (Entry && Entry->UnitButton)
            Entry->UnitButton->ToggleButtonIsSelected(false);
    }

    if (Button) Button->ToggleButtonIsSelected(true);
}

void UPage_UnitSpawn::OnCategoryButtonClicked(UCustomButtonWidget* Button, int Index)
{
    if (FName* Tag = CategoryButtonTagMap.Find(Button))
    {
        CurrentTagFilter = *Tag;
        UpdateCategoryButtonSelection(Button);
        ApplyFilters();
    }
}

void UPage_UnitSpawn::UpdateCategoryButtonSelection(UCustomButtonWidget* SelectedButton)
{
    for (UCustomButtonWidget* Btn : CategoryButtons)
    {
        if (Btn) Btn->ToggleButtonIsSelected(Btn == SelectedButton);
    }
}

void UPage_UnitSpawn::OnSearchTextChanged(const FText& Text)
{
    CurrentSearchText = Text.ToString();
    CurrentSearchText.TrimStartAndEndInline();
    CurrentSearchText.ToLowerInline();
    ApplyFilters();
}

void UPage_UnitSpawn::ApplyFilters()
{
    const bool bFilterByTag = !CurrentTagFilter.IsNone();

    for (UUnitsEntryWidget* Entry : EntryList)
    {
        if (!Entry) continue;

        const bool bMatchesSearch = CurrentSearchText.IsEmpty() || Entry->MatchesSearch(CurrentSearchText);
        const bool bMatchesTag = !bFilterByTag || Entry->HasTag(CurrentTagFilter);

        Entry->SetVisibility((bMatchesSearch && bMatchesTag) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

void UPage_UnitSpawn::OnShowUnitSelectionPressed()
{
    if (ListBorder)
    {
        ListBorder->SetVisibility(ListBorder->GetVisibility() == ESlateVisibility::Visible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }
}

void UPage_UnitSpawn::OnInspectorToggleClicked(UCustomButtonWidget* Button, int Index)
{
	if (!Anim_ToggleInspector || !InspectorContainer)
		return;

	if (bIsInspectorOpen)
	{
		// Fermer
		PlayAnimation(Anim_ToggleInspector, 0.0f, 1, EUMGSequencePlayMode::Reverse);
		bIsInspectorOpen = false;
	}
	else
	{
		// Ouvrir
		InspectorContainer->SetVisibility(ESlateVisibility::Visible);
		
		PlayAnimation(Anim_ToggleInspector, 0.0f, 1, EUMGSequencePlayMode::Forward);
		bIsInspectorOpen = true;
	}

	// Pivoter l'icone du bouton
	if (InspectorToggleButton)
	{
		InspectorToggleButton->SetRenderTransformAngle(bIsInspectorOpen ? 0.0f : 180.0f);
	}
}

void UPage_UnitSpawn::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	Super::OnAnimationFinished_Implementation(Animation);

	if (Animation == Anim_ToggleInspector)
	{
		if (!bIsInspectorOpen && InspectorContainer)
		{
			InspectorContainer->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

#undef LOCTEXT_NAMESPACE
