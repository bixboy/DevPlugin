#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Placement/PlacementItemData.h"
#include "Data/Placement/PlacementTypes.h"
#include "PlacementHandlerComponent.generated.h"

class UPlacementUnitData;

USTRUCT()
struct FAsyncSpawnBatch
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<const UPlacementItemData> ItemData;

	TArray<FTransform> Transforms;

	int32 CurrentIndex = 0;
};


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JUPITERPLUGIN_API UPlacementHandlerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPlacementHandlerComponent();
	
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UFUNCTION(Server, Reliable)
	void Server_RequestPlacement(const UPlacementItemData* ItemData, const FVector& Location, const FRotator& Rotation, 
		int32 Count, ESpawnFormation Formation, FIntPoint CustomDimensions = FIntPoint::ZeroValue);
	
private:
    void SpawnSingleActor(const UPlacementItemData* Item, const FVector& Location, const FRotator& Rotation);
    
    void SpawnUnitGroup(const UPlacementUnitData* UnitData, const FVector& Location, const FRotator& Rotation, int32 Count, 
    	ESpawnFormation Formation, FIntPoint CustomDimensions);

    void GenerateFormationOffsets(TArray<FVector>& OutOffsets, int32 Count, float Spacing, const FRotator& Facing, 
    	ESpawnFormation Formation, FIntPoint CustomDimensions = FIntPoint::ZeroValue) const;

	// --- Async Spawning State ---
	
	TArray<FAsyncSpawnBatch> SpawnQueue;
	
	UPROPERTY(EditDefaultsOnly, Category = "Optimization")
	int32 MaxSpawnsPerFrame = 20;

};
