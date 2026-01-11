#include "UI/UnitsSelection/UnitsEntryWidget.h"
#include "Components/Unit/UnitSpawnComponent.h"
#include "Data/UnitsSelectionDataAsset.h"
#include "Units/SoldierRts.h"
#include "UI/CustomButtonWidget.h"


void UUnitsEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Logic removed: UnitsEntryWidget is now passive and initialized via InitEntry/SetSpawnComponent.
	if (UnitButton)
	{
		UnitButton->OnButtonClicked.AddDynamic(this, &UUnitsEntryWidget::OnUnitSelected);
	}
}

void UUnitsEntryWidget::SetSpawnComponent(TWeakObjectPtr<UUnitSpawnComponent> InSpawnComponent)
{
	SpawnComponent = InSpawnComponent;
}

void UUnitsEntryWidget::InitEntry(UUnitsSelectionDataAsset* DataAsset)
{
	if (!DataAsset)
		return;

	FUnitsSelectionData UnitData = DataAsset->UnitSelectionData;

	CachedUnitName = UnitData.UnitName;
	UnitTags = UnitData.UnitTags;

	if (UnitButton)
	{
		UnitButton->SetButtonTexture(UnitData.UnitImage);
		UnitButton->SetButtonText(CachedUnitName);
	}
	
	UnitClass = UnitData.UnitClass;
}

void UUnitsEntryWidget::OnUnitSelected(UCustomButtonWidget* Button, int Index)
{
	if (!UnitClass || !SpawnComponent.IsValid())
		return;

	SpawnComponent->SetUnitToSpawn(UnitClass);
}

bool UUnitsEntryWidget::MatchesSearch(const FString& SearchLower) const
{
	if (SearchLower.IsEmpty())
		return true;

	FString NameLower = CachedUnitName.ToString();
	NameLower.ToLowerInline();

	return NameLower.Contains(SearchLower);
}

bool UUnitsEntryWidget::HasTag(FName Tag) const
{
	if (Tag.IsNone())
		return true;

	return UnitTags.Contains(Tag);
}