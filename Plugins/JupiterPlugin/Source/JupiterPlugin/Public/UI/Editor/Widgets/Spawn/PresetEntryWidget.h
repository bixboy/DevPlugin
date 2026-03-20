#pragma once
#include "CoreMinimal.h"
#include "UI/CustomButtonWidget.h"
#include "PresetEntryWidget.generated.h"

class UTextBlock;

struct FPlacementPreset;


/** A single preset list item — inherits directly from UCustomButtonWidget. */
UCLASS()
class JUPITERPLUGIN_API UPresetEntryWidget : public UCustomButtonWidget
{
	GENERATED_BODY()

public:
	/** Populate the widget from a preset. */
	void InitFromPreset(const FPlacementPreset& Preset);

	FGuid GetPresetID() const { return CachedPresetID; }
	FName GetPresetName() const { return CachedPresetName; }

	// --- Optional bound widgets ---

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCustomButtonWidget> Btn_Delete;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_ActorCount;

private:
	FGuid CachedPresetID;
	FName CachedPresetName;
};
