#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnitAvoidanceComponent.generated.h"

class ADynamicPathManager;

/**
 * Handles local avoidance between units using a lightweight spatial hashing grid.
 */
UCLASS(ClassGroup = (Navigation), meta = (BlueprintSpawnableComponent))
class DYNAMICPATHFINDINGRUNTIME_API UUnitAvoidanceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UUnitAvoidanceComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** Radius used to determine which neighbors influence the avoidance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float NeighborRadius;

    /** Weight applied to the separation force. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float SeparationWeight;

    /** Maximum magnitude of the avoidance correction. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float MaxAvoidanceForce;

    /** Strength of reciprocal velocity obstacle adjustment. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float RVOWeight;

    /** Computes the avoidance velocity correction. */
    FVector ComputeAvoidance(const FVector& DesiredVelocity) const;

    /** Sets the manager responsible for spatial hashing. */
    void SetManager(ADynamicPathManager* InManager);

    /** Returns the most recent computed velocity. */
    FVector GetCurrentVelocity() const { return CurrentVelocity; }

    /** Internal use: updates the cached cell key. */
    void UpdateSpatialCellKey(const FIntVector& NewKey);

    FIntVector GetSpatialCellKey() const { return SpatialCellKey; }

private:
    TWeakObjectPtr<ADynamicPathManager> Manager;

    FVector CurrentVelocity;

    FVector LastWorldLocation;

    FIntVector SpatialCellKey;
};
