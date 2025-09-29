#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DynamicPathManager.generated.h"

class UGridNavigationComponent;
class UUnitNavigationComponent;
class UUnitAvoidanceComponent;

USTRUCT()
struct FDynamicAvoidanceNeighbor
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<UUnitAvoidanceComponent> Component;

    UPROPERTY()
    FVector Location;

    UPROPERTY()
    FVector Velocity;
};

UCLASS()
class DYNAMICPATHFINDINGRUNTIME_API ADynamicPathManager : public AActor
{
    GENERATED_BODY()

public:
    ADynamicPathManager();

    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;

    /** Voxel grid responsible for global navigation. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
    UGridNavigationComponent* GridComponent;

    /** Size of the spatial hashing cells used for avoidance lookups. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float SpatialHashCellSize;

    /** Enables or disables debug rendering for the grid. */
    UFUNCTION(BlueprintCallable, Category = "Dynamic Pathfinding")
    void SetDebugEnabled(bool bEnabled);

    /** Blueprint callable helper to update the navigation target. */
    UFUNCTION(BlueprintCallable, Category = "Dynamic Pathfinding")
    void SetTargetLocation(const FVector& TargetLocation);

    /** Console helper to toggle debug drawing. */
    UFUNCTION(Exec)
    void ToggleDynamicPathDebug();

    /** Allows units to register/unregister with the manager. */
    void RegisterUnit(UUnitNavigationComponent* UnitComponent);
    void UnregisterUnit(UUnitNavigationComponent* UnitComponent);

    void RegisterAvoidanceComponent(UUnitAvoidanceComponent* Component);
    void UnregisterAvoidanceComponent(UUnitAvoidanceComponent* Component);

    void UpdateSpatialEntry(UUnitAvoidanceComponent* Component, const FVector& Location);

    FVector ComputeAvoidanceForComponent(const UUnitAvoidanceComponent* Component, const FVector& DesiredVelocity) const;

    FVector GetFlowDirectionAtLocation(const FVector& Location) const;

    void MarkObstacle(const FVector& Location, const FVector& Extents, bool bBlocked);

protected:
    virtual void PostRegisterAllComponents() override;

private:
    struct FSpatialCell
    {
        TArray<TWeakObjectPtr<UUnitAvoidanceComponent>> Components;
    };

    TSet<TWeakObjectPtr<UUnitNavigationComponent>> RegisteredUnits;

    TMap<FIntVector, FSpatialCell> SpatialHash;

    FIntVector GetCellKey(const FVector& Location) const;

    void RemoveComponentFromCell(UUnitAvoidanceComponent* Component, const FIntVector& CellKey);

    void DrawAvoidanceDebug() const;

    UPROPERTY(EditAnywhere, Category = "Debug")
    bool bDrawAvoidanceDebug;
};
