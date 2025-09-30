#pragma once

#include "Components/ActorComponent.h"
#include "FlowFieldTypes.h"
#include "HAL/PlatformRWLock.h"
#include "FlowVoxelGridComponent.generated.h"

struct FFlowFieldBuildInput;
struct FFlowFieldBuildOutput;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ULTRAFLOWFIELDRUNTIME_API UFlowVoxelGridComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFlowVoxelGridComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Field")
    FFlowFieldSettings Settings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Field")
    bool bAutoInitialize = true;

    virtual void BeginPlay() override;

    void InitializeIfNeeded();

    void SetWorldBounds(const FBox& InBounds);

    bool WorldToVoxel(const FVector& WorldLocation, FIntVector& OutChunk, FIntVector& OutLocalVoxel) const;
    FVector VoxelToWorldCenter(const FIntVector& ChunkCoord, const FIntVector& LocalVoxel) const;

    FFlowVoxelChunk* GetOrCreateChunk(const FIntVector& ChunkCoord);
    const FFlowVoxelChunk* GetChunk(const FIntVector& ChunkCoord) const;

    void ForEachChunk(TFunctionRef<void(const FIntVector&, FFlowVoxelChunk&)> Func);

    void MarkRegionDirty(const FIntVector& ChunkCoord, const FIntVector& LocalMin, const FIntVector& LocalMax);

    void SetObstacleBox(const FVector& Center, const FVector& Extents, bool bBlocked);

    void FillBuildInput(FFlowFieldBuildInput& OutInput) const;
    void ConsumeBuildOutput(const FFlowFieldBuildOutput& Output);

    int32 GetChunkVoxelCount() const;
    FORCEINLINE float GetVoxelSize() const { return Settings.VoxelSize; }

private:
    TMap<FIntVector, FFlowVoxelChunk> ChunkMap;
    mutable FRWLock ChunkMapLock;
};

