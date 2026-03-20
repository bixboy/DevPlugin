#include "UI/Editor/Widgets/Spawn/UnitsEntryWidget.h"
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

void UUnitsEntryWidget::SetPlacementSystem(UCameraPlacementSystem* InPlacementSystem)
{
	PlacementSystem = InPlacementSystem;
}

void UUnitsEntryWidget::InitEntry(UPlacementUnitData* Data)
{
	if (!Data)
		return;

	PlacementData = Data;
	CachedUnitName = Data->DisplayName;
	UnitTags = Data->Tags;
    
	if (UnitButton)
	{
		UnitButton->SetButtonTexture(Data->Icon);
		UnitButton->SetButtonText(CachedUnitName);
	}
}

void UUnitsEntryWidget::OnUnitSelected(UCustomButtonWidget* Button, int Index)
{
	if (!PlacementData || !PlacementSystem.IsValid())
		return;

	PlacementSystem->StartPlacement(PlacementData.Get());
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