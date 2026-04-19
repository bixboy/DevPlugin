#include "UI/Editor/Widgets/Spawn/PropsEntryWidget.h"
#include "Data/Placement/PlacementItemData.h"
#include "Player/JupiterPlayerSystem/CameraPlacementSystem.h"
#include "UI/CustomButtonWidget.h"


void UPropsEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (ItemButton)
		ItemButton->OnButtonClicked.AddDynamic(this, &UPropsEntryWidget::OnItemSelected);
}

void UPropsEntryWidget::SetPlacementSystem(UCameraPlacementSystem* InPlacementSystem)
{
	PlacementSystem = InPlacementSystem;
}

void UPropsEntryWidget::InitEntry(UPlacementItemData* Data)
{
	if (!Data)
		return;

	PlacementData = Data;
	CachedDisplayName = Data->DisplayName;
	ItemTags = Data->Tags;

	if (ItemButton)
	{
		ItemButton->SetButtonTexture(Data->Icon);
		ItemButton->SetButtonText(CachedDisplayName);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PropsEntryWidget::InitEntry - ItemButton is NULL for data '%s'"), *CachedDisplayName.ToString());
	}
}

void UPropsEntryWidget::OnItemSelected(UCustomButtonWidget* Button, int Index)
{
	if (!PlacementData || !PlacementSystem.IsValid())
		return;

	PlacementSystem->StartPlacement(PlacementData);
}

bool UPropsEntryWidget::MatchesSearch(const FString& SearchLower) const
{
	if (SearchLower.IsEmpty())
		return true;

	FString NameLower = CachedDisplayName.ToString();
	NameLower.ToLowerInline();

	return NameLower.Contains(SearchLower);
}

bool UPropsEntryWidget::HasTag(FName Tag) const
{
	if (Tag.IsNone())
		return true;

	return ItemTags.Contains(Tag);
}
