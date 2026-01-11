#include "UI/UnitsSelection/UnitSpawnAxisWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/Unit/UnitSpawnComponent.h"

void UUnitSpawnAxisWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (FormationX && FormationY)
    {
        FormationX->OnTextCommitted.AddDynamic(this, &UUnitSpawnAxisWidget::OnCustomFormationXCommitted);
        FormationY->OnTextCommitted.AddDynamic(this, &UUnitSpawnAxisWidget::OnCustomFormationYCommitted);
    }
}

void UUnitSpawnAxisWidget::NativeDestruct()
{
    Super::NativeDestruct();

    if (FormationX && FormationY)
    {
        FormationX->OnTextCommitted.RemoveDynamic(this, &UUnitSpawnAxisWidget::OnCustomFormationXCommitted);
        FormationY->OnTextCommitted.RemoveDynamic(this, &UUnitSpawnAxisWidget::OnCustomFormationYCommitted);
    }

    if (SpawnComponent)
    {
        SpawnComponent->OnSpawnFormationChanged.RemoveDynamic(this, &UUnitSpawnAxisWidget::RefreshCustomFormationInputs);
        SpawnComponent->OnCustomFormationDimensionsChanged.RemoveDynamic(this, &UUnitSpawnAxisWidget::HandleCustomFormationDimensionsChanged);
        SpawnComponent = nullptr;
    }
}

void UUnitSpawnAxisWidget::SetupWithComponent(UUnitSpawnComponent* InSpawnComponent)
{
    if (SpawnComponent)
    {
        SpawnComponent->OnSpawnFormationChanged.RemoveDynamic(this, &UUnitSpawnAxisWidget::RefreshCustomFormationInputs);
        SpawnComponent->OnCustomFormationDimensionsChanged.RemoveDynamic(this, &UUnitSpawnAxisWidget::HandleCustomFormationDimensionsChanged);
    }

    SpawnComponent = InSpawnComponent;

    if (SpawnComponent)
    {
        SpawnComponent->OnSpawnFormationChanged.AddDynamic(this, &UUnitSpawnAxisWidget::RefreshCustomFormationInputs);
        SpawnComponent->OnCustomFormationDimensionsChanged.AddDynamic(this, &UUnitSpawnAxisWidget::HandleCustomFormationDimensionsChanged);
    }

    RefreshCustomFormationInputs(SpawnComponent ? SpawnComponent->GetSpawnFormation() : ESpawnFormation::Square);
}

void UUnitSpawnAxisWidget::OnCustomFormationXCommitted(const FText& Text, ETextCommit::Type /*CommitMethod*/)
{
    if (!SpawnComponent)
        return;

    const int32 CurrentY = SpawnComponent->GetCustomFormationDimensions().Y;
    int32 NewX = FMath::Max(1, FCString::Atoi(*Text.ToString()));

    // ✅ Évite les boucles de rafraîchissement forcées
    bIsUpdatingFromUI = true;
    SpawnComponent->SetCustomFormationDimensions(FIntPoint(NewX, CurrentY));
    bIsUpdatingFromUI = false;

    ApplyCustomFormationToSpawnCount();
}

void UUnitSpawnAxisWidget::OnCustomFormationYCommitted(const FText& Text, ETextCommit::Type /*CommitMethod*/)
{
    if (!SpawnComponent)
        return;

    const int32 CurrentX = SpawnComponent->GetCustomFormationDimensions().X;
    int32 NewY = FMath::Max(1, FCString::Atoi(*Text.ToString()));

    bIsUpdatingFromUI = true;
    SpawnComponent->SetCustomFormationDimensions(FIntPoint(CurrentX, NewY));
    bIsUpdatingFromUI = false;

    ApplyCustomFormationToSpawnCount();
}

void UUnitSpawnAxisWidget::HandleCustomFormationDimensionsChanged(FIntPoint /*NewDimensions*/)
{
    if (bIsUpdatingFromUI)
        return; // ✅ Empêche d’écraser la saisie manuelle

    const ESpawnFormation CurrentFormation = SpawnComponent ? SpawnComponent->GetSpawnFormation() : ESpawnFormation::Square;
    RefreshCustomFormationInputs(CurrentFormation);
}

void UUnitSpawnAxisWidget::RefreshCustomFormationInputs(ESpawnFormation NewFormation)
{
    const bool bIsCustom = NewFormation == ESpawnFormation::Custom;
    const FIntPoint Dimensions = SpawnComponent ? SpawnComponent->GetCustomFormationDimensions() : FIntPoint(1, 1);

    if (FormationX && !bIsUpdatingFromUI)
    {
        FormationX->SetIsEnabled(bIsCustom);
        FormationX->SetText(FText::AsNumber(Dimensions.X));
    }

    if (FormationY && !bIsUpdatingFromUI)
    {
        FormationY->SetIsEnabled(bIsCustom);
        FormationY->SetText(FText::AsNumber(Dimensions.Y));
    }
}

void UUnitSpawnAxisWidget::ApplyCustomFormationToSpawnCount() const
{
    if (!SpawnComponent || SpawnComponent->GetSpawnFormation() != ESpawnFormation::Custom)
        return;

    const FIntPoint Dimensions = SpawnComponent->GetCustomFormationDimensions();
    const int32 DesiredCount = FMath::Max(1, Dimensions.X * Dimensions.Y);

    if (SpawnComponent->GetUnitsPerSpawn() == DesiredCount)
        return;

    SpawnComponent->SetUnitsPerSpawn(DesiredCount);
}
