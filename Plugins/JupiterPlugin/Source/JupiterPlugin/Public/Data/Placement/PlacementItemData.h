#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StreamableRenderAsset.h"
#include "Interfaces/PlacementItemInterface.h"
#include "PlacementItemData.generated.h"


UCLASS(Abstract, BlueprintType)
class JUPITERPLUGIN_API UPlacementItemData : public UPrimaryDataAsset, public IPlacementItemInterface
{
	GENERATED_BODY()

public:
    virtual UObject* GetPreviewAsset_Implementation() const override;
	
    virtual bool SupportsFormations_Implementation() const override;
	
	
    virtual bool IsGroupPlacement_Implementation() const override;
	
    virtual bool IsPlacementValid_Implementation(const FVector& Location, const FHitResult& Hit) const override;

	
    virtual int32 GetDefaultUnitCount_Implementation() const override;
	
    virtual float GetFormationSpacing_Implementation() const override;
	
    virtual uint8 GetDefaultFormation_Implementation() const override;
	
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	UTexture2D* Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
	TArray<FName> Tags;
	
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement|Visuals")
	TSoftObjectPtr<UStreamableRenderAsset> PreviewMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement|Visuals")
	FVector InternalScale = FVector(1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement|Visuals")
	FVector PreviewOffset = FVector::ZeroVector;
	
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement|Logic")
	TSubclassOf<AActor> ActorToSpawn;
};
