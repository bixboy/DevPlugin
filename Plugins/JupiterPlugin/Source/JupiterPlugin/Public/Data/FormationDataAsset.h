#pragma once

#include "CoreMinimal.h"
#include "AiData.h"
#include "Engine/DataAsset.h"
#include "FormationDataAsset.generated.h"

UCLASS()
class JUPITERPLUGIN_API UFormationDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Data Settings")
	FPrimaryAssetType DataType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TEnumAsByte<EFormation> FormationType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FText DisplayName;

	/** Normalized slot offsets that describe this formation. Each entry represents a unit position. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TArray<FVector2D> SlotOffsets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FVector Offset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool Alternate;
};
