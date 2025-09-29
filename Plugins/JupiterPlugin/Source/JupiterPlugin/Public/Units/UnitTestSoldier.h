#pragma once

#include "Units/SoldierRts.h"
#include "UnitTestSoldier.generated.h"

class UUnitNavigationComponent;
class UUnitAvoidanceComponent;

/**
 * Lightweight Soldier variant tailored for testing the dynamic pathfinding system.
 * The unit automatically registers navigation and avoidance components so that
 * designers can drop it in a level and immediately benefit from the new
 * flow-field based locomotion without additional setup.
 */
UCLASS()
class JUPITERPLUGIN_API AUnitTestSoldier : public ASoldierRts
{
    GENERATED_BODY()

public:
    AUnitTestSoldier();

    virtual void BeginPlay() override;
    virtual void CommandMove_Implementation(FCommandData CommandData) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
    /** Ensures component defaults stay in sync with the base character settings. */
    void InitializeNavigationDefaults();

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dynamic Pathfinding", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UUnitNavigationComponent> UnitNavigationComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dynamic Pathfinding", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UUnitAvoidanceComponent> UnitAvoidanceComponent;
};
