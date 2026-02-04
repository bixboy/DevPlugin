#pragma once
#include "CoreMinimal.h"
#include "Data/Placement/PlacementItemData.h"
#include "Data/Placement/PlacementTypes.h"
#include "PlacementUnitData.generated.h"


UCLASS(BlueprintType)
class JUPITERPLUGIN_API UPlacementUnitData : public UPlacementItemData
{
	GENERATED_BODY()

public:
	// --- Formation Settings ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement|Unit")
	ESpawnFormation DefaultFormation = ESpawnFormation::Square;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement|Unit", meta = (ClampMin = 1))
	int32 DefaultUnitCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement|Unit", meta = (ClampMin = 0.0f))
	float FormationSpacing = 150.0f;
    
};
