#pragma once

#include "CoreMinimal.h"

/**
 * Settings used to build a flow field.
 */
struct PLUGINSDEVELOPMENT_API FFlowFieldSettings
{
        /** Number of cells along X and Y. */
        FIntPoint GridSize = FIntPoint(1, 1);

        /** Size of a cell in world units. */
        float CellSize = 100.f;

        /** Origin of the grid in world space. */
        FVector Origin = FVector::ZeroVector;

        /** Allow diagonal movement when computing the flow field. */
        bool bAllowDiagonal = true;

        /** Multiplier applied to diagonal moves. Typical value is sqrt(2). */
        float DiagonalCostMultiplier = 1.41421356f;

        /**
         * Base movement cost used for cardinal steps. Diagonal steps automatically use DiagonalCostMultiplier.
         * Increasing this value globally slows agents following the field but preserves directions.
         */
        float StepCost = 1.0f;

        /**
         * Cells can provide their own traversal weight (see SetTraversalWeights). The final movement cost becomes
         * StepCost * CellTraversalWeightMultiplier / NormalisedWeight.
         */
        float CellTraversalWeightMultiplier = 1.0f;

        /** Cells with a traversal weight of zero are considered fully blocked. */
        float BlockedCellCost = 1e8f;

        /**
         * Additional weight applied when evaluating neighbors.
         * Useful to bias agents to take shorter (lower integration) paths.
         */
        float HeuristicWeight = 1.0f;

        /** When true, the flow field will smooth the generated directions. */
        bool bSmoothDirections = true;
};

/** Debug snapshot of a flow field that can be inspected or visualised. */
struct PLUGINSDEVELOPMENT_API FFlowFieldDebugSnapshot
{
        TArray<float> IntegrationField;
        TArray<FVector2D> DirectionField;
        TArray<uint8> WalkableField;
        FIntPoint GridSize = FIntPoint::ZeroValue;
        float CellSize = 100.f;
        FVector Origin = FVector::ZeroVector;
        FIntPoint Destination = FIntPoint(-1, -1);
};

/** Lightweight grid based flow field implementation intended for large worlds. */
class PLUGINSDEVELOPMENT_API FFlowField
{
public:
        explicit FFlowField(const FFlowFieldSettings& InSettings = FFlowFieldSettings());

        /** Updates the settings and rebuilds the internal buffers if needed. */
        void ApplySettings(const FFlowFieldSettings& InSettings);

        /**
         * Sets the traversal weights for the field. Each entry represents a cell and is expected to be in the range [0, 255].
         * A value of 0 marks a blocked cell. Any other value is normalised to [0, 1] and inverted to contribute to the movement cost.
         * The number of entries must match GridSize.X * GridSize.Y.
         */
        void SetTraversalWeights(const TArray<uint8>& InWeights);

        /** Returns the current settings. */
        const FFlowFieldSettings& GetSettings() const { return Settings; }

        /** Returns true if the given cell is walkable. */
        bool IsWalkable(const FIntPoint& Cell) const;

        /** Builds the flow field given the destination cell. Returns false if the destination is invalid or blocked. */
        bool Build(const FIntPoint& DestinationCell);

        /** Returns the direction (normalised) for the supplied cell. */
        FVector2D GetDirectionForCell(const FIntPoint& Cell) const;

        /** Converts a world position into a cell coordinate. */
        FIntPoint WorldToCell(const FVector& WorldPosition) const;

        /** Converts a cell coordinate into a world position at the cell centre. */
        FVector CellToWorld(const FIntPoint& Cell) const;

        /** Samples the flow field direction for a given world position. */
        FVector GetDirectionForWorldPosition(const FVector& WorldPosition) const;

        /** Creates a debug snapshot that can be visualised or logged. */
        FFlowFieldDebugSnapshot CreateDebugSnapshot() const;

private:
        int32 ToLinearIndex(const FIntPoint& Cell) const;
        bool IsCellValid(const FIntPoint& Cell) const;
        void ResetFields();
        void RebuildFlowDirections();

        FFlowFieldSettings Settings;
        TArray<float> IntegrationField;
        TArray<FVector2D> DirectionField;
        TArray<uint8> TraversalWeights;
        FIntPoint Destination = FIntPoint(-1, -1);
};

