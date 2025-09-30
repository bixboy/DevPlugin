#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h"
#include "UObject/ObjectPtr.h"
#include "Containers/BitArray.h"
#include "Containers/Queue.h"
#include "HAL/CriticalSection.h"

#include "FlowField.h"

#include "FlowFieldManager.generated.h"

namespace FlowField
{
        struct FTraversalEvaluationContext;
}

class FTraversalWeightBuildTask;
template <typename TaskType>
class FAsyncTask;

/**
 * Centralised manager that owns a flow field and exposes debug utilities to visualise it in the level.
 */
UCLASS()
class PLUGINSDEVELOPMENT_API AFlowFieldManager : public AActor
{
        GENERATED_BODY()

public:
        AFlowFieldManager();

        virtual void Tick(float DeltaSeconds) override;
        virtual void OnConstruction(const FTransform& Transform) override;
        virtual void BeginPlay() override;

        /** Rebuilds the flow field using the supplied destination cell. */
        UFUNCTION(BlueprintCallable, Category = "Flow Field")
        bool BuildFlowFieldToCell(const FIntPoint& DestinationCell);

        /** Rebuilds the flow field using the supplied world destination. */
        UFUNCTION(BlueprintCallable, Category = "Flow Field")
        bool BuildFlowFieldToWorldLocation(const FVector& DestinationLocation);

        /** Rebuilds the flow field using the supplied world destination, ensuring traversal data is up to date. */
        UFUNCTION(BlueprintCallable, Category = "Flow Field")
        bool RebuildFlowFieldTo(const FVector& DestinationWorld);

        /** Marks or clears an obstacle inside the flow field grid. */
        UFUNCTION(BlueprintCallable, Category = "Flow Field")
        void MarkObstacleBox(const FVector& Center, const FVector& Extents, bool bBlocked);

        /** Samples the flow direction for a given world position. */
        UFUNCTION(BlueprintCallable, Category = "Flow Field")
        FVector GetDirectionForWorldPosition(const FVector& WorldPosition) const;

        /** Returns the current flow field settings. */
        const FFlowFieldSettings& GetSettings() const { return FlowFieldSettings; }

        /** Converts a world position into a cell coordinate using the managed flow field. */
        FIntPoint WorldToCell(const FVector& WorldPosition) const { return FlowField.WorldToCell(WorldPosition); }

        /** Converts a cell coordinate into a world position using the managed flow field. */
        FVector CellToWorld(const FIntPoint& Cell) const { return FlowField.CellToWorld(Cell); }

        /** Returns true if the supplied cell is walkable within the current traversal weights. */
        bool IsWalkable(const FIntPoint& Cell) const { return FlowField.IsWalkable(Cell); }

        /** Returns true if the manager currently has a valid built field. */
        bool HasValidField() const { return bHasBuiltField; }

protected:
        void UpdateFlowFieldSettings();
        void UpdateTraversalWeights();
        void RefreshDebugSnapshot();
        void DrawDebug() const;

private:
        /** Automatically resize the flow field to fit the terrain bounds. */
        UPROPERTY(EditAnywhere, Category = "Flow Field")
        bool bAutoSizeToTerrain = true;

        /** Actors that describe the terrain to fit. When empty, landscapes in the level will be used. */
        UPROPERTY(EditInstanceOnly, Category = "Flow Field", meta = (EditCondition = "bAutoSizeToTerrain"))
        TArray<TObjectPtr<AActor>> TerrainActors;

        /** Additional padding (in world units) applied around the detected terrain bounds. */
        UPROPERTY(EditAnywhere, Category = "Flow Field", meta = (EditCondition = "bAutoSizeToTerrain"))
        float TerrainBoundsPadding = 0.0f;

        /** Height above the terrain used to start downward traces when evaluating walkable cells. */
        UPROPERTY(EditAnywhere, Category = "Flow Field", meta = (EditCondition = "bAutoSizeToTerrain", ClampMin = "0.0"))
        float TerrainTraceHeight = 5000.0f;

        /** Depth below the terrain used when tracing for walkable cells. */
        UPROPERTY(EditAnywhere, Category = "Flow Field", meta = (EditCondition = "bAutoSizeToTerrain", ClampMin = "0.0"))
        float TerrainTraceDepth = 2000.0f;

        /** Collision channel used when tracing against the terrain. */
        UPROPERTY(EditAnywhere, Category = "Flow Field", meta = (EditCondition = "bAutoSizeToTerrain"))
        TEnumAsByte<ECollisionChannel> TerrainCollisionChannel = ECC_WorldStatic;

        /** Maximum slope angle (in degrees) that is considered walkable. */
        UPROPERTY(EditAnywhere, Category = "Flow Field", meta = (ClampMin = "0.0", ClampMax = "90.0"))
        float MaxWalkableSlopeAngle = 45.0f;

        /** Collision channel used when querying for blocking obstacles. */
        UPROPERTY(EditAnywhere, Category = "Flow Field|Obstacles")
        TEnumAsByte<ECollisionChannel> ObstacleCollisionChannel = ECC_GameTraceChannel2;

        /** Half height of the obstacle overlap volume. */
        UPROPERTY(EditAnywhere, Category = "Flow Field|Obstacles", meta = (ClampMin = "0.0"))
        float ObstacleQueryHalfHeight = 120.0f;

        /** Additional padding applied to the XY size of the obstacle query. */
        UPROPERTY(EditAnywhere, Category = "Flow Field|Obstacles", meta = (ClampMin = "0.0"))
        float ObstacleQueryInflation = 15.0f;

        /** Vertical offset applied to the obstacle overlap volume origin. */
        UPROPERTY(EditAnywhere, Category = "Flow Field|Obstacles")
        float ObstacleQueryOffsetZ = 50.0f;

        /** Size of the flow field grid. */
        UPROPERTY(EditAnywhere, Category = "Flow Field")
        FIntPoint GridSize = FIntPoint(32, 32);

        /** Size of each cell in world units. */
        UPROPERTY(EditAnywhere, Category = "Flow Field", meta = (ClampMin = "1"))
        float CellSize = 200.0f;

        /** Whether diagonal movement is allowed when generating the flow field. */
        UPROPERTY(EditAnywhere, Category = "Flow Field")
        bool bAllowDiagonal = true;

        /** Cost multiplier for diagonal movement. */
        UPROPERTY(EditAnywhere, Category = "Flow Field", meta = (ClampMin = "0.0"))
        float DiagonalCostMultiplier = 1.41421356f;

        /** Base traversal cost for every step. */
        UPROPERTY(EditAnywhere, Category = "Flow Field", meta = (ClampMin = "0.0"))
        float StepCost = 1.0f;

        /** Traversal weight multiplier applied to each cell. */
        UPROPERTY(EditAnywhere, Category = "Flow Field", meta = (ClampMin = "0.0"))
        float CellTraversalWeightMultiplier = 1.0f;

        /** Additional heuristic weight applied when evaluating neighbours. */
        UPROPERTY(EditAnywhere, Category = "Flow Field", meta = (ClampMin = "0.0"))
        float HeuristicWeight = 1.0f;

        /** Whether the field should smooth the resulting directions. */
        UPROPERTY(EditAnywhere, Category = "Flow Field")
        bool bSmoothDirections = true;

        /** Optional offset applied after centering the grid around the actor location. */
        UPROPERTY(EditAnywhere, Category = "Flow Field")
        FVector AdditionalOriginOffset = FVector::ZeroVector;

        /** Default traversal weight assigned to each cell. */
        UPROPERTY(EditAnywhere, Category = "Flow Field", meta = (ClampMin = "0", ClampMax = "255"))
        int32 DefaultTraversalWeight = 255;

        /** Height offset applied when drawing debug information. */
        UPROPERTY(EditAnywhere, Category = "Debug")
        float DebugHeightOffset = 50.0f;

        /** Enables debug visualisation of the flow field. */
        UPROPERTY(EditAnywhere, Category = "Debug")
        bool bEnableDebugDraw = true;

        /** Draws the grid bounds when debug draw is enabled. */
        UPROPERTY(EditAnywhere, Category = "Debug", meta = (EditCondition = "bEnableDebugDraw"))
        bool bDrawGrid = true;

        /** Draws direction arrows for each cell. */
        UPROPERTY(EditAnywhere, Category = "Debug", meta = (EditCondition = "bEnableDebugDraw"))
        bool bDrawDirections = true;

        /** Draws integration costs for each cell. */
        UPROPERTY(EditAnywhere, Category = "Debug", meta = (EditCondition = "bEnableDebugDraw"))
        bool bDrawIntegrationValues = false;

        /** Draws blocked cells with a dedicated colour. */
        UPROPERTY(EditAnywhere, Category = "Debug", meta = (EditCondition = "bEnableDebugDraw"))
        bool bHighlightBlockedCells = true;

        /** Draws the current destination location. */
        UPROPERTY(EditAnywhere, Category = "Debug", meta = (EditCondition = "bEnableDebugDraw"))
        bool bDrawDestination = true;

        /** Colour used when drawing flow directions. */
        UPROPERTY(EditAnywhere, Category = "Debug")
        FColor DirectionColor = FColor::Green;

        /** Colour used when drawing integration cost text. */
        UPROPERTY(EditAnywhere, Category = "Debug")
        FColor IntegrationTextColor = FColor::White;

        /** Colour used to highlight blocked cells. */
        UPROPERTY(EditAnywhere, Category = "Debug")
        FColor BlockedCellColor = FColor::Red;

        /** Colour used to draw the grid lines. */
        UPROPERTY(EditAnywhere, Category = "Debug")
        FColor GridColor = FColor::Cyan;

        /** Thickness applied when drawing debug lines. */
        UPROPERTY(EditAnywhere, Category = "Debug", meta = (ClampMin = "0.0"))
        float DebugLineThickness = 2.0f;

        /** Length of the direction arrows relative to the cell size. */
        UPROPERTY(EditAnywhere, Category = "Debug", meta = (ClampMin = "0.0"))
        float DirectionArrowScale = 0.4f;

        /** Scale applied to debug text. */
        UPROPERTY(EditAnywhere, Category = "Debug", meta = (ClampMin = "0.1"))
        float DebugTextScale = 1.0f;

        /** Radius used when drawing the destination marker. */
        UPROPERTY(EditAnywhere, Category = "Debug", meta = (ClampMin = "0.0"))
        float DestinationMarkerRadius = 75.0f;

        /** Colour used when rendering blocked obstacles. */
        UPROPERTY(EditAnywhere, Category = "Debug")
        FColor ObstacleDebugColor = FColor::Red;

private:
        FFlowField FlowField;
        FFlowFieldSettings FlowFieldSettings;
        TArray<uint8> TraversalWeights;
        TArray<uint8> ManualObstacleMask;
        TArray<uint8> ObstacleDebugMask;
        FFlowFieldDebugSnapshot DebugSnapshot;
        bool bHasBuiltField = false;
        bool bHasDebugSnapshot = false;
        FVector CachedDestinationWorld = FVector::ZeroVector;
        FBox CachedTerrainBounds = FBox(ForceInit);

        TBitArray<> DirtyCellMask;
        TQueue<int32> DirtyCellQueue;
        mutable FCriticalSection TraversalDataLock;
        bool bTraversalInitialised = false;
        bool bTraversalWeightsDirtyForField = false;

        TUniquePtr<FAsyncTask<FTraversalWeightBuildTask>> ActiveTraversalTask;

        void RecalculateTerrainBounds();

        void ServiceTraversalWeightUpdates(float DeltaSeconds);
        void MarkAllCellsDirty();
        void MarkRegionDirty(const FIntPoint& MinCell, const FIntPoint& MaxCell);
        void MarkCellDirty(int32 LinearIndex);
        int32 ProcessDirtyCells(int32 MaxCellsToProcess);
        void FlushTraversalWeightsToField();
        void CancelPendingAsyncTask();
        void ConsumeTraversalTask();
        bool HasPendingTraversalUpdates() const;
        void EnsureTraversalWeightsUpToDate(bool bForceSync);
        bool IsCellIndexValid(int32 Index) const;
        FIntPoint ClampCellToGrid(const FIntPoint& Cell) const;

        uint8 SampleTraversalWeightForCell(int32 CellIndex, bool& bOutBlockedByObstacle) const;
        void ApplyTraversalWeight(int32 CellIndex, uint8 Weight, bool bBlockedByObstacle);
        FlowField::FTraversalEvaluationContext BuildTraversalContext(bool bCopyManualObstacles) const;
};

