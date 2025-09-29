#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HAL/RWLock.h"
#include "HAL/ThreadSafeBool.h"
#include "GridNavigationComponent.generated.h"

class ADynamicPathManager;

/**
 * Component responsible for maintaining a voxelized representation of the world and generating
 * flow field data that units can use to reach their target destination.
 */
UCLASS(ClassGroup = (Navigation), meta = (BlueprintSpawnableComponent))
class DYNAMICPATHFINDINGRUNTIME_API UGridNavigationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGridNavigationComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Size of the grid in voxels. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
    FIntVector GridSize;

    /** Uniform size of a voxel in world units. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
    float VoxelSize;

    /** World-space origin of the grid (corner at index 0,0,0). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
    FVector GridOrigin;

    /** How far dirty regions expand when recomputing the flow field. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FlowField")
    int32 DirtyPropagationDepth;

    /** Whether to render debug information for the voxel grid and flow field. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
    bool bDrawDebug;

    /** Duration for debug primitives. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
    float DebugDrawDuration;

    /** Sets the current goal location for the flow field and triggers regeneration. */
    void SetTargetLocation(const FVector& InTargetLocation, bool bImmediate = false);

    /** Marks a voxel as blocked or walkable. */
    void SetVoxelBlocked(const FIntVector& Coordinates, bool bBlocked);

    /** Returns the normalized flow direction for a given world position. */
    FVector GetFlowDirectionAtLocation(const FVector& WorldLocation) const;

    /** Queries whether a voxel is marked as walkable. */
    bool IsVoxelWalkable(const FIntVector& Coordinates) const;

    /** Enables or disables debug drawing. */
    void SetDebugEnabled(bool bEnabled);

    /** Draws debug visualization if enabled. */
    void DrawDebug();

    FORCEINLINE FVector GetTargetLocation() const { return TargetLocation; }

protected:
    void InitializeGrid();

    void RequestFlowFieldRebuild(bool bForceFullRebuild);

    void LaunchFlowFieldTask();

    void CompleteFlowFieldTask(TArray<int32>&& InDistanceField, TArray<FVector>&& InFlowField);

    bool IsValidCoordinates(const FIntVector& Coordinates) const;
    int32 GetIndex(const FIntVector& Coordinates) const;
    FIntVector GetCoordinatesFromIndex(int32 Index) const;
    FVector GetVoxelCenter(const FIntVector& Coordinates) const;

    void ExpandDirtyRegion(const FIntVector& Coordinates);

    void ClearDirtyRegion();

private:
    int32 NumVoxels;

    FVector TargetLocation;

    UPROPERTY(Transient)
    TArray<uint8> WalkableFlags;

    UPROPERTY(Transient)
    TArray<FVector> FlowFieldDirections;

    UPROPERTY(Transient)
    TArray<int32> DistanceField;

    mutable FRWLock DataLock;

    bool bHasValidFlowField;

    FIntVector DirtyMin;
    FIntVector DirtyMax;
    bool bHasDirtyRegion;
    bool bPendingFullRebuild;

    FThreadSafeBool bAsyncTaskRunning;
    FThreadSafeBool bDestroying;

    void BuildFlowFieldData(const FIntVector& RegionMin, const FIntVector& RegionMax, TArray<int32>& OutDistance, TArray<FVector>& OutFlowField) const;

    void DrawDebugInternal() const;
};
