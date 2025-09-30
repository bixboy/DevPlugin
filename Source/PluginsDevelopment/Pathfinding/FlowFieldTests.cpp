#if WITH_DEV_AUTOMATION_TESTS

#include "FlowField.h"
#include "Misc/AutomationTest.h"

namespace
{
        int32 ToIndex(const FFlowFieldSettings& Settings, int32 X, int32 Y)
        {
                return Y * Settings.GridSize.X + X;
        }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlowFieldBasicRoutingTest, "PluginsDevelopment.FlowField.BasicRouting", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlowFieldUnreachableTest, "PluginsDevelopment.FlowField.Unreachable", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlowFieldBasicRoutingTest::RunTest(const FString& Parameters)
{
        FFlowFieldSettings Settings;
        Settings.GridSize = FIntPoint(5, 5);
        Settings.CellSize = 100.0f;
        Settings.Origin = FVector::ZeroVector;
        Settings.bAllowDiagonal = true;
        Settings.bSmoothDirections = false;

        FFlowField Flow(Settings);

        TArray<uint8> Weights;
        Weights.Init(255, Settings.GridSize.X * Settings.GridSize.Y);

        // Create a wall leaving a single gap to validate that the flow field routes through it.
        Weights[ToIndex(Settings, 2, 0)] = 0;
        Weights[ToIndex(Settings, 2, 1)] = 0;
        Weights[ToIndex(Settings, 2, 3)] = 0;
        Weights[ToIndex(Settings, 2, 4)] = 0;

        Flow.SetTraversalWeights(Weights);

        const bool bBuilt = Flow.Build(FIntPoint(4, 2));
        TestTrue(TEXT("Flow field build succeeded"), bBuilt);

        if (bBuilt)
        {
                const FVector2D DirectionOrigin = Flow.GetDirectionForCell(FIntPoint(0, 2));
                TestTrue(TEXT("Direction points towards the east"), DirectionOrigin.X > 0.9f && FMath::IsNearlyZero(DirectionOrigin.Y, 0.2f));

                const FVector2D DirectionNorth = Flow.GetDirectionForCell(FIntPoint(1, 1));
                TestTrue(TEXT("Direction routes around the wall"), DirectionNorth.Y > 0.2f);

                const FVector SampleWorld = Flow.CellToWorld(FIntPoint(1, 1));
                const FIntPoint SampleCell = Flow.WorldToCell(SampleWorld);
                TestEqual(TEXT("World/Cell mapping"), SampleCell, FIntPoint(1, 1));

                const FVector WorldDirection = Flow.GetDirectionForWorldPosition(SampleWorld);
                TestTrue(TEXT("World direction has zero Z"), FMath::IsNearlyZero(WorldDirection.Z));

                const FFlowFieldDebugSnapshot Snapshot = Flow.CreateDebugSnapshot();
                TestEqual(TEXT("Snapshot integration size"), Snapshot.IntegrationField.Num(), Settings.GridSize.X * Settings.GridSize.Y);
                TestEqual(TEXT("Snapshot direction size"), Snapshot.DirectionField.Num(), Settings.GridSize.X * Settings.GridSize.Y);
                TestEqual(TEXT("Snapshot traversal size"), Snapshot.WalkableField.Num(), Settings.GridSize.X * Settings.GridSize.Y);
                TestEqual(TEXT("Snapshot destination"), Snapshot.Destination, FIntPoint(4, 2));
        }

        return true;
}

bool FFlowFieldUnreachableTest::RunTest(const FString& Parameters)
{
        FFlowFieldSettings Settings;
        Settings.GridSize = FIntPoint(3, 3);
        Settings.CellSize = 100.0f;
        Settings.bAllowDiagonal = false;

        FFlowField Flow(Settings);

        TArray<uint8> Weights;
        Weights.Init(0, Settings.GridSize.X * Settings.GridSize.Y);
        Weights[ToIndex(Settings, 1, 1)] = 255;

        Flow.SetTraversalWeights(Weights);

        const bool bBuilt = Flow.Build(FIntPoint(1, 1));
        TestTrue(TEXT("Flow field build for single cell"), bBuilt);

        const FVector2D Direction = Flow.GetDirectionForCell(FIntPoint(0, 0));
        TestTrue(TEXT("Unreachable cells have no direction"), Direction.IsNearlyZero());

        return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
