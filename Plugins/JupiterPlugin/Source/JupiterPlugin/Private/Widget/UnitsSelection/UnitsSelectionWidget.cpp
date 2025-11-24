#include "Widget/UnitsSelection/UnitsSelectionWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/WrapBox.h"
#include "Kismet/GameplayStatics.h"
#include "Components/UnitSpawnComponent.h"
#include "Widget/UnitsSelection/UnitSpawnCountWidget.h"
#include "Widget/UnitsSelection/UnitSpawnFormationWidget.h"
#include "Widget/UnitsSelection/UnitSpawnAxisWidget.h"
#include "Widget/UnitsSelection/UnitsEntryWidget.h"
#include "Widget/CustomButtonWidget.h"
#include "Data/UnitsSelectionDataAsset.h"
#include "Algo/Sort.h"
#include "Components/WidgetSwitcher.h"
#include "Widget/Patrol/PatrolEditorWidget.h"

#define LOCTEXT_NAMESPACE "UnitsSelectionWidget"

void UUnitsSelectionWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        if (APawn* PlayerPawn = PlayerController->GetPawn())
            SpawnComponent = PlayerPawn->FindComponentByClass<UUnitSpawnComponent>();
    }

    if (SpawnCountWidget)
        SpawnCountWidget->SetupWithComponent(SpawnComponent);

    if (SpawnFormationWidget)
        SpawnFormationWidget->SetupWithComponent(SpawnComponent);

    if (SpawnAxisWidget)
        SpawnAxisWidget->SetupWithComponent(SpawnComponent);

    InitializeSpawnOptionWidgets();

    if (Btn_ShowUnitsSelection)
        Btn_ShowUnitsSelection->OnPressed.AddDynamic(this, &UUnitsSelectionWidget::OnShowUnitSelectionPressed);

    if (SearchTextBox)
    {
        SearchTextBox->OnTextChanged.AddDynamic(this, &UUnitsSelectionWidget::OnSearchTextChanged);

        CurrentSearchText = SearchTextBox->GetText().ToString();
        CurrentSearchText.TrimStartAndEndInline();
        CurrentSearchText.ToLowerInline();
    }

    if (Btn_OpenSpawnPage)
        Btn_OpenSpawnPage->OnButtonClicked.AddDynamic(this, &UUnitsSelectionWidget::OnOpenSpawnPage);

    if (Btn_OpenPatrolPage)
        Btn_OpenPatrolPage->OnButtonClicked.AddDynamic(this, &UUnitsSelectionWidget::OnOpenPatrolPage);

    // Default to spawn page
    OnOpenSpawnPage(Btn_OpenSpawnPage, 0);

    OnShowUnitSelectionPressed();
    SetupUnitsList();
}

void UUnitsSelectionWidget::OnOpenSpawnPage(UCustomButtonWidget* Button, int Index)
{
    if (ContentSwitcher)
    {
        ContentSwitcher->SetActiveWidgetIndex(0);
    }
    
    if (Btn_OpenSpawnPage)
        Btn_OpenSpawnPage->ToggleButtonIsSelected(true);
        
    if (Btn_OpenPatrolPage)
        Btn_OpenPatrolPage->ToggleButtonIsSelected(false);
}

void UUnitsSelectionWidget::OnOpenPatrolPage(UCustomButtonWidget* Button, int Index)
{
    if (ContentSwitcher)
    {
        ContentSwitcher->SetActiveWidgetIndex(1);
    }
    
    if (Btn_OpenSpawnPage)
        Btn_OpenSpawnPage->ToggleButtonIsSelected(false);
        
    if (Btn_OpenPatrolPage)
        Btn_OpenPatrolPage->ToggleButtonIsSelected(true);
    
    if (PatrolEditorPage)
    {
        PatrolEditorPage->SetupGlobal();
    }
}

void UUnitsSelectionWidget::NativeDestruct()
{
    if (Btn_ShowUnitsSelection)
        Btn_ShowUnitsSelection->OnPressed.RemoveDynamic(this, &UUnitsSelectionWidget::OnShowUnitSelectionPressed);

    if (CountOptionButton)
        CountOptionButton->OnButtonClicked.RemoveDynamic(this, &UUnitsSelectionWidget::OnCountOptionSelected);

    if (FormationOptionButton)
        FormationOptionButton->OnButtonClicked.RemoveDynamic(this, &UUnitsSelectionWidget::OnFormationOptionSelected);

    if (SearchTextBox)
        SearchTextBox->OnTextChanged.RemoveDynamic(this, &UUnitsSelectionWidget::OnSearchTextChanged);

    for (UCustomButtonWidget* CategoryButton : CategoryButtons)
    {
        if (CategoryButton)
            CategoryButton->OnButtonClicked.RemoveDynamic(this, &UUnitsSelectionWidget::OnCategoryButtonClicked);
    }

    CategoryButtons.Reset();
    CategoryButtonTagMap.Reset();

    Super::NativeDestruct();
}

// --------------------------------------------------
// ----------- LISTE ET FILTRES D’UNITÉS -------------
// --------------------------------------------------

void UUnitsSelectionWidget::SetupUnitsList()
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
        WrapBox->AddChild(UnitWidget);
        EntryList.Add(UnitWidget);

        if (UnitWidget->UnitButton)
            UnitWidget->UnitButton->OnButtonClicked.AddDynamic(this, &UUnitsSelectionWidget::OnUnitSelected);

        for (const FName& Tag : UnitWidget->GetUnitTags())
        {
            if (!Tag.IsNone())
                CachedCategoryTags.AddUnique(Tag);
        }
    }

    SetupCategoryButtons();
    ApplyFilters();
}

void UUnitsSelectionWidget::InitializeSpawnOptionWidgets()
{
    if (CountOptionButton)
        CountOptionButton->OnButtonClicked.AddDynamic(this, &UUnitsSelectionWidget::OnCountOptionSelected);

    if (FormationOptionButton)
        FormationOptionButton->OnButtonClicked.AddDynamic(this, &UUnitsSelectionWidget::OnFormationOptionSelected);

    if (CountOptionButton)
    {
        OnCountOptionSelected(CountOptionButton, CountOptionButton->ButtonIndex);
    }
    else if (FormationOptionButton)
    {
        OnFormationOptionSelected(FormationOptionButton, FormationOptionButton->ButtonIndex);
    }
    else
    {
        UpdateSpawnOptionVisibility(true);
    }
}

void UUnitsSelectionWidget::OnShowUnitSelectionPressed()
{
    if (!ListBorder)
        return;

    ListBorder->SetVisibility(
        ListBorder->GetVisibility() == ESlateVisibility::Visible
            ? ESlateVisibility::Collapsed
            : ESlateVisibility::Visible);
}

void UUnitsSelectionWidget::OnUnitSelected(UCustomButtonWidget* Button, int Index)
{
    for (UUnitsEntryWidget* EntryWidget : EntryList)
    {
        if (EntryWidget && EntryWidget->UnitButton)
            EntryWidget->UnitButton->ToggleButtonIsSelected(false);
    }

    if (Button)
        Button->ToggleButtonIsSelected(true);
}

void UUnitsSelectionWidget::SetupCategoryButtons()
{
    if (!CategoryWrapBox || !CategoryButtonClass)
    {
        CategoryButtons.Reset();
        CategoryButtonTagMap.Reset();
        return;
    }

    for (UCustomButtonWidget* CategoryButton : CategoryButtons)
    {
        if (CategoryButton)
            CategoryButton->OnButtonClicked.RemoveDynamic(this, &UUnitsSelectionWidget::OnCategoryButtonClicked);
    }

    CategoryButtons.Reset();
    CategoryButtonTagMap.Reset();

    CategoryWrapBox->ClearChildren();

    auto CreateCategoryButton = [this](const FText& Label, FName Tag)
    {
        UCustomButtonWidget* CategoryButton = CreateWidget<UCustomButtonWidget>(GetWorld(), CategoryButtonClass);
        if (!CategoryButton)
            return;

        CategoryButton->SetButtonText(Label);
        CategoryButton->ButtonIndex = CategoryButtons.Num();
        CategoryButton->OnButtonClicked.AddDynamic(this, &UUnitsSelectionWidget::OnCategoryButtonClicked);

        CategoryWrapBox->AddChild(CategoryButton);
        CategoryButtons.Add(CategoryButton);
        CategoryButtonTagMap.Add(CategoryButton, Tag);
    };

    CreateCategoryButton(LOCTEXT("UnitsSelection_CategoryAll", "All"), NAME_None);

    TArray<FName> SortedTags = CachedCategoryTags;
    SortedTags.Sort([](const FName& A, const FName& B)
    {
        return A.LexicalLess(B);
    });

    for (const FName& Tag : SortedTags)
    {
        CreateCategoryButton(FText::FromName(Tag), Tag);
    }

    UCustomButtonWidget* ButtonToSelect = nullptr;

    if (CurrentTagFilter.IsNone() && CategoryButtons.Num() > 0)
    {
        ButtonToSelect = CategoryButtons[0];
    }
    else
    {
        for (UCustomButtonWidget* CategoryButton : CategoryButtons)
        {
            if (const FName* FoundTag = CategoryButtonTagMap.Find(CategoryButton))
            {
                if (*FoundTag == CurrentTagFilter)
                {
                    ButtonToSelect = CategoryButton;
                    break;
                }
            }
        }
    }

    if (!ButtonToSelect && CategoryButtons.Num() > 0)
    {
        ButtonToSelect = CategoryButtons[0];
        CurrentTagFilter = NAME_None;
    }

    if (ButtonToSelect)
        UpdateCategoryButtonSelection(ButtonToSelect);
}

void UUnitsSelectionWidget::OnCategoryButtonClicked(UCustomButtonWidget* Button, int /*Index*/)
{
    if (!Button)
        return;

    if (const FName* FoundTag = CategoryButtonTagMap.Find(Button))
    {
        CurrentTagFilter = *FoundTag;
        UpdateCategoryButtonSelection(Button);
        ApplyFilters();
    }
}

void UUnitsSelectionWidget::OnSearchTextChanged(const FText& Text)
{
    CurrentSearchText = Text.ToString();
    CurrentSearchText.TrimStartAndEndInline();
    CurrentSearchText.ToLowerInline();

    ApplyFilters();
}

void UUnitsSelectionWidget::UpdateCategoryButtonSelection(UCustomButtonWidget* SelectedButton)
{
    for (UCustomButtonWidget* CategoryButton : CategoryButtons)
    {
        if (CategoryButton)
            CategoryButton->ToggleButtonIsSelected(CategoryButton == SelectedButton);
    }
}

void UUnitsSelectionWidget::ApplyFilters()
{
    const bool bFilterByTag = !CurrentTagFilter.IsNone();

    for (UUnitsEntryWidget* EntryWidget : EntryList)
    {
        if (!EntryWidget)
            continue;

        const bool bMatchesSearch = CurrentSearchText.IsEmpty() || EntryWidget->MatchesSearch(CurrentSearchText);
        const bool bMatchesTag = !bFilterByTag || EntryWidget->HasTag(CurrentTagFilter);

        const ESlateVisibility TargetVisibility =
            (bMatchesSearch && bMatchesTag)
                ? ESlateVisibility::Visible
                : ESlateVisibility::Collapsed;

        EntryWidget->SetVisibility(TargetVisibility);
    }
}

void UUnitsSelectionWidget::OnCountOptionSelected(UCustomButtonWidget* Button, int /*Index*/)
{
    UpdateSpawnOptionButtons(Button);
    UpdateSpawnOptionVisibility(true);
}

void UUnitsSelectionWidget::OnFormationOptionSelected(UCustomButtonWidget* Button, int /*Index*/)
{
    UpdateSpawnOptionButtons(Button);
    UpdateSpawnOptionVisibility(false);
}

void UUnitsSelectionWidget::UpdateSpawnOptionButtons(UCustomButtonWidget* SelectedButton)
{
    if (CountOptionButton)
        CountOptionButton->ToggleButtonIsSelected(CountOptionButton == SelectedButton);

    if (FormationOptionButton)
        FormationOptionButton->ToggleButtonIsSelected(FormationOptionButton == SelectedButton);
}

void UUnitsSelectionWidget::UpdateSpawnOptionVisibility(bool bShowCountOptions)
{
    const ESlateVisibility CountVisibility = bShowCountOptions ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
    const ESlateVisibility FormationVisibility = bShowCountOptions ? ESlateVisibility::Collapsed : ESlateVisibility::Visible;

    if (SpawnCountWidget)
        SpawnCountWidget->SetVisibility(CountVisibility);

    if (SpawnFormationWidget)
        SpawnFormationWidget->SetVisibility(FormationVisibility);

    if (SpawnAxisWidget)
        SpawnAxisWidget->SetVisibility(FormationVisibility);
}

#undef LOCTEXT_NAMESPACE
