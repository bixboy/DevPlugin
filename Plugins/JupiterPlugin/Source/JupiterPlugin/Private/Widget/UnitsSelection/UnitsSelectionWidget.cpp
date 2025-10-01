#include "Widget/UnitsSelection/UnitsSelectionWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/UnitSpawnComponent.h"
#include "Components/WrapBox.h"
#include "Data/UnitsSelectionDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Widget/CustomButtonWidget.h"
#include "Widget/UnitsSelection/UnitsEntryWidget.h"
#include "Algo/Sort.h"

#define LOCTEXT_NAMESPACE "UnitsSelectionWidget"


void UUnitsSelectionWidget::NativeOnInitialized()
{
        Super::NativeOnInitialized();

        if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
        {
                if (APawn* PlayerPawn = PlayerController->GetPawn())
                        SpawnComponent = PlayerPawn->FindComponentByClass<UUnitSpawnComponent>();
        }

        if (SpawnComponent)
        {
                CachedSpawnCount = FMath::Max(1, SpawnComponent->GetUnitsPerSpawn());
                SpawnComponent->OnSpawnCountChanged.AddDynamic(this, &UUnitsSelectionWidget::HandleSpawnCountChanged);
        }
        else
        {
                CachedSpawnCount = 1;
        }

        if (SpawnCountTextBox)
        {
                SpawnCountTextBox->OnTextCommitted.AddDynamic(this, &UUnitsSelectionWidget::OnSpawnCountTextCommitted);
        }

    if (SearchTextBox)
    {
        SearchTextBox->OnTextChanged.AddDynamic(this, &UUnitsSelectionWidget::OnSearchTextChanged);

        CurrentSearchText = SearchTextBox->GetText().ToString();
        CurrentSearchText.TrimStartAndEndInline();
        CurrentSearchText.ToLowerInline();
    }

        if (Btn_IncreaseSpawnCount)
        {
                Btn_IncreaseSpawnCount->OnButtonClicked.AddDynamic(this, &UUnitsSelectionWidget::OnIncreaseSpawnCount);
        }

        if (Btn_DecreaseSpawnCount)
        {
                Btn_DecreaseSpawnCount->OnButtonClicked.AddDynamic(this, &UUnitsSelectionWidget::OnDecreaseSpawnCount);
        }

        if (Btn_ShowUnitsSelection)
                Btn_ShowUnitsSelection->OnPressed.AddDynamic(this, &UUnitsSelectionWidget::OnShowUnitSelectionPressed);

        RefreshSpawnCountDisplay();
        OnShowUnitSelectionPressed();
        SetupUnitsList();
}

void UUnitsSelectionWidget::NativeDestruct()
{
        if (SpawnComponent)
        {
                SpawnComponent->OnSpawnCountChanged.RemoveDynamic(this, &UUnitsSelectionWidget::HandleSpawnCountChanged);
        }

        if (SpawnCountTextBox)
        {
                SpawnCountTextBox->OnTextCommitted.RemoveDynamic(this, &UUnitsSelectionWidget::OnSpawnCountTextCommitted);
        }

    if (SearchTextBox)
    {
        SearchTextBox->OnTextChanged.RemoveDynamic(this, &UUnitsSelectionWidget::OnSearchTextChanged);
    }

        if (Btn_IncreaseSpawnCount)
        {
                Btn_IncreaseSpawnCount->OnButtonClicked.RemoveDynamic(this, &UUnitsSelectionWidget::OnIncreaseSpawnCount);
        }

        if (Btn_DecreaseSpawnCount)
        {
                Btn_DecreaseSpawnCount->OnButtonClicked.RemoveDynamic(this, &UUnitsSelectionWidget::OnDecreaseSpawnCount);
        }

        if (Btn_ShowUnitsSelection)
        {
                Btn_ShowUnitsSelection->OnPressed.RemoveDynamic(this, &UUnitsSelectionWidget::OnShowUnitSelectionPressed);
        }

    for (UCustomButtonWidget* CategoryButton : CategoryButtons)
    {
        if (CategoryButton)
        {
            CategoryButton->OnButtonClicked.RemoveDynamic(this, &UUnitsSelectionWidget::OnCategoryButtonClicked);
        }
    }
    CategoryButtons.Reset();
    CategoryButtonTagMap.Reset();

        Super::NativeDestruct();
}

void UUnitsSelectionWidget::SetupUnitsList()
{
    if (!WrapBox)
    {
        return;
    }

    WrapBox->ClearChildren();
    EntryList.Reset();
    CachedCategoryTags.Reset();

    for (UUnitsSelectionDataAsset* Data : UnitsSelectionDataAssets)
    {
        if (!Data)
        {
            continue;
        }

        UUnitsEntryWidget* UnitWidget = CreateWidget<UUnitsEntryWidget>(GetWorld(), UnitsEntryClass);
        if (!UnitWidget)
        {
            continue;
        }

        UnitWidget->InitEntry(Data);
        WrapBox->AddChild(UnitWidget);
        EntryList.Add(UnitWidget);

        if (UnitWidget->UnitButton)
        {
            UnitWidget->UnitButton->OnButtonClicked.AddDynamic(this, &UUnitsSelectionWidget::OnUnitSelected);
        }

        for (const FName& Tag : UnitWidget->GetUnitTags())
        {
            if (!Tag.IsNone())
            {
                CachedCategoryTags.AddUnique(Tag);
            }
        }
    }

    SetupCategoryButtons();
    ApplyFilters();
}

void UUnitsSelectionWidget::OnShowUnitSelectionPressed()
{
    if (!ListBorder)
    {
        return;
    }

    if (ListBorder->GetVisibility() == ESlateVisibility::Visible)
    {
        ListBorder->SetVisibility(ESlateVisibility::Collapsed);
    }
    else
    {
        ListBorder->SetVisibility(ESlateVisibility::Visible);
    }
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

void UUnitsSelectionWidget::OnCategoryButtonClicked(UCustomButtonWidget* Button, int /*Index*/)
{
    if (!Button)
    {
        return;
    }

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

void UUnitsSelectionWidget::OnSpawnCountTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
        int32 ParsedValue = CachedSpawnCount;
        const FString TextString = Text.ToString();
        
        if (!TextString.IsEmpty())
                ParsedValue = FCString::Atoi(*TextString);

        ApplySpawnCount(ParsedValue);
}

void UUnitsSelectionWidget::OnIncreaseSpawnCount(UCustomButtonWidget* /*Button*/, int /*Index*/)
{
        ApplySpawnCount(CachedSpawnCount + 1);
}

void UUnitsSelectionWidget::OnDecreaseSpawnCount(UCustomButtonWidget* /*Button*/, int /*Index*/)
{
        ApplySpawnCount(CachedSpawnCount - 1);
}

void UUnitsSelectionWidget::HandleSpawnCountChanged(int32 NewCount)
{
        CachedSpawnCount = FMath::Max(1, NewCount);
        RefreshSpawnCountDisplay();
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
        {
            CategoryButton->OnButtonClicked.RemoveDynamic(this, &UUnitsSelectionWidget::OnCategoryButtonClicked);
        }
    }

    CategoryButtons.Reset();
    CategoryButtonTagMap.Reset();

    CategoryWrapBox->ClearChildren();

    auto CreateCategoryButton = [this](const FText& Label, FName Tag)
    {
        UCustomButtonWidget* CategoryButton = CreateWidget<UCustomButtonWidget>(GetWorld(), CategoryButtonClass);
        if (!CategoryButton)
        {
            return;
        }

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
    {
        UpdateCategoryButtonSelection(ButtonToSelect);
    }
}

void UUnitsSelectionWidget::UpdateCategoryButtonSelection(UCustomButtonWidget* SelectedButton)
{
    for (UCustomButtonWidget* CategoryButton : CategoryButtons)
    {
        if (CategoryButton)
        {
            CategoryButton->ToggleButtonIsSelected(CategoryButton == SelectedButton);
        }
    }
}

void UUnitsSelectionWidget::ApplyFilters()
{
    const bool bFilterByTag = !CurrentTagFilter.IsNone();

    for (UUnitsEntryWidget* EntryWidget : EntryList)
    {
        if (!EntryWidget)
        {
            continue;
        }

        const bool bMatchesSearch = CurrentSearchText.IsEmpty() || EntryWidget->MatchesSearch(CurrentSearchText);
        const bool bMatchesTag = !bFilterByTag || EntryWidget->HasTag(CurrentTagFilter);

        const ESlateVisibility TargetVisibility = (bMatchesSearch && bMatchesTag) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
        EntryWidget->SetVisibility(TargetVisibility);
    }
}

void UUnitsSelectionWidget::ApplySpawnCount(int32 NewCount)
{
        const int32 ClampedCount = FMath::Clamp(NewCount, 1, MaxSpawnCount);

        if (SpawnComponent)
                SpawnComponent->SetUnitsPerSpawn(ClampedCount);

        CachedSpawnCount = ClampedCount;
        RefreshSpawnCountDisplay();
}

void UUnitsSelectionWidget::RefreshSpawnCountDisplay() const
{
        if (SpawnCountTextBox)
                SpawnCountTextBox->SetText(FText::AsNumber(CachedSpawnCount));
}

#undef LOCTEXT_NAMESPACE
