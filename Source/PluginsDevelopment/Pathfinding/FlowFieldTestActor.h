#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "FlowField.h"

#include "FlowFieldTestActor.generated.h"

/**
 * Simple actor that roams around by following a flow field towards random destinations.
 */
UCLASS()
class PLUGINSDEVELOPMENT_API AFlowFieldTestActor : public AActor
{
	GENERATED_BODY()

public:
	AFlowFieldTestActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

private:
	/** Rebuilds the flow field settings and traversal weights around the actor. */
	void UpdateFlowFieldSettings();

	/** Picks a new destination around the actor and rebuilds the flow field towards it. */
	bool TryAssignNewDestination();

	/** Returns true if the actor reached the current destination. */
	bool HasReachedDestination() const;

private:
	/** Size of the flow field grid. */
	UPROPERTY(EditAnywhere, Category = "Flow Field")
	FIntPoint GridSize = FIntPoint(32, 32);

	/** Size of each flow field cell in world units. */
	UPROPERTY(EditAnywhere, Category = "Flow Field")
	float CellSize = 200.0f;

	/** Whether diagonal movement is allowed when generating the flow field. */
	UPROPERTY(EditAnywhere, Category = "Flow Field")
	bool bAllowDiagonal = true;

	/** Cost multiplier for diagonal movement. */
	UPROPERTY(EditAnywhere, Category = "Flow Field")
	float DiagonalCostMultiplier = 1.41421356f;

	/** Base traversal cost for every step. */
	UPROPERTY(EditAnywhere, Category = "Flow Field")
	float StepCost = 1.0f;

	/** Traversal weight multiplier applied to each cell. */
	UPROPERTY(EditAnywhere, Category = "Flow Field")
	float CellTraversalWeightMultiplier = 1.0f;

	/** Additional heuristic weight applied when evaluating neighbours. */
	UPROPERTY(EditAnywhere, Category = "Flow Field")
	float HeuristicWeight = 1.0f;

	/** Whether the field should smooth the resulting directions. */
	UPROPERTY(EditAnywhere, Category = "Flow Field")
	bool bSmoothDirections = true;

	/** Search radius (in world units) used when picking a new random destination. */
	UPROPERTY(EditAnywhere, Category = "Flow Field")
	float DestinationSearchRadius = 2000.0f;

	/** Distance threshold used to consider the destination reached. */
	UPROPERTY(EditAnywhere, Category = "Flow Field")
	float AcceptanceRadius = 100.0f;

	/** Movement speed while following the flow field. */
	UPROPERTY(EditAnywhere, Category = "Flow Field")
	float MovementSpeed = 600.0f;

	/** Maximum number of attempts when searching for a new destination. */
	UPROPERTY(EditAnywhere, Category = "Flow Field")
	int32 MaxDestinationAttempts = 24;

private:
	FFlowField FlowField;
	FFlowFieldSettings FlowFieldSettings;
	TArray<uint8> TraversalWeights;
	FVector CurrentDestination = FVector::ZeroVector;
	FVector AnchorLocation = FVector::ZeroVector;
	bool bHasDestination = false;
};
