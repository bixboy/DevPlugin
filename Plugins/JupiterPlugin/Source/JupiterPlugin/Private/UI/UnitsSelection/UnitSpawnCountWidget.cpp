#include "UI/UnitsSelection/UnitSpawnCountWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/Unit/UnitSpawnComponent.h"
#include "UI/CustomButtonWidget.h"


void UUnitSpawnCountWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Btn_IncreaseSpawnCount)
		Btn_IncreaseSpawnCount->OnButtonClicked.AddDynamic(this, &UUnitSpawnCountWidget::OnIncreasePressed);

	if (Btn_DecreaseSpawnCount)
		Btn_DecreaseSpawnCount->OnButtonClicked.AddDynamic(this, &UUnitSpawnCountWidget::OnDecreasePressed);

	if (SpawnCountTextBox)
		SpawnCountTextBox->OnTextCommitted.AddDynamic(this, &UUnitSpawnCountWidget::OnSpawnCountCommitted);

	RefreshDisplay();
}

void UUnitSpawnCountWidget::NativeDestruct()
{
	Super::NativeDestruct();

	if (Btn_IncreaseSpawnCount)
		Btn_IncreaseSpawnCount->OnButtonClicked.RemoveDynamic(this, &UUnitSpawnCountWidget::OnIncreasePressed);

	if (Btn_DecreaseSpawnCount)
		Btn_DecreaseSpawnCount->OnButtonClicked.RemoveDynamic(this, &UUnitSpawnCountWidget::OnDecreasePressed);

	if (SpawnCountTextBox)
		SpawnCountTextBox->OnTextCommitted.RemoveDynamic(this, &UUnitSpawnCountWidget::OnSpawnCountCommitted);

	if (SpawnComponent.IsValid())
		SpawnComponent->OnSpawnCountChanged.RemoveDynamic(this, &UUnitSpawnCountWidget::HandleSpawnCountChanged);
		
}

void UUnitSpawnCountWidget::SetupWithComponent(UUnitSpawnComponent* InSpawnComponent)
{
    if (SpawnComponent.IsValid())
    {
        SpawnComponent->OnSpawnCountChanged.RemoveDynamic(this, &UUnitSpawnCountWidget::HandleSpawnCountChanged);
    }

	SpawnComponent = InSpawnComponent;

	if (SpawnComponent.IsValid())
	{
		CachedSpawnCount = FMath::Max(1, SpawnComponent->GetUnitsPerSpawn());
		SpawnComponent->OnSpawnCountChanged.AddUniqueDynamic(this, &UUnitSpawnCountWidget::HandleSpawnCountChanged);
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

	if (SpawnComponent.IsValid())
		SpawnComponent->SetUnitsPerSpawn(Clamped);

	RefreshDisplay();
}

void UUnitSpawnCountWidget::OnIncreasePressed(UCustomButtonWidget* /*Button*/, int /*Index*/)
{
	OnSpawnCountCommitted(FText::AsNumber(CachedSpawnCount + 1), ETextCommit::Default);
}

void UUnitSpawnCountWidget::OnDecreasePressed(UCustomButtonWidget* /*Button*/, int /*Index*/)
{
	OnSpawnCountCommitted(FText::AsNumber(CachedSpawnCount - 1), ETextCommit::Default);
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
}