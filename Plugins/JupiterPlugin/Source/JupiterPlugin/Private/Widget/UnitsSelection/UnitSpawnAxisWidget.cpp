#include "Widget/UnitsSelection/UnitSpawnAxisWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/UnitSpawnComponent.h"


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

        const ESpawnFormation CurrentFormation = SpawnComponent ? SpawnComponent->GetSpawnFormation() : ESpawnFormation::Square;
        RefreshCustomFormationInputs(CurrentFormation);
}

void UUnitSpawnAxisWidget::OnCustomFormationXCommitted(const FText& Text, ETextCommit::Type /*CommitMethod*/)
{
        if (!SpawnComponent)
                return;

        int32 ParsedValue = SpawnComponent->GetCustomFormationDimensions().X;

        if (!Text.IsEmpty())
                ParsedValue = FMath::Max(1, FCString::Atoi(*Text.ToString()));

        const int32 CurrentY = SpawnComponent->GetCustomFormationDimensions().Y;
        SpawnComponent->SetCustomFormationDimensions(FIntPoint(ParsedValue, CurrentY));

        const ESpawnFormation CurrentFormation = SpawnComponent->GetSpawnFormation();
        RefreshCustomFormationInputs(CurrentFormation);
}

void UUnitSpawnAxisWidget::OnCustomFormationYCommitted(const FText& Text, ETextCommit::Type /*CommitMethod*/)
{
        if (!SpawnComponent)
                return;

        int32 ParsedValue = SpawnComponent->GetCustomFormationDimensions().Y;

        if (!Text.IsEmpty())
                ParsedValue = FMath::Max(1, FCString::Atoi(*Text.ToString()));

        const int32 CurrentX = SpawnComponent->GetCustomFormationDimensions().X;
        SpawnComponent->SetCustomFormationDimensions(FIntPoint(CurrentX, ParsedValue));

        const ESpawnFormation CurrentFormation = SpawnComponent->GetSpawnFormation();
        RefreshCustomFormationInputs(CurrentFormation);
}

void UUnitSpawnAxisWidget::RefreshCustomFormationInputs(ESpawnFormation NewFormation)
{
        const bool bIsCustom = NewFormation == ESpawnFormation::Custom;
        const FIntPoint Dimensions = SpawnComponent ? SpawnComponent->GetCustomFormationDimensions() : FIntPoint(1, 1);

        if (FormationX)
        {
                FormationX->SetIsEnabled(bIsCustom);
                FormationX->SetText(FText::AsNumber(Dimensions.X));
        }

        if (FormationY)
        {
                FormationY->SetIsEnabled(bIsCustom);
                FormationY->SetText(FText::AsNumber(Dimensions.Y));
        }

        if (bIsCustom)
        {
                ApplyCustomFormationToSpawnCount();
        }
}

void UUnitSpawnAxisWidget::HandleCustomFormationDimensionsChanged(FIntPoint /*NewDimensions*/)
{
        const ESpawnFormation CurrentFormation = SpawnComponent ? SpawnComponent->GetSpawnFormation() : ESpawnFormation::Square;
        RefreshCustomFormationInputs(CurrentFormation);
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
