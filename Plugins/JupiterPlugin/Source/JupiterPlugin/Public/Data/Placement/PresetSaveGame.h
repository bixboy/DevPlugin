#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Data/Placement/PresetData.h"
#include "PresetSaveGame.generated.h"


UCLASS()
class JUPITERPLUGIN_API UPresetSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FPlacementPreset> Presets;
};
