#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnitNavigationComponent.generated.h"

class ADynamicPathManager;
class UUnitAvoidanceComponent;

/**
 * Component responsible for combining flow field navigation with local avoidance for a unit.
 */
UCLASS(ClassGroup = (Navigation), meta = (BlueprintSpawnableComponent))
class DYNAMICPATHFINDINGRUNTIME_API UUnitNavigationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UUnitNavigationComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
    float MoveSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
    float AvoidanceStrength;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
    float MaxSpeed;

    /** Attempts to locate a manager in the world if one is not already assigned. */
    void EnsureManagerReference();

    void SetManager(ADynamicPathManager* InManager);

private:
    TWeakObjectPtr<ADynamicPathManager> Manager;

    UPROPERTY()
    UUnitAvoidanceComponent* AvoidanceComponent;

    FVector CurrentVelocity;

    void ApplyMovement(const FVector& Velocity, float DeltaTime);

public:
    FORCEINLINE ADynamicPathManager* GetManager() const { return Manager.Get(); }
};
