#pragma once

#include "CoreMinimal.h"
#include "UnitsSelectionData.h"
#include "Engine/DataAsset.h"
#include "UnitsSelectionDataAsset.generated.h"


UCLASS()
class JUPITERPLUGIN_API UUnitsSelectionDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FUnitsSelectionData UnitSelectionData;
};
