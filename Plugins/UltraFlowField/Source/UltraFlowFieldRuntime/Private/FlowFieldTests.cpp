#include "Misc/AutomationTest.h"
#include "FlowVoxelGridComponent.h"
#include "FlowFieldScheduler.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlowFieldSingleGoalTest, "UltraFlowField.Distance.SingleGoal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlowFieldObstacleTest, "UltraFlowField.Distance.Obstacle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlowFieldSingleGoalTest::RunTest(const FString& Parameters)
{
    UFlowVoxelGridComponent* Grid = NewObject<UFlowVoxelGridComponent>(GetTransientPackage());
    Grid->Settings.VoxelSize = 100.f;
    Grid->Settings.ChunkSizeX = 8;
    Grid->Settings.ChunkSizeY = 8;
    Grid->Settings.ChunkSizeZ = 1;
    Grid->Settings.WorldBounds = FBox(FVector(-400), FVector(400));
    Grid->InitializeIfNeeded();

    FFlowFieldBuildInput Input;
    Grid->FillBuildInput(Input);
    Input.Target = FVector::ZeroVector;
    Input.Layer = EAgentLayer::Ground;

    TSharedPtr<FFlowFieldBuildOutput> Output = RunFlowFieldBuildImmediately(Input);
    TestTrue(TEXT("Output valid"), Output.IsValid());

    FIntVector ChunkCoord, LocalVoxel;
    const bool bVoxelValid = Grid->WorldToVoxel(FVector(300, 0, 0), ChunkCoord, LocalVoxel);
    TestTrue(TEXT("World to voxel"), bVoxelValid);

    if (bVoxelValid && Output.IsValid())
    {
        if (const FFlowFieldBuffer* Buffer = Output->ChunkResults.Find(ChunkCoord))
        {
            const int32 Index = LocalVoxel.Z * Buffer->Dimensions.Y * Buffer->Dimensions.X + LocalVoxel.Y * Buffer->Dimensions.X + LocalVoxel.X;
            if (Buffer->IsValidIndex(Index))
            {
                const FIntVector Packed = Buffer->PackedDirections[Index];
                const FVector Dir = FVector((float)Packed.X, (float)Packed.Y, (float)Packed.Z);
                TestTrue(TEXT("Direction computed"), !Dir.IsNearlyZero());
            }
            else
            {
                AddError(TEXT("Invalid voxel index"));
            }
        }
        else
        {
            AddError(TEXT("Chunk missing from output"));
        }
    }

    return true;
}

bool FFlowFieldObstacleTest::RunTest(const FString& Parameters)
{
    UFlowVoxelGridComponent* Grid = NewObject<UFlowVoxelGridComponent>(GetTransientPackage());
    Grid->Settings.VoxelSize = 100.f;
    Grid->Settings.ChunkSizeX = 8;
    Grid->Settings.ChunkSizeY = 8;
    Grid->Settings.ChunkSizeZ = 1;
    Grid->Settings.WorldBounds = FBox(FVector(-400), FVector(400));
    Grid->InitializeIfNeeded();

    Grid->SetObstacleBox(FVector(100, 0, 0), FVector(50, 50, 50), true);

    FFlowFieldBuildInput Input;
    Grid->FillBuildInput(Input);
    Input.Target = FVector::ZeroVector;
    Input.Layer = EAgentLayer::Ground;

    TSharedPtr<FFlowFieldBuildOutput> Output = RunFlowFieldBuildImmediately(Input);
    TestTrue(TEXT("Output valid"), Output.IsValid());

    FIntVector ChunkCoord, LocalVoxel;
    const bool bVoxelValid = Grid->WorldToVoxel(FVector(300, 0, 0), ChunkCoord, LocalVoxel);
    TestTrue(TEXT("World to voxel"), bVoxelValid);

    if (bVoxelValid && Output.IsValid())
    {
        if (const FFlowFieldBuffer* Buffer = Output->ChunkResults.Find(ChunkCoord))
        {
            const int32 Index = LocalVoxel.Z * Buffer->Dimensions.Y * Buffer->Dimensions.X + LocalVoxel.Y * Buffer->Dimensions.X + LocalVoxel.X;
            if (Buffer->IsValidIndex(Index))
            {
                const FIntVector Packed = Buffer->PackedDirections[Index];
                const FVector Dir = FVector((float)Packed.X, (float)Packed.Y, (float)Packed.Z);
                TestTrue(TEXT("Direction avoids obstacle"), !Dir.Equals(FVector::ForwardVector));
            }
            else
            {
                AddError(TEXT("Invalid voxel index"));
            }
        }
        else
        {
            AddError(TEXT("Chunk missing from output"));
        }
    }

    return true;
}

