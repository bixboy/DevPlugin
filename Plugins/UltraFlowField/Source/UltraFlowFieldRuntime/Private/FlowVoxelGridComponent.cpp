#include "FlowVoxelGridComponent.h"
#include "Async/ParallelFor.h"
#include "Engine/World.h"

UFlowVoxelGridComponent::UFlowVoxelGridComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UFlowVoxelGridComponent::BeginPlay()
{
    Super::BeginPlay();
    if (bAutoInitialize)
    {
        InitializeIfNeeded();
    }
}

void UFlowVoxelGridComponent::InitializeIfNeeded()
{
    FWriteScopeLock ScopeLock(ChunkMapLock);
    if (ChunkMap.Num() == 0)
    {
        const FIntVector ChunkSize(Settings.ChunkSizeX, Settings.ChunkSizeY, Settings.ChunkSizeZ);
        const int32 Total = ChunkSize.X * ChunkSize.Y * ChunkSize.Z;
        const int32 ChunkCountX = FMath::Max(1, FMath::CeilToInt(Settings.WorldBounds.GetSize().X / (Settings.VoxelSize * ChunkSize.X)));
        const int32 ChunkCountY = FMath::Max(1, FMath::CeilToInt(Settings.WorldBounds.GetSize().Y / (Settings.VoxelSize * ChunkSize.Y)));
        const int32 ChunkCountZ = FMath::Max(1, FMath::CeilToInt(Settings.WorldBounds.GetSize().Z / (Settings.VoxelSize * ChunkSize.Z)));

        for (int32 X = 0; X < ChunkCountX; ++X)
        {
            for (int32 Y = 0; Y < ChunkCountY; ++Y)
            {
                for (int32 Z = 0; Z < ChunkCountZ; ++Z)
                {
                    const FIntVector ChunkKey(X, Y, Z);
                    FFlowVoxelChunk& Chunk = ChunkMap.Add(ChunkKey);
                    Chunk.Init(ChunkSize);
                    Chunk.OriginVoxel = ChunkKey * ChunkSize;
                    Chunk.Voxels.SetNum(Total);
                    for (FFlowVoxel& Voxel : Chunk.Voxels)
                    {
                        Voxel.WalkableMask = 0xFF;
                        Voxel.Cost = 1;
                        Voxel.Distance = TNumericLimits<int32>::Max();
                        Voxel.FlowDirPacked = FIntVector::ZeroValue;
                    }
                }
            }
        }
    }
}

void UFlowVoxelGridComponent::SetWorldBounds(const FBox& InBounds)
{
    Settings.WorldBounds = InBounds;
    InitializeIfNeeded();
}

bool UFlowVoxelGridComponent::WorldToVoxel(const FVector& WorldLocation, FIntVector& OutChunk, FIntVector& OutLocalVoxel) const
{
    const FVector Local = WorldLocation - Settings.GridOrigin;
    const FVector Relative = Local / Settings.VoxelSize;
    if (!Settings.WorldBounds.IsValid || Settings.WorldBounds.IsInside(WorldLocation))
    {
        const FIntVector ChunkSize(Settings.ChunkSizeX, Settings.ChunkSizeY, Settings.ChunkSizeZ);
        const FIntVector VoxelCoord(FMath::FloorToInt(Relative.X), FMath::FloorToInt(Relative.Y), FMath::FloorToInt(Relative.Z));
        OutChunk = FIntVector(
            FMath::FloorToInt((float)VoxelCoord.X / ChunkSize.X),
            FMath::FloorToInt((float)VoxelCoord.Y / ChunkSize.Y),
            FMath::FloorToInt((float)VoxelCoord.Z / ChunkSize.Z));
        const FIntVector ChunkOrigin = OutChunk * ChunkSize;
        OutLocalVoxel = VoxelCoord - ChunkOrigin;
        return true;
    }
    return false;
}

FVector UFlowVoxelGridComponent::VoxelToWorldCenter(const FIntVector& ChunkCoord, const FIntVector& LocalVoxel) const
{
    const FVector Base = Settings.GridOrigin + FVector(ChunkCoord * FIntVector(Settings.ChunkSizeX, Settings.ChunkSizeY, Settings.ChunkSizeZ)) * Settings.VoxelSize;
    return Base + FVector(LocalVoxel) * Settings.VoxelSize + FVector(Settings.VoxelSize * 0.5f);
}

FFlowVoxelChunk* UFlowVoxelGridComponent::GetOrCreateChunk(const FIntVector& ChunkCoord)
{
    FWriteScopeLock ScopeLock(ChunkMapLock);
    if (FFlowVoxelChunk* Chunk = ChunkMap.Find(ChunkCoord))
    {
        return Chunk;
    }

    FFlowVoxelChunk& NewChunk = ChunkMap.Add(ChunkCoord);
    NewChunk.Init(FIntVector(Settings.ChunkSizeX, Settings.ChunkSizeY, Settings.ChunkSizeZ));
    NewChunk.OriginVoxel = ChunkCoord * FIntVector(Settings.ChunkSizeX, Settings.ChunkSizeY, Settings.ChunkSizeZ);
    return &NewChunk;
}

const FFlowVoxelChunk* UFlowVoxelGridComponent::GetChunk(const FIntVector& ChunkCoord) const
{
    FReadScopeLock ScopeLock(ChunkMapLock);
    return ChunkMap.Find(ChunkCoord);
}

void UFlowVoxelGridComponent::ForEachChunk(TFunctionRef<void(const FIntVector&, FFlowVoxelChunk&)> Func)
{
    FWriteScopeLock ScopeLock(ChunkMapLock);
    for (auto& Pair : ChunkMap)
    {
        Func(Pair.Key, Pair.Value);
    }
}

void UFlowVoxelGridComponent::MarkRegionDirty(const FIntVector& ChunkCoord, const FIntVector& LocalMin, const FIntVector& LocalMax)
{
    FFlowVoxelChunk* Chunk = GetOrCreateChunk(ChunkCoord);
    if (!Chunk)
    {
        return;
    }

    Chunk->bIsDirty = true;

    const FIntVector ClampedMin(
        FMath::Clamp(LocalMin.X, 0, Chunk->Dimensions.X - 1),
        FMath::Clamp(LocalMin.Y, 0, Chunk->Dimensions.Y - 1),
        FMath::Clamp(LocalMin.Z, 0, Chunk->Dimensions.Z - 1));
    const FIntVector ClampedMax(
        FMath::Clamp(LocalMax.X, 0, Chunk->Dimensions.X - 1),
        FMath::Clamp(LocalMax.Y, 0, Chunk->Dimensions.Y - 1),
        FMath::Clamp(LocalMax.Z, 0, Chunk->Dimensions.Z - 1));

    for (int32 Z = ClampedMin.Z; Z <= ClampedMax.Z; ++Z)
    {
        for (int32 Y = ClampedMin.Y; Y <= ClampedMax.Y; ++Y)
        {
            for (int32 X = ClampedMin.X; X <= ClampedMax.X; ++X)
            {
                const int32 Index = Chunk->GetIndex(FIntVector(X, Y, Z));
                if (Chunk->Voxels.IsValidIndex(Index))
                {
                    Chunk->Voxels[Index].Distance = TNumericLimits<int32>::Max();
                    Chunk->Voxels[Index].FlowDirPacked = FIntVector::ZeroValue;
                }
            }
        }
    }
}

void UFlowVoxelGridComponent::SetObstacleBox(const FVector& Center, const FVector& Extents, bool bBlocked)
{
    const FVector Min = Center - Extents;
    const FVector Max = Center + Extents;

    const FIntVector ChunkSize(Settings.ChunkSizeX, Settings.ChunkSizeY, Settings.ChunkSizeZ);

    FIntVector ChunkMin, LocalMin;
    FIntVector ChunkMax, LocalMax;

    if (!WorldToVoxel(Min, ChunkMin, LocalMin))
    {
        const FVector RelativeMin = (Min - Settings.GridOrigin) / Settings.VoxelSize;
        const FIntVector VoxelMin(FMath::FloorToInt(RelativeMin.X), FMath::FloorToInt(RelativeMin.Y), FMath::FloorToInt(RelativeMin.Z));
        const FIntVector ChunkSize(Settings.ChunkSizeX, Settings.ChunkSizeY, Settings.ChunkSizeZ);
        ChunkMin = FIntVector(
            FMath::FloorToInt((float)VoxelMin.X / ChunkSize.X),
            FMath::FloorToInt((float)VoxelMin.Y / ChunkSize.Y),
            FMath::FloorToInt((float)VoxelMin.Z / ChunkSize.Z));
        LocalMin = VoxelMin - ChunkMin * ChunkSize;
    }

    if (!WorldToVoxel(Max, ChunkMax, LocalMax))
    {
        const FVector RelativeMax = (Max - Settings.GridOrigin) / Settings.VoxelSize;
        const FIntVector VoxelMax(FMath::FloorToInt(RelativeMax.X), FMath::FloorToInt(RelativeMax.Y), FMath::FloorToInt(RelativeMax.Z));
        const FIntVector ChunkSize(Settings.ChunkSizeX, Settings.ChunkSizeY, Settings.ChunkSizeZ);
        ChunkMax = FIntVector(
            FMath::FloorToInt((float)VoxelMax.X / ChunkSize.X),
            FMath::FloorToInt((float)VoxelMax.Y / ChunkSize.Y),
            FMath::FloorToInt((float)VoxelMax.Z / ChunkSize.Z));
        LocalMax = VoxelMax - ChunkMax * ChunkSize;
    }

    for (int32 X = ChunkMin.X; X <= ChunkMax.X; ++X)
    {
        for (int32 Y = ChunkMin.Y; Y <= ChunkMax.Y; ++Y)
        {
            for (int32 Z = ChunkMin.Z; Z <= ChunkMax.Z; ++Z)
            {
                FFlowVoxelChunk* Chunk = GetOrCreateChunk(FIntVector(X, Y, Z));
                if (!Chunk)
                {
                    continue;
                }

                const FIntVector LocalStart = (X == ChunkMin.X) ? LocalMin : FIntVector::ZeroValue;
                const FIntVector LocalEnd = (X == ChunkMax.X) ? LocalMax : Chunk->Dimensions - FIntVector(1, 1, 1);

                for (int32 LZ = LocalStart.Z; LZ <= LocalEnd.Z; ++LZ)
                {
                    for (int32 LY = LocalStart.Y; LY <= LocalEnd.Y; ++LY)
                    {
                        for (int32 LX = LocalStart.X; LX <= LocalEnd.X; ++LX)
                        {
                            const int32 Index = Chunk->GetIndex(FIntVector(LX, LY, LZ));
                            if (Chunk->Voxels.IsValidIndex(Index))
                            {
                                if (bBlocked)
                                {
                                    Chunk->Voxels[Index].WalkableMask = 0;
                                }
                                else
                                {
                                    Chunk->Voxels[Index].WalkableMask = 0xFF;
                                }
                            }
                        }
                    }
                }

                Chunk->bIsDirty = true;
            }
        }
    }
}

void UFlowVoxelGridComponent::FillBuildInput(FFlowFieldBuildInput& OutInput) const
{
    FReadScopeLock ScopeLock(ChunkMapLock);
    OutInput.ChunkSnapshots = ChunkMap;
    OutInput.VoxelSize = Settings.VoxelSize;
    OutInput.ChunkSize = FIntVector(Settings.ChunkSizeX, Settings.ChunkSizeY, Settings.ChunkSizeZ);
}

void UFlowVoxelGridComponent::ConsumeBuildOutput(const FFlowFieldBuildOutput& Output)
{
    FWriteScopeLock ScopeLock(ChunkMapLock);
    for (const auto& Pair : Output.ChunkResults)
    {
        if (FFlowVoxelChunk* Chunk = ChunkMap.Find(Pair.Key))
        {
            const FFlowFieldBuffer& Buffer = Pair.Value;
            const int32 Total = Chunk->Dimensions.X * Chunk->Dimensions.Y * Chunk->Dimensions.Z;
            for (int32 Index = 0; Index < Total && Buffer.DistanceField.IsValidIndex(Index); ++Index)
            {
                Chunk->Voxels[Index].Distance = Buffer.DistanceField[Index];
                Chunk->Voxels[Index].FlowDirPacked = Buffer.PackedDirections[Index];
            }
            Chunk->bIsDirty = false;
        }
    }
}

int32 UFlowVoxelGridComponent::GetChunkVoxelCount() const
{
    return Settings.ChunkSizeX * Settings.ChunkSizeY * Settings.ChunkSizeZ;
}

