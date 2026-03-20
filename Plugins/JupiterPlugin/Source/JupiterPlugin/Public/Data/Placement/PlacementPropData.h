#pragma once
#include "CoreMinimal.h"
#include "Data/Placement/PlacementItemData.h"
#include "PlacementPropData.generated.h"


UCLASS(BlueprintType)
class JUPITERPLUGIN_API UPlacementPropData : public UPlacementItemData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement|Prop")
	FVector SpawnScale = FVector(1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	bool bCanBePartOfPreset = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	bool bAutoGround = true;
};
