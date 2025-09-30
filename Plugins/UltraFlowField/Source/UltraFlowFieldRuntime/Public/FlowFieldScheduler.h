#pragma once

#include "CoreMinimal.h"
#include "FlowFieldTypes.h"
#include "FlowFieldScheduler.generated.h"

class UFlowVoxelGridComponent;

USTRUCT()
struct FFlowFieldBuildInput
{
    GENERATED_BODY()

    FVector Target = FVector::ZeroVector;
    EAgentLayer Layer = EAgentLayer::Ground;
    float VoxelSize = 100.f;
    FIntVector ChunkSize = FIntVector(32, 32, 16);
    TMap<FIntVector, FFlowVoxelChunk> ChunkSnapshots;
};

USTRUCT()
struct FFlowFieldBuildOutput
{
    GENERATED_BODY()

    TMap<FIntVector, FFlowFieldBuffer> ChunkResults;
    FVector Target = FVector::ZeroVector;
    EAgentLayer Layer = EAgentLayer::Ground;
};

DECLARE_DELEGATE_OneParam(FFlowFieldBuildCompleted, TSharedPtr<FFlowFieldBuildOutput> /*Result*/);

struct FFlowFieldJob
{
    FFlowFieldBuildInput Input;
    FFlowFieldBuildCompleted CompletionDelegate;
    FThreadSafeBool bCancelled = false;
};

class FFlowFieldScheduler
{
public:
    FFlowFieldScheduler();
    ~FFlowFieldScheduler();

    void EnqueueBuild(TSharedRef<FFlowFieldJob> Job);
    void CancelAll();

    void Tick();

private:
    void LaunchJob(const TSharedRef<FFlowFieldJob>& Job);

    TQueue<TSharedRef<FFlowFieldJob>, EQueueMode::Mpsc> PendingJobs;
    TQueue<TSharedPtr<FFlowFieldBuildOutput>, EQueueMode::Mpsc> CompletedJobs;

    FCriticalSection ActiveLock;
    TSet<TSharedRef<FFlowFieldJob>> ActiveJobs;
};

ULTRAFLOWFIELDRUNTIME_API TSharedPtr<FFlowFieldBuildOutput> RunFlowFieldBuildImmediately(const FFlowFieldBuildInput& Input);

