#include "Widget/UnitsSelection/UnitSpawnFormationWidget.h"
#include "Components/ComboBoxString.h"


void UUnitSpawnFormationWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (FormationDropdown)
        FormationDropdown->OnSelectionChanged.AddDynamic(this, &UUnitSpawnFormationWidget::OnFormationChanged);
}

void UUnitSpawnFormationWidget::NativeDestruct()
{
        Super::NativeDestruct();

        if (SpawnComponent)
        {
                SpawnComponent->OnSpawnFormationChanged.RemoveDynamic(this, &UUnitSpawnFormationWidget::UpdateSelection);
                SpawnComponent = nullptr;
        }
}


void UUnitSpawnFormationWidget::SetupWithComponent(UUnitSpawnComponent* InSpawnComponent)
{
    if (SpawnComponent)
    {
        SpawnComponent->OnSpawnFormationChanged.RemoveDynamic(this, &UUnitSpawnFormationWidget::UpdateSelection);
    }

    SpawnComponent = InSpawnComponent;
    InitializeFormationOptions();
    const ESpawnFormation InitialFormation = SpawnComponent ? SpawnComponent->GetSpawnFormation() : ESpawnFormation::Square;
    UpdateSelection(InitialFormation);

    if (SpawnComponent)
        SpawnComponent->OnSpawnFormationChanged.AddDynamic(this, &UUnitSpawnFormationWidget::UpdateSelection);
}

void UUnitSpawnFormationWidget::InitializeFormationOptions()
{
    if (!FormationDropdown)
        return;

    FormationDropdown->ClearOptions();
    OptionToFormation.Reset();
    FormationToOption.Reset();

    if (const UEnum* EnumPtr = StaticEnum<ESpawnFormation>())
    {
        for (int32 i = 0; i < EnumPtr->NumEnums(); ++i)
        {
            if (EnumPtr->HasMetaData(TEXT("Hidden"), i))
                continue;

            const FString EnumName = EnumPtr->GetNameStringByIndex(i);
            if (EnumName.Contains(TEXT("MAX")))
                continue;

            const ESpawnFormation FormationValue = static_cast<ESpawnFormation>(EnumPtr->GetValueByIndex(i));
            const FString DisplayName = EnumPtr->GetDisplayNameTextByIndex(i).ToString();

            FormationDropdown->AddOption(DisplayName);
            OptionToFormation.Add(DisplayName, FormationValue);
            FormationToOption.Add(FormationValue, DisplayName);
        }
    }
}

void UUnitSpawnFormationWidget::UpdateSelection(ESpawnFormation /*NewFormation*/)
{
    if (!FormationDropdown || !SpawnComponent)
        return;

    if (const FString* Found = FormationToOption.Find(SpawnComponent->GetSpawnFormation()))
    {
        bUpdatingSelection = true;
        FormationDropdown->SetSelectedOption(*Found);
        bUpdatingSelection = false;
    }
}

void UUnitSpawnFormationWidget::OnFormationChanged(FString SelectedItem, ESelectInfo::Type /*SelectionType*/)
{
    if (bUpdatingSelection || !SpawnComponent)
        return;

    if (const ESpawnFormation* Found = OptionToFormation.Find(SelectedItem))
    {
        SpawnComponent->SetSpawnFormation(*Found);
    }
}