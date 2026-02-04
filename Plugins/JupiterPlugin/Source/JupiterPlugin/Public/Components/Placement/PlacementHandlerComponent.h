#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Placement/PlacementItemData.h"
#include "Data/Placement/PlacementTypes.h"
#include "PlacementHandlerComponent.generated.h"

class UPlacementUnitData;


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JUPITERPLUGIN_API UPlacementHandlerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPlacementHandlerComponent();

protected:
	virtual void BeginPlay() override;

public:	
	
	UFUNCTION(Server, Reliable, WithValidation)
    void Server_RequestPlacement(const UPlacementItemData* ItemData, const FVector& Location, const FRotator& Rotation, int32 Count, ESpawnFormation Formation, FIntPoint CustomDimensions = FIntPoint::ZeroValue);

private:
    void SpawnSingleActor(const UPlacementItemData* Item, const FVector& Location, const FRotator& Rotation);
    
    void SpawnUnitGroup(const UPlacementUnitData* UnitData, const FVector& Location, const FRotator& Rotation, int32 Count, ESpawnFormation Formation, FIntPoint CustomDimensions);

    void GenerateFormationOffsets(TArray<FVector>& OutOffsets, int32 Count, float Spacing, const FRotator& Facing, ESpawnFormation Formation, FIntPoint CustomDimensions = FIntPoint::ZeroValue) const;

};
