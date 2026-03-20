#pragma once
#include "CoreMinimal.h"
#include "PlacementItemData.h"
#include "PresetData.generated.h"


USTRUCT(BlueprintType)
struct JUPITERPLUGIN_API FPresetActorEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	TSubclassOf<AActor> ActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	FTransform RelativeTransform;
};


USTRUCT(BlueprintType)
struct JUPITERPLUGIN_API FPlacementPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	FName PresetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	FGuid PresetID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	TArray<FPresetActorEntry> Entries;

	bool IsValid() const { return PresetID.IsValid() && Entries.Num() > 0; }
};


UCLASS(BlueprintType)
class JUPITERPLUGIN_API UPlacementPresetData : public UPlacementItemData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	FPlacementPreset Preset;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("PlacementPresetData", GetFName());
	}
};
