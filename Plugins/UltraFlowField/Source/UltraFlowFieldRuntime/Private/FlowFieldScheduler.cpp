#include "FlowFieldScheduler.h"
#include "FlowVoxelGridComponent.h"
#include "Async/Async.h"
#include "Containers/Queue.h"
#include "HAL/PlatformProcess.h"
#include "Misc/ScopeLock.h"
#include "Algo/Heap.h"

namespace UltraFlowField
{
    struct FQueueNode
    {
        int32 Index;
        int32 Distance;

        bool operator<(const FQueueNode& Other) const
        {
            return Distance > Other.Distance;
        }
    };

    static void BuildDistanceAndFlow(const FFlowFieldBuildInput& Input, FFlowFieldBuildOutput& Output)
    {
        Output.Target = Input.Target;
        Output.Layer = Input.Layer;

        const FVector Target = Input.Target;
        const float VoxelSize = Input.VoxelSize;
        const FIntVector ChunkSize = Input.ChunkSize;

        const FVector TargetVoxelF = Target / VoxelSize;
        const FIntVector TargetVoxel(
            FMath::FloorToInt(TargetVoxelF.X),
            FMath::FloorToInt(TargetVoxelF.Y),
            FMath::FloorToInt(TargetVoxelF.Z));

        const TArray<FIntVector> NeighborOffsets = {
            FIntVector(1, 0, 0), FIntVector(-1, 0, 0),
            FIntVector(0, 1, 0), FIntVector(0, -1, 0),
            FIntVector(0, 0, 1), FIntVector(0, 0, -1)
        };

        for (const auto& ChunkPair : Input.ChunkSnapshots)
        {
            const FIntVector ChunkCoord = ChunkPair.Key;
            const FFlowVoxelChunk& Chunk = ChunkPair.Value;

            FFlowFieldBuffer& Buffer = Output.ChunkResults.Add(ChunkCoord);
            const int32 Total = Chunk.Dimensions.X * Chunk.Dimensions.Y * Chunk.Dimensions.Z;
            Buffer.DistanceField.SetNumUninitialized(Total);
            Buffer.PackedDirections.SetNumUninitialized(Total);
            Buffer.Dimensions = Chunk.Dimensions;

            struct FHeapNode
            {
                int32 Index;
                int32 Distance;
            };

            TArray<int32>& Distances = Buffer.DistanceField;
            for (int32 Index = 0; Index < Total; ++Index)
            {
                Distances[Index] = TNumericLimits<int32>::Max();
                Buffer.PackedDirections[Index] = FIntVector::ZeroValue;
            }

            auto GetLocalCoord = [&Chunk](int32 Index)
            {
                const int32 X = Index % Chunk.Dimensions.X;
                const int32 Y = (Index / Chunk.Dimensions.X) % Chunk.Dimensions.Y;
                const int32 Z = Index / (Chunk.Dimensions.X * Chunk.Dimensions.Y);
                return FIntVector(X, Y, Z);
            };

            auto ToIndex = [&Chunk](const FIntVector& Local)
            {
                return (Local.Z * Chunk.Dimensions.Y * Chunk.Dimensions.X) + (Local.Y * Chunk.Dimensions.X) + Local.X;
            };

            // Priority queue for Dijkstra
            TArray<FQueueNode> Heap;

            auto Push = [&Heap](int32 Index, int32 Distance)
            {
                FQueueNode Node{Index, Distance};
                Heap.HeapPush(Node, [](const FQueueNode& A, const FQueueNode& B) { return A.Distance < B.Distance; });
            };

            auto Pop = [&Heap](int32& OutIndex, int32& OutDistance)
            {
                FQueueNode Node;
                Heap.HeapPop(Node, [](const FQueueNode& A, const FQueueNode& B) { return A.Distance < B.Distance; });
                OutIndex = Node.Index;
                OutDistance = Node.Distance;
            };

            auto IsEmpty = [&Heap]() { return Heap.Num() == 0; };

            // Seed goal inside chunk if overlaps
            const FIntVector ChunkOriginVoxel = ChunkCoord * Chunk.Dimensions;
            const FIntVector TargetLocal = TargetVoxel - ChunkOriginVoxel;
            if (Chunk.IsValidLocal(TargetLocal))
            {
                const int32 GoalIndex = ToIndex(TargetLocal);
                Distances[GoalIndex] = 0;
                Push(GoalIndex, 0);
            }

            while (!IsEmpty())
            {
                int32 CurrentIndex;
                int32 CurrentDistance;
                Pop(CurrentIndex, CurrentDistance);

                if (CurrentDistance > Distances[CurrentIndex])
                {
                    continue;
                }

                const FIntVector CurrentLocal = GetLocalCoord(CurrentIndex);

                for (const FIntVector& Offset : NeighborOffsets)
                {
                    const FIntVector NeighborLocal = CurrentLocal + Offset;
                    if (!Chunk.IsValidLocal(NeighborLocal))
                    {
                        continue;
                    }

                    const int32 NeighborIndex = ToIndex(NeighborLocal);
                    if (!Chunk.Voxels.IsValidIndex(NeighborIndex))
                    {
                        continue;
                    }

                    const FFlowVoxel& NeighborVoxel = Chunk.Voxels[NeighborIndex];
                    if ((NeighborVoxel.WalkableMask & (1 << (uint8)Input.Layer)) == 0)
                    {
                        continue;
                    }

                    const int32 StepCost = NeighborVoxel.Cost;
                    const int32 NewDistance = CurrentDistance + StepCost;
                    if (NewDistance < Distances[NeighborIndex])
                    {
                        Distances[NeighborIndex] = NewDistance;
                        Push(NeighborIndex, NewDistance);
                    }
                }
            }

            // Flow direction synthesis
            for (int32 Index = 0; Index < Total; ++Index)
            {
                const int32 Distance = Distances[Index];
                if (Distance == TNumericLimits<int32>::Max())
                {
                    Buffer.PackedDirections[Index] = FIntVector::ZeroValue;
                    continue;
                }

                const FIntVector Local = GetLocalCoord(Index);
                int32 BestDistance = Distance;
                FIntVector BestOffset = FIntVector::ZeroValue;

                for (const FIntVector& Offset : NeighborOffsets)
                {
                    const FIntVector NeighborLocal = Local + Offset;
                    if (!Chunk.IsValidLocal(NeighborLocal))
                    {
                        continue;
                    }

                    const int32 NeighborIndex = ToIndex(NeighborLocal);
                    if (!Chunk.Voxels.IsValidIndex(NeighborIndex))
                    {
                        continue;
                    }

                    const int32 NeighborDistance = Distances[NeighborIndex];
                    if (NeighborDistance < BestDistance)
                    {
                        BestDistance = NeighborDistance;
                        BestOffset = Offset;
                    }
                }

                Buffer.PackedDirections[Index] = BestOffset;
            }
        }
    }
}

FFlowFieldScheduler::FFlowFieldScheduler()
{
}

FFlowFieldScheduler::~FFlowFieldScheduler()
{
    CancelAll();
}

void FFlowFieldScheduler::EnqueueBuild(TSharedRef<FFlowFieldJob> Job)
{
    PendingJobs.Enqueue(Job);
    LaunchJob(Job);
}

void FFlowFieldScheduler::CancelAll()
{
    TSharedRef<FFlowFieldJob> Job;
    while (PendingJobs.Dequeue(Job))
    {
        Job->bCancelled = true;
    }

    FScopeLock Scope(&ActiveLock);
    for (const TSharedRef<FFlowFieldJob>& ActiveJob : ActiveJobs)
    {
        ActiveJob->bCancelled = true;
    }
    ActiveJobs.Empty();
}

void FFlowFieldScheduler::Tick()
{
    TSharedPtr<FFlowFieldBuildOutput> Output;
    while (CompletedJobs.Dequeue(Output))
    {
        if (Output.IsValid())
        {
            Output->ChunkResults.Compact();
        }
    }
}

void FFlowFieldScheduler::LaunchJob(const TSharedRef<FFlowFieldJob>& Job)
{
    {
        FScopeLock Scope(&ActiveLock);
        ActiveJobs.Add(Job);
    }

    AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, Job]()
    {
        if (Job->bCancelled)
        {
            FScopeLock Scope(&ActiveLock);
            ActiveJobs.Remove(Job);
            return;
        }

        TSharedPtr<FFlowFieldBuildOutput> Output = MakeShared<FFlowFieldBuildOutput>();
        UltraFlowField::BuildDistanceAndFlow(Job->Input, *Output);

        if (!Job->bCancelled)
        {
            CompletedJobs.Enqueue(Output);
            Job->CompletionDelegate.ExecuteIfBound(Output);
        }

        FScopeLock Scope(&ActiveLock);
        ActiveJobs.Remove(Job);
    });
}

TSharedPtr<FFlowFieldBuildOutput> RunFlowFieldBuildImmediately(const FFlowFieldBuildInput& Input)
{
    TSharedPtr<FFlowFieldBuildOutput> Output = MakeShared<FFlowFieldBuildOutput>();
    UltraFlowField::BuildDistanceAndFlow(Input, *Output);
    return Output;
}

