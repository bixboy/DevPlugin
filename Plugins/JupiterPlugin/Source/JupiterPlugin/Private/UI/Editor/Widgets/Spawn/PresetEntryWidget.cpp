#include "UI/Editor/Widgets/Spawn/PresetEntryWidget.h"
#include "Data/Placement/PresetData.h"
#include "Components/TextBlock.h"


void UPresetEntryWidget::InitFromPreset(const FPlacementPreset& Preset)
{
	CachedPresetID = Preset.PresetID;
	CachedPresetName = Preset.PresetName;

	SetButtonText(FText::FromName(Preset.PresetName));

	if (Txt_ActorCount)
	{
		Txt_ActorCount->SetText(FText::AsNumber(Preset.Entries.Num()));
	}
}
