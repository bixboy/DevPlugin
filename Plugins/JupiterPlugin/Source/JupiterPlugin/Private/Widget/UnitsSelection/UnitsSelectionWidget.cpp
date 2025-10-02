#include "Widget/UnitsSelection/UnitsSelectionWidget.h"

#include "Algo/Sort.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/UnitSpawnComponent.h"
#include "Components/Widget.h"
#include "Components/WrapBox.h"
#include "Data/UnitsSelectionDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/LexFromString.h"
#include "Types/SlateEnums.h"
#include "Widget/CustomButtonWidget.h"
#include "Widget/UnitsSelection/UnitsEntryWidget.h"

#define LOCTEXT_NAMESPACE "UnitsSelectionWidget"

namespace UnitsSelectionWidget::Private
{
template <typename DelegateType, typename UserObjectType, typename FuncType>
void AddDynamicIfValid(DelegateType& Delegate, UserObjectType* UserObject, FuncType Func)
{
    if (UserObject)
    {
        Delegate.AddDynamic(UserObject, Func);
    }
}

template <typename DelegateType, typename UserObjectType, typename FuncType>
void RemoveDynamicIfValid(DelegateType& Delegate, UserObjectType* UserObject, FuncType Func)
{
    if (UserObject)
    {
        Delegate.RemoveDynamic(UserObject, Func);
    }
}

template <typename WidgetType>
void SetVisibilityIfChanged(WidgetType* Widget, ESlateVisibility NewVisibility)
{
    if (Widget && Widget->GetVisibility() != NewVisibility)
    {
        Widget->SetVisibility(NewVisibility);
    }
}

template <typename WidgetType>
void ToggleVisibility(WidgetType* Widget, ESlateVisibility VisibleState = ESlateVisibility::Visible, ESlateVisibility HiddenState = ESlateVisibility::Collapsed)
{
    if (!Widget)
    {
        return;
    }

    const ESlateVisibility CurrentVisibility = Widget->GetVisibility();
    const ESlateVisibility TargetVisibility = (CurrentVisibility == VisibleState) ? HiddenState : VisibleState;
    SetVisibilityIfChanged(Widget, TargetVisibility);
}

void ClearCategoryButtons(TArray<UCustomButtonWidget*>& Buttons, TMap<UCustomButtonWidget*, FName>& ButtonTagMap, UUnitsSelectionWidget* Widget)
{
    for (UCustomButtonWidget* CategoryButton : Buttons)
    {
        if (CategoryButton)
        {
            RemoveDynamicIfValid(CategoryButton->OnButtonClicked, Widget, &UUnitsSelectionWidget::OnCategoryButtonClicked);
        }
    }

    Buttons.Reset();
    ButtonTagMap.Reset();
}
}

void UUnitsSelectionWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        if (APawn* PlayerPawn = PlayerController->GetPawn())
        {
            SpawnComponent = PlayerPawn->FindComponentByClass<UUnitSpawnComponent>();
        }
    }

    if (SpawnComponent)
    {
        CachedSpawnCount = FMath::Max(1, SpawnComponent->GetUnitsPerSpawn());
        UnitsSelectionWidget::Private::AddDynamicIfValid(SpawnComponent->OnSpawnCountChanged, this, &UUnitsSelectionWidget::HandleSpawnCountChanged);
        UnitsSelectionWidget::Private::AddDynamicIfValid(SpawnComponent->OnSpawnFormationChanged, this, &UUnitsSelectionWidget::HandleSpawnFormationChanged);
        UnitsSelectionWidget::Private::AddDynamicIfValid(SpawnComponent->OnCustomFormationDimensionsChanged, this, &UUnitsSelectionWidget::HandleCustomFormationDimensionsChanged);
    }
    else
    {
        CachedSpawnCount = 1;
    }

    if (SpawnCountTextBox)
    {
        UnitsSelectionWidget::Private::AddDynamicIfValid(SpawnCountTextBox->OnTextCommitted, this, &UUnitsSelectionWidget::OnSpawnCountTextCommitted);
    }

    if (SearchTextBox)
    {
        UnitsSelectionWidget::Private::AddDynamicIfValid(SearchTextBox->OnTextChanged, this, &UUnitsSelectionWidget::OnSearchTextChanged);

        FString InitialSearch = SearchTextBox->GetText().ToString();
        InitialSearch.TrimStartAndEndInline();
        InitialSearch.ToLowerInline();
        CurrentSearchText = MoveTemp(InitialSearch);
    }

    if (Btn_IncreaseSpawnCount)
    {
        UnitsSelectionWidget::Private::AddDynamicIfValid(Btn_IncreaseSpawnCount->OnButtonClicked, this, &UUnitsSelectionWidget::OnIncreaseSpawnCount);
    }

    if (Btn_DecreaseSpawnCount)
    {
        UnitsSelectionWidget::Private::AddDynamicIfValid(Btn_DecreaseSpawnCount->OnButtonClicked, this, &UUnitsSelectionWidget::OnDecreaseSpawnCount);
    }

    if (Btn_ShowUnitsSelection)
    {
        UnitsSelectionWidget::Private::AddDynamicIfValid(Btn_ShowUnitsSelection->OnPressed, this, &UUnitsSelectionWidget::OnShowUnitSelectionPressed);
    }

    if (CustomFormationXTextBox)
    {
        UnitsSelectionWidget::Private::AddDynamicIfValid(CustomFormationXTextBox->OnTextCommitted, this, &UUnitsSelectionWidget::OnCustomFormationXCommitted);
    }

    if (CustomFormationYTextBox)
    {
        UnitsSelectionWidget::Private::AddDynamicIfValid(CustomFormationYTextBox->OnTextCommitted, this, &UUnitsSelectionWidget::OnCustomFormationYCommitted);
    }

    RefreshSpawnCountDisplay();
    InitializeFormationOptions();
    UpdateFormationSelection();
    RefreshCustomFormationInputs();
    OnShowUnitSelectionPressed();
    SetupUnitsList();
}

void UUnitsSelectionWidget::NativeDestruct()
{
    if (SpawnComponent)
    {
        UnitsSelectionWidget::Private::RemoveDynamicIfValid(SpawnComponent->OnSpawnCountChanged, this, &UUnitsSelectionWidget::HandleSpawnCountChanged);
        UnitsSelectionWidget::Private::RemoveDynamicIfValid(SpawnComponent->OnSpawnFormationChanged, this, &UUnitsSelectionWidget::HandleSpawnFormationChanged);
        UnitsSelectionWidget::Private::RemoveDynamicIfValid(SpawnComponent->OnCustomFormationDimensionsChanged, this, &UUnitsSelectionWidget::HandleCustomFormationDimensionsChanged);
    }

    if (SpawnCountTextBox)
    {
        UnitsSelectionWidget::Private::RemoveDynamicIfValid(SpawnCountTextBox->OnTextCommitted, this, &UUnitsSelectionWidget::OnSpawnCountTextCommitted);
    }

    if (SearchTextBox)
    {
        UnitsSelectionWidget::Private::RemoveDynamicIfValid(SearchTextBox->OnTextChanged, this, &UUnitsSelectionWidget::OnSearchTextChanged);
    }

    if (Btn_IncreaseSpawnCount)
    {
        UnitsSelectionWidget::Private::RemoveDynamicIfValid(Btn_IncreaseSpawnCount->OnButtonClicked, this, &UUnitsSelectionWidget::OnIncreaseSpawnCount);
    }

    if (Btn_DecreaseSpawnCount)
    {
        UnitsSelectionWidget::Private::RemoveDynamicIfValid(Btn_DecreaseSpawnCount->OnButtonClicked, this, &UUnitsSelectionWidget::OnDecreaseSpawnCount);
    }

    if (Btn_ShowUnitsSelection)
    {
        UnitsSelectionWidget::Private::RemoveDynamicIfValid(Btn_ShowUnitsSelection->OnPressed, this, &UUnitsSelectionWidget::OnShowUnitSelectionPressed);
    }

    if (FormationDropdown)
    {
        UnitsSelectionWidget::Private::RemoveDynamicIfValid(FormationDropdown->OnSelectionChanged, this, &UUnitsSelectionWidget::OnFormationOptionChanged);
    }

    if (CustomFormationXTextBox)
    {
        UnitsSelectionWidget::Private::RemoveDynamicIfValid(CustomFormationXTextBox->OnTextCommitted, this, &UUnitsSelectionWidget::OnCustomFormationXCommitted);
    }

    if (CustomFormationYTextBox)
    {
        UnitsSelectionWidget::Private::RemoveDynamicIfValid(CustomFormationYTextBox->OnTextCommitted, this, &UUnitsSelectionWidget::OnCustomFormationYCommitted);
    }

    UnitsSelectionWidget::Private::ClearCategoryButtons(CategoryButtons, CategoryButtonTagMap, this);

    Super::NativeDestruct();
}

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
        {
            UnitsSelectionWidget::Private::AddDynamicIfValid(UnitWidget->UnitButton->OnButtonClicked, this, &UUnitsSelectionWidget::OnUnitSelected);
        }

        for (const FName& Tag : UnitWidget->GetUnitTags())
        {
            if (!Tag.IsNone())
                CachedCategoryTags.AddUnique(Tag);
        }
    }

    SetupCategoryButtons();
    ApplyFilters();
}

void UUnitsSelectionWidget::OnShowUnitSelectionPressed()
{
    UnitsSelectionWidget::Private::ToggleVisibility(ListBorder);
}

void UUnitsSelectionWidget::OnUnitSelected(UCustomButtonWidget* Button, int Index)
{
    const UCustomButtonWidget* SelectedButton = Button;

    for (UUnitsEntryWidget* EntryWidget : EntryList)
    {
        if (EntryWidget && EntryWidget->UnitButton)
        {
            EntryWidget->UnitButton->ToggleButtonIsSelected(EntryWidget->UnitButton == SelectedButton);
        }
    }
}

void UUnitsSelectionWidget::OnCategoryButtonClicked(UCustomButtonWidget* Button, int /*Index*/)
{
    if (!Button)
        return;

    if (const FName* FoundTag = CategoryButtonTagMap.Find(Button))
    {
        const bool bTagChanged = CurrentTagFilter != *FoundTag;
        CurrentTagFilter = *FoundTag;
        UpdateCategoryButtonSelection(Button);
        if (bTagChanged)
        {
            ApplyFilters();
        }
    }
}

void UUnitsSelectionWidget::OnSearchTextChanged(const FText& Text)
{
    FString NewSearchText = Text.ToString();
    NewSearchText.TrimStartAndEndInline();
    NewSearchText.ToLowerInline();

    if (CurrentSearchText == NewSearchText)
    {
        return;
    }

    CurrentSearchText = MoveTemp(NewSearchText);
    ApplyFilters();
}

void UUnitsSelectionWidget::OnSpawnCountTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    int32 ParsedValue = CachedSpawnCount;
    const FString TextString = Text.ToString();

    if (!TextString.IsEmpty())
    {
        if (!LexFromString(ParsedValue, *TextString))
        {
            ParsedValue = 0;
        }
    }

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

void UUnitsSelectionWidget::HandleSpawnFormationChanged(ESpawnFormation /*NewFormation*/)
{
    UpdateFormationSelection();
    RefreshCustomFormationInputs();
}

void UUnitsSelectionWidget::HandleCustomFormationDimensionsChanged(FIntPoint /*NewDimensions*/)
{
    RefreshCustomFormationInputs();
}

void UUnitsSelectionWidget::SetupCategoryButtons()
{
    if (!CategoryWrapBox || !CategoryButtonClass)
    {
        UnitsSelectionWidget::Private::ClearCategoryButtons(CategoryButtons, CategoryButtonTagMap, this);
        return;
    }

    UnitsSelectionWidget::Private::ClearCategoryButtons(CategoryButtons, CategoryButtonTagMap, this);

    CategoryWrapBox->ClearChildren();

    auto CreateCategoryButton = [this](const FText& Label, FName Tag)
    {
        UCustomButtonWidget* CategoryButton = CreateWidget<UCustomButtonWidget>(GetWorld(), CategoryButtonClass);
        if (!CategoryButton)
            return;

        CategoryButton->SetButtonText(Label);
        CategoryButton->ButtonIndex = CategoryButtons.Num();
        UnitsSelectionWidget::Private::AddDynamicIfValid(CategoryButton->OnButtonClicked, this, &UUnitsSelectionWidget::OnCategoryButtonClicked);

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

void UUnitsSelectionWidget::InitializeFormationOptions()
{
    if (!FormationDropdown)
    {
        OptionToFormation.Reset();
        FormationToOption.Reset();
        return;
    }

    FormationDropdown->ClearOptions();
    OptionToFormation.Reset();
    FormationToOption.Reset();

    if (const UEnum* EnumPtr = StaticEnum<ESpawnFormation>())
    {
        for (int32 EnumIndex = 0; EnumIndex < EnumPtr->NumEnums(); ++EnumIndex)
        {
            if (EnumPtr->HasMetaData(TEXT("Hidden"), EnumIndex))
                continue;

            const FString EnumName = EnumPtr->GetNameStringByIndex(EnumIndex);
            if (EnumName.Contains(TEXT("MAX")))
                continue;

            const ESpawnFormation FormationValue = static_cast<ESpawnFormation>(EnumPtr->GetValueByIndex(EnumIndex));
            const FString DisplayName = EnumPtr->GetDisplayNameTextByIndex(EnumIndex).ToString();

            FormationDropdown->AddOption(DisplayName);
            OptionToFormation.Add(DisplayName, FormationValue);
            FormationToOption.Add(FormationValue, DisplayName);
        }
    }

    UnitsSelectionWidget::Private::RemoveDynamicIfValid(FormationDropdown->OnSelectionChanged, this, &UUnitsSelectionWidget::OnFormationOptionChanged);
    UnitsSelectionWidget::Private::AddDynamicIfValid(FormationDropdown->OnSelectionChanged, this, &UUnitsSelectionWidget::OnFormationOptionChanged);

    if (SpawnComponent)
    {
        UpdateFormationSelection();
    }
    else if (FormationDropdown->GetOptionCount() > 0)
    {
        const FString& DefaultOption = FormationDropdown->GetOptionAtIndex(0);
        bUpdatingFormationSelection = true;
        FormationDropdown->SetSelectedOption(DefaultOption);
        bUpdatingFormationSelection = false;
    }
}

void UUnitsSelectionWidget::UpdateFormationSelection()
{
    if (!FormationDropdown || !SpawnComponent)
        return;

    if (const FString* FoundOption = FormationToOption.Find(SpawnComponent->GetSpawnFormation()))
    {
        bUpdatingFormationSelection = true;
        FormationDropdown->SetSelectedOption(*FoundOption);
        bUpdatingFormationSelection = false;
    }
}

void UUnitsSelectionWidget::RefreshCustomFormationInputs() const
{
    const bool bIsCustom = SpawnComponent && SpawnComponent->GetSpawnFormation() == ESpawnFormation::Custom;
    const FIntPoint Dimensions = SpawnComponent ? SpawnComponent->GetCustomFormationDimensions() : FIntPoint(1, 1);

    if (CustomFormationXTextBox)
    {
        CustomFormationXTextBox->SetIsEnabled(bIsCustom);
        CustomFormationXTextBox->SetText(FText::AsNumber(Dimensions.X));
    }

    if (CustomFormationYTextBox)
    {
        CustomFormationYTextBox->SetIsEnabled(bIsCustom);
        CustomFormationYTextBox->SetText(FText::AsNumber(Dimensions.Y));
    }
}

void UUnitsSelectionWidget::OnFormationOptionChanged(FString SelectedItem, ESelectInfo::Type /*SelectionType*/)
{
    if (bUpdatingFormationSelection || !SpawnComponent)
        return;

    if (const ESpawnFormation* Formation = OptionToFormation.Find(SelectedItem))
    {
        SpawnComponent->SetSpawnFormation(*Formation);
        RefreshCustomFormationInputs();
    }
}

void UUnitsSelectionWidget::OnCustomFormationXCommitted(const FText& Text, ETextCommit::Type /*CommitMethod*/)
{
    if (!SpawnComponent)
        return;

    int32 ParsedValue = SpawnComponent->GetCustomFormationDimensions().X;
    const FString TextString = Text.ToString();

    if (!TextString.IsEmpty())
    {
        if (LexFromString(ParsedValue, *TextString))
        {
            ParsedValue = FMath::Max(1, ParsedValue);
        }
        else
        {
            ParsedValue = 1;
        }
    }

    const int32 CurrentY = SpawnComponent->GetCustomFormationDimensions().Y;
    SpawnComponent->SetCustomFormationDimensions(FIntPoint(ParsedValue, CurrentY));
    RefreshCustomFormationInputs();
}

void UUnitsSelectionWidget::OnCustomFormationYCommitted(const FText& Text, ETextCommit::Type /*CommitMethod*/)
{
    if (!SpawnComponent)
        return;

    int32 ParsedValue = SpawnComponent->GetCustomFormationDimensions().Y;
    const FString TextString = Text.ToString();

    if (!TextString.IsEmpty())
    {
        if (LexFromString(ParsedValue, *TextString))
        {
            ParsedValue = FMath::Max(1, ParsedValue);
        }
        else
        {
            ParsedValue = 1;
        }
    }

    const int32 CurrentX = SpawnComponent->GetCustomFormationDimensions().X;
    SpawnComponent->SetCustomFormationDimensions(FIntPoint(CurrentX, ParsedValue));
    RefreshCustomFormationInputs();
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
    const bool bHasSearch = !CurrentSearchText.IsEmpty();

    if (!bHasSearch && !bFilterByTag)
    {
        for (UUnitsEntryWidget* EntryWidget : EntryList)
        {
            if (EntryWidget)
            {
                UnitsSelectionWidget::Private::SetVisibilityIfChanged(EntryWidget, ESlateVisibility::Visible);
            }
        }
        return;
    }

    for (UUnitsEntryWidget* EntryWidget : EntryList)
    {
        if (!EntryWidget)
        {
            continue;
        }

        const bool bMatchesTag = !bFilterByTag || EntryWidget->HasTag(CurrentTagFilter);

        if (!bMatchesTag)
        {
            UnitsSelectionWidget::Private::SetVisibilityIfChanged(EntryWidget, ESlateVisibility::Collapsed);
            continue;
        }

        const bool bMatchesSearch = !bHasSearch || EntryWidget->MatchesSearch(CurrentSearchText);
        const ESlateVisibility TargetVisibility = (bMatchesSearch && bMatchesTag) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
        UnitsSelectionWidget::Private::SetVisibilityIfChanged(EntryWidget, TargetVisibility);
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
