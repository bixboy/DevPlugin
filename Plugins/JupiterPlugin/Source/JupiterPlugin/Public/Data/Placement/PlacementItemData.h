#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StreamableRenderAsset.h"
#include "PlacementItemData.generated.h"


UCLASS(Abstract, BlueprintType)
class JUPITERPLUGIN_API UPlacementItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- UI Info ---
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	UTexture2D* Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	TArray<FName> Tags;

	// --- Visuals ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement|Visuals")
	TSoftObjectPtr<UStreamableRenderAsset> PreviewMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement|Visuals")
	FVector InternalScale = FVector(1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement|Visuals")
	FVector PreviewOffset = FVector::ZeroVector;

	// --- Logic ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement|Logic")
	TSubclassOf<AActor> ActorToSpawn;
};
