#pragma once

#include "CoreMinimal.h"
#include "FlowFieldTypes.generated.h"

UENUM(BlueprintType)
enum class EAgentLayer : uint8
{
    Ground = 0,
    Flying,
    Amphibious,
    Custom0,
    Custom1,
    Custom2,
    Custom3
};

USTRUCT(BlueprintType)
struct FFlowFieldSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Field")
    float VoxelSize = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Field")
    int32 ChunkSizeX = 32;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Field")
    int32 ChunkSizeY = 32;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Field")
    int32 ChunkSizeZ = 16;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Field")
    int32 MaxLOD = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Field")
    FVector GridOrigin = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow Field")
    FBox WorldBounds = FBox(EForceInit::ForceInitToZero);
};

USTRUCT()
struct FFlowVoxel
{
    GENERATED_BODY()

    UPROPERTY()
    uint8 WalkableMask = 0xFF;

    UPROPERTY()
    uint16 Cost = 1;

    UPROPERTY()
    int32 Distance = TNumericLimits<int32>::Max();

    UPROPERTY()
    FIntVector FlowDirPacked = FIntVector::ZeroValue;
};

USTRUCT()
struct FFlowFieldBuffer
{
    GENERATED_BODY()

    TArray<int32> DistanceField;
    TArray<FIntVector> PackedDirections;
    FIntVector Dimensions = FIntVector::ZeroValue;

    void Reset()
    {
        DistanceField.Reset();
        PackedDirections.Reset();
        Dimensions = FIntVector::ZeroValue;
    }

    FORCEINLINE bool IsValidIndex(int32 Index) const
    {
        return DistanceField.IsValidIndex(Index) && PackedDirections.IsValidIndex(Index);
    }
};

USTRUCT()
struct FFlowVoxelChunk
{
    GENERATED_BODY()

    TArray<FFlowVoxel> Voxels;
    FIntVector Dimensions = FIntVector::ZeroValue;
    FIntVector OriginVoxel = FIntVector::ZeroValue;
    bool bIsDirty = true;

    void Init(const FIntVector& InDimensions)
    {
        Dimensions = InDimensions;
        const int32 Total = Dimensions.X * Dimensions.Y * Dimensions.Z;
        Voxels.SetNum(Total);
        bIsDirty = true;
    }

    FORCEINLINE int32 GetIndex(const FIntVector& Local) const
    {
        return (Local.Z * Dimensions.Y * Dimensions.X) + (Local.Y * Dimensions.X) + Local.X;
    }

    FORCEINLINE bool IsValidLocal(const FIntVector& Local) const
    {
        return Local.X >= 0 && Local.X < Dimensions.X && Local.Y >= 0 && Local.Y < Dimensions.Y && Local.Z >= 0 && Local.Z < Dimensions.Z;
    }
};

USTRUCT()
struct FFlowFieldDirtyRegion
{
    GENERATED_BODY()

    UPROPERTY()
    FIntVector ChunkCoord = FIntVector::ZeroValue;

    UPROPERTY()
    FIntVector VoxelMin = FIntVector::ZeroValue;

    UPROPERTY()
    FIntVector VoxelMax = FIntVector::ZeroValue;

    FFlowFieldDirtyRegion() = default;

    FFlowFieldDirtyRegion(const FIntVector& InChunk, const FIntVector& InMin, const FIntVector& InMax)
        : ChunkCoord(InChunk)
        , VoxelMin(InMin)
        , VoxelMax(InMax)
    {
    }
};

