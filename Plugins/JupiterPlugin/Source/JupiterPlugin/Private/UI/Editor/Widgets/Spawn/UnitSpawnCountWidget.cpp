#include "UI/Editor/Widgets/Spawn/UnitSpawnCountWidget.h"
#include "Player/JupiterPlayerSystem/CameraPlacementSystem.h"
#include "Components/EditableTextBox.h"
#include "UI/CustomSliderWidget.h"


void UUnitSpawnCountWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (SpawnCountTextBox)
		SpawnCountTextBox->OnTextCommitted.AddDynamic(this, &UUnitSpawnCountWidget::OnSpawnCountCommitted);

	if (SpawnCountSlider)
	{
		UE_LOG(LogTemp, Warning, TEXT("UnitSpawnCountWidget::NativeOnInitialized - SpawnCountSlider FOUND"));
		SpawnCountSlider->OnValueChanged.AddDynamic(this, &UUnitSpawnCountWidget::OnSliderValueChanged);
		SpawnCountSlider->SetMinMax(1.0f, static_cast<float>(MaxSpawnCount));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UnitSpawnCountWidget::NativeOnInitialized - SpawnCountSlider MISSING"));
	}

	RefreshDisplay();
}

void UUnitSpawnCountWidget::NativeDestruct()
{
	Super::NativeDestruct();

	Super::NativeDestruct();

	if (SpawnCountTextBox)
		SpawnCountTextBox->OnTextCommitted.RemoveDynamic(this, &UUnitSpawnCountWidget::OnSpawnCountCommitted);

	if (SpawnCountSlider)
		SpawnCountSlider->OnValueChanged.RemoveDynamic(this, &UUnitSpawnCountWidget::OnSliderValueChanged);

	if (PlacementSystem.IsValid())
		PlacementSystem->OnSpawnCountChanged.RemoveDynamic(this, &UUnitSpawnCountWidget::HandleSpawnCountChanged);
		
}

void UUnitSpawnCountWidget::SetupWithSystem(UCameraPlacementSystem* InPlacementSystem)
{
	UE_LOG(LogTemp, Warning, TEXT("UnitSpawnCountWidget::SetupWithSystem - InPlacementSystem: %s"), InPlacementSystem ? *InPlacementSystem->GetName() : TEXT("NULL"));

    if (PlacementSystem.IsValid())
    {
        PlacementSystem->OnSpawnCountChanged.RemoveDynamic(this, &UUnitSpawnCountWidget::HandleSpawnCountChanged);
    }

	PlacementSystem = InPlacementSystem;

	if (PlacementSystem.IsValid())
	{
		CachedSpawnCount = FMath::Max(1, PlacementSystem->CurrentSpawnCount);
		PlacementSystem->OnSpawnCountChanged.AddUniqueDynamic(this, &UUnitSpawnCountWidget::HandleSpawnCountChanged);
	}

	RefreshDisplay();
}

void UUnitSpawnCountWidget::OnSpawnCountCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	int32 ParsedValue = CachedSpawnCount;
	const FString TextString = Text.ToString();	

	if (!TextString.IsEmpty())
		ParsedValue = FCString::Atoi(*TextString);
	
	const int32 Clamped = FMath::Clamp(ParsedValue, 1, MaxSpawnCount);
	CachedSpawnCount = Clamped;

	if (PlacementSystem.IsValid())
		PlacementSystem->SetSpawnCount(Clamped);

	RefreshDisplay();
}

void UUnitSpawnCountWidget::OnSliderValueChanged(float Value)
{
	const int32 NewValue = FMath::RoundToInt(Value);
	if (NewValue != CachedSpawnCount)
	{
		CachedSpawnCount = NewValue;
		if (PlacementSystem.IsValid())
		{
			PlacementSystem->SetSpawnCount(CachedSpawnCount);
		}
		RefreshDisplay();
	}
}

void UUnitSpawnCountWidget::HandleSpawnCountChanged(int32 NewCount)
{
	CachedSpawnCount = FMath::Max(1, NewCount);
	RefreshDisplay();
}

void UUnitSpawnCountWidget::RefreshDisplay() const
{
	if (SpawnCountTextBox)
		SpawnCountTextBox->SetText(FText::AsNumber(CachedSpawnCount));

	if (SpawnCountSlider)
		SpawnCountSlider->SetValue(static_cast<float>(CachedSpawnCount));
}