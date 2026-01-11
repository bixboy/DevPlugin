#include "UI/UnitsSelection/UnitSpawnFormationWidget.h"
#include "Components/ComboBoxString.h"


void UUnitSpawnFormationWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    InitializeFormationOptions();

    if (FormationDropdown)
    {
        FormationDropdown->OnSelectionChanged.AddDynamic(this, &UUnitSpawnFormationWidget::OnFormationChanged);
    }
}

void UUnitSpawnFormationWidget::NativeDestruct()
{
    if (SpawnComponent.IsValid())
    {
        SpawnComponent->OnSpawnFormationChanged.RemoveDynamic(this, &UUnitSpawnFormationWidget::UpdateSelectionFromComponent);
    }
    
    Super::NativeDestruct();
}

void UUnitSpawnFormationWidget::SetupWithComponent(UUnitSpawnComponent* InSpawnComponent)
{
    if (SpawnComponent.IsValid())
    {
        SpawnComponent->OnSpawnFormationChanged.RemoveDynamic(this, &UUnitSpawnFormationWidget::UpdateSelectionFromComponent);
    }

    SpawnComponent = InSpawnComponent;
    UpdateSelectionFromComponent();

    if (SpawnComponent.IsValid())
    {
        SpawnComponent->OnSpawnFormationChanged.AddUniqueDynamic(this, &UUnitSpawnFormationWidget::UpdateSelectionFromComponent);
    }
}

void UUnitSpawnFormationWidget::InitializeFormationOptions()
{
    if (!FormationDropdown)
    	return;

    FormationDropdown->ClearOptions();
    OptionToFormation.Reset();
    FormationToOption.Reset();

    const UEnum* EnumPtr = StaticEnum<ESpawnFormation>();
    if (!EnumPtr) return;

    for (int32 i = 0; i < EnumPtr->NumEnums(); ++i)
    {
        if (EnumPtr->HasMetaData(TEXT("Hidden"), i))
        	continue;

        if (EnumPtr->GetNameStringByIndex(i).Contains(TEXT("MAX")))
        	continue;

        const ESpawnFormation FormationValue = static_cast<ESpawnFormation>(EnumPtr->GetValueByIndex(i));
        const FString DisplayName = EnumPtr->GetDisplayNameTextByIndex(i).ToString();

        FormationDropdown->AddOption(DisplayName);
        OptionToFormation.Add(DisplayName, FormationValue);
        FormationToOption.Add(FormationValue, DisplayName);
    }
}

void UUnitSpawnFormationWidget::UpdateSelectionFromComponent(ESpawnFormation /*NewFormation*/)
{
    if (!FormationDropdown || !SpawnComponent.IsValid())
    	return;

    const ESpawnFormation CurrentFormation = SpawnComponent->GetSpawnFormation();

    if (const FString* FoundLabel = FormationToOption.Find(CurrentFormation))
    {
        bUpdatingSelection = true;
        FormationDropdown->SetSelectedOption(*FoundLabel);
        bUpdatingSelection = false;
    }
}

void UUnitSpawnFormationWidget::UpdateSelectionFromComponent()
{
    UpdateSelectionFromComponent(ESpawnFormation::Square); 
}

void UUnitSpawnFormationWidget::OnFormationChanged(FString SelectedItem, ESelectInfo::Type /*SelectionType*/)
{
    // Si c'est nous qui mettons à jour l'UI, on ne renvoie pas la data au composant
    if (bUpdatingSelection || !SpawnComponent.IsValid()) return;

    if (const ESpawnFormation* FoundFormation = OptionToFormation.Find(SelectedItem))
    {
        SpawnComponent->SetSpawnFormation(*FoundFormation);
    }
}