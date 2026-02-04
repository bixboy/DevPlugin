#include "UI/Editor/Widgets/Spawn/UnitSpawnAxisWidget.h"
#include "Player/JupiterPlayerSystem/CameraPlacementSystem.h"
#include "Components/EditableTextBox.h"

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

    if (PlacementSystem.IsValid())
    {
        PlacementSystem->OnSpawnFormationChanged.RemoveDynamic(this, &UUnitSpawnAxisWidget::RefreshCustomFormationInputs);
        PlacementSystem->OnCustomFormationDimensionsChanged.RemoveDynamic(this, &UUnitSpawnAxisWidget::HandleCustomFormationDimensionsChanged);
        PlacementSystem = nullptr;
    }
}

void UUnitSpawnAxisWidget::SetupWithSystem(UCameraPlacementSystem* InPlacementSystem)
{
    if (PlacementSystem.IsValid())
    {
        PlacementSystem->OnSpawnFormationChanged.RemoveDynamic(this, &UUnitSpawnAxisWidget::RefreshCustomFormationInputs);
        PlacementSystem->OnCustomFormationDimensionsChanged.RemoveDynamic(this, &UUnitSpawnAxisWidget::HandleCustomFormationDimensionsChanged);
    }

    PlacementSystem = InPlacementSystem;

    if (PlacementSystem.IsValid())
    {
        PlacementSystem->OnSpawnFormationChanged.AddDynamic(this, &UUnitSpawnAxisWidget::RefreshCustomFormationInputs);
        PlacementSystem->OnCustomFormationDimensionsChanged.AddDynamic(this, &UUnitSpawnAxisWidget::HandleCustomFormationDimensionsChanged);
    }

    RefreshCustomFormationInputs(PlacementSystem.IsValid() ? PlacementSystem->CurrentFormation : ESpawnFormation::Square);
}

void UUnitSpawnAxisWidget::OnCustomFormationXCommitted(const FText& Text, ETextCommit::Type /*CommitMethod*/)
{
    if (!PlacementSystem.IsValid())
        return;

    const int32 CurrentY = PlacementSystem->CustomFormationDimensions.Y;
    int32 NewX = FMath::Max(1, FCString::Atoi(*Text.ToString()));

    bIsUpdatingFromUI = true;
    PlacementSystem->SetCustomFormationDimensions(FIntPoint(NewX, CurrentY));
    bIsUpdatingFromUI = false;

    ApplyCustomFormationToSpawnCount();
}

void UUnitSpawnAxisWidget::OnCustomFormationYCommitted(const FText& Text, ETextCommit::Type /*CommitMethod*/)
{
    if (!PlacementSystem.IsValid())
        return;

    const int32 CurrentX = PlacementSystem->CustomFormationDimensions.X;
    int32 NewY = FMath::Max(1, FCString::Atoi(*Text.ToString()));

    bIsUpdatingFromUI = true;
    PlacementSystem->SetCustomFormationDimensions(FIntPoint(CurrentX, NewY));
    bIsUpdatingFromUI = false;

    ApplyCustomFormationToSpawnCount();
}

void UUnitSpawnAxisWidget::HandleCustomFormationDimensionsChanged(FIntPoint /*NewDimensions*/)
{
    if (bIsUpdatingFromUI)
        return; 

    const ESpawnFormation CurrentFormation = PlacementSystem.IsValid() ? PlacementSystem->CurrentFormation : ESpawnFormation::Square;
    RefreshCustomFormationInputs(CurrentFormation);
}

void UUnitSpawnAxisWidget::RefreshCustomFormationInputs(ESpawnFormation NewFormation)
{
    const bool bIsCustom = NewFormation == ESpawnFormation::Custom;
    const FIntPoint Dimensions = PlacementSystem.IsValid() ? PlacementSystem->CustomFormationDimensions : FIntPoint(1, 1);

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
    if (!PlacementSystem.IsValid() || PlacementSystem->CurrentFormation != ESpawnFormation::Custom)
        return;

    const FIntPoint Dimensions = PlacementSystem->CustomFormationDimensions;
    const int32 DesiredCount = FMath::Max(1, Dimensions.X * Dimensions.Y);

    if (PlacementSystem->CurrentSpawnCount == DesiredCount)
        return;

    PlacementSystem->SetSpawnCount(DesiredCount);
}
