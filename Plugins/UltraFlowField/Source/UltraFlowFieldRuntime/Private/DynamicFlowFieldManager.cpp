#include "DynamicFlowFieldManager.h"
#include "FlowVoxelGridComponent.h"
#include "Async/Async.h"
#include "DrawDebugHelpers.h"
#include "Components/BillboardComponent.h"
#include "Engine/World.h"

ADynamicFlowFieldManager::ADynamicFlowFieldManager()
{
    PrimaryActorTick.bCanEverTick = true;
    VoxelGrid = CreateDefaultSubobject<UFlowVoxelGridComponent>(TEXT("VoxelGrid"));
    SpriteComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
    RootComponent = SpriteComponent;
}

void ADynamicFlowFieldManager::BeginPlay()
{
    Super::BeginPlay();
    if (VoxelGrid)
    {
        VoxelGrid->InitializeIfNeeded();
    }
}

void ADynamicFlowFieldManager::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    HandleCompletedJobs();

    if (bEnableDebugDraw)
    {
        DrawDebugInfo();
    }
}

void ADynamicFlowFieldManager::SetGlobalTarget(const FVector& Target)
{
    CurrentTarget = Target;

    if (!VoxelGrid)
    {
        return;
    }

    FFlowFieldBuildInput Input;
    VoxelGrid->FillBuildInput(Input);
    Input.Target = Target;
    Input.Layer = ActiveLayer;

    TSharedRef<FFlowFieldJob> Job = MakeShared<FFlowFieldJob>();
    Job->Input = MoveTemp(Input);
    Job->CompletionDelegate = FFlowFieldBuildCompleted::CreateLambda([this](TSharedPtr<FFlowFieldBuildOutput> Result)
    {
        if (Result.IsValid())
        {
            AsyncTask(ENamedThreads::GameThread, [this, Result]()
            {
                SwapBuffers(Result);
            });
        }
    });

    Scheduler.EnqueueBuild(Job);
}

void ADynamicFlowFieldManager::SetAgentLayer(EAgentLayer InLayer)
{
    ActiveLayer = InLayer;
    RequestRebuild();
}

void ADynamicFlowFieldManager::MarkObstacleBox(const FVector& Center, const FVector& Extents, bool bBlocked)
{
    if (!VoxelGrid)
    {
        return;
    }

    VoxelGrid->SetObstacleBox(Center, Extents, bBlocked);
    RequestRebuild();
}

void ADynamicFlowFieldManager::SetDebugEnabled(bool bEnabled)
{
    bEnableDebugDraw = bEnabled;
}

FVector ADynamicFlowFieldManager::GetFlowDirectionAt(const FVector& WorldLocation, EAgentLayer Layer) const
{
    if (!VoxelGrid)
    {
        return FVector::ZeroVector;
    }

    FIntVector ChunkCoord;
    FIntVector LocalVoxel;
    if (!VoxelGrid->WorldToVoxel(WorldLocation, ChunkCoord, LocalVoxel))
    {
        return FVector::ZeroVector;
    }

    FReadScopeLock Scope(BufferLock);
    if (const TSharedPtr<FFlowFieldBuffer>* BufferPtr = FrontBuffers.Find(ChunkCoord))
    {
        const TSharedPtr<FFlowFieldBuffer>& Buffer = *BufferPtr;
        if (Buffer.IsValid())
        {
            const int32 Index = LocalVoxel.Z * Buffer->Dimensions.Y * Buffer->Dimensions.X + LocalVoxel.Y * Buffer->Dimensions.X + LocalVoxel.X;
            if (Buffer->IsValidIndex(Index))
            {
                const FIntVector Packed = Buffer->PackedDirections[Index];
                FVector Dir = FVector((float)Packed.X, (float)Packed.Y, (float)Packed.Z);
                if (!Dir.IsNearlyZero())
                {
                    Dir = Dir.GetSafeNormal();
                }
                return Dir;
            }
        }
    }

    return FVector::ZeroVector;
}

void ADynamicFlowFieldManager::ToggleThrottle(bool bEnabled)
{
    bThrottle = bEnabled;
}

void ADynamicFlowFieldManager::RequestRebuild()
{
    SetGlobalTarget(CurrentTarget);
}

void ADynamicFlowFieldManager::HandleCompletedJobs()
{
    Scheduler.Tick();
}

void ADynamicFlowFieldManager::SwapBuffers(TSharedPtr<FFlowFieldBuildOutput> Output)
{
    if (!Output.IsValid())
    {
        return;
    }

    FWriteScopeLock Scope(BufferLock);
    BackBuffers.Reset();
    for (const auto& Pair : Output->ChunkResults)
    {
        TSharedPtr<FFlowFieldBuffer> Buffer = MakeShared<FFlowFieldBuffer>(Pair.Value);
        BackBuffers.Add(Pair.Key, Buffer);
    }

    FrontBuffers = MoveTemp(BackBuffers);
}

void ADynamicFlowFieldManager::DrawDebugInfo()
{
    if (!VoxelGrid)
    {
        return;
    }

    VoxelGrid->ForEachChunk([this](const FIntVector& ChunkCoord, FFlowVoxelChunk& Chunk)
    {
        if (!FrontBuffers.Contains(ChunkCoord))
        {
            return;
        }

        const FVector ChunkOriginWorld = VoxelGrid->VoxelToWorldCenter(ChunkCoord, FIntVector::ZeroValue) - FVector(VoxelGrid->GetVoxelSize() * 0.5f);

        for (int32 Z = 0; Z < Chunk.Dimensions.Z; ++Z)
        {
            for (int32 Y = 0; Y < Chunk.Dimensions.Y; ++Y)
            {
                for (int32 X = 0; X < Chunk.Dimensions.X; ++X)
                {
                    if ((X + Y + Z) % 8 != 0)
                    {
                        continue;
                    }

                    const FVector VoxelWorld = ChunkOriginWorld + FVector(X, Y, Z) * VoxelGrid->GetVoxelSize() + FVector(VoxelGrid->GetVoxelSize() * 0.5f);
                    const FVector Dir = GetFlowDirectionAt(VoxelWorld, ActiveLayer);
                    if (!Dir.IsNearlyZero())
                    {
                        DrawDebugDirectionalArrow(GetWorld(), VoxelWorld, VoxelWorld + Dir * VoxelGrid->GetVoxelSize(), 20.f, FColor::Green, false, -1.f, 0, 2.f);
                    }
                }
            }
        }
    });
}

