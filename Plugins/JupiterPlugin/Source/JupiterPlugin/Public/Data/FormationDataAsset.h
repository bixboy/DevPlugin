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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FVector Offset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool Alternate;
};
