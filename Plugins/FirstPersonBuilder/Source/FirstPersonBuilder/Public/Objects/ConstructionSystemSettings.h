#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ConstructionSystemSettings.generated.h"

class UMaterialInterface;
class USoundBase;
class UNiagaraSystem;

/**
 * Data asset defining global settings for the construction system.
 */
UCLASS(BlueprintType)
class FIRSTPERSONBUILDER_API UConstructionSystemSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|SweetSpot")
	TObjectPtr<UMaterialInterface> SweetSpotMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction")
	float MaxPlacementDistance = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction")
	int32 ProgressPerHit = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|Feedback")
	TObjectPtr<USoundBase> DefaultHitSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|Feedback")
	TObjectPtr<USoundBase> DefaultPerfectHitSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|Feedback")
	TObjectPtr<UNiagaraSystem> DefaultHitParticle;

	/** Visual theme and icon settings for the catalog menu */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction|UI")
	TObjectPtr<class UBuilderMenuTheme> MenuTheme;
};
