#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "FlowFieldTestActor.generated.h"

class AFlowFieldManager;

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
        /** Picks a new destination around the actor and rebuilds the flow field towards it. */
        bool TryAssignNewDestination();

        /** Returns true if the actor reached the current destination. */
        bool HasReachedDestination() const;

        /** Attempts to find a flow field manager in the current world. */
        AFlowFieldManager* ResolveFlowFieldManager();

private:
        /** Flow field manager that will be used to drive this actor. */
        UPROPERTY(EditInstanceOnly, Category = "Flow Field")
        AFlowFieldManager* FlowFieldManager = nullptr;

        /** Whether the actor should automatically look for a manager if none is assigned. */
        UPROPERTY(EditAnywhere, Category = "Flow Field")
        bool bAutoFindManager = true;

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
        FVector CurrentDestination = FVector::ZeroVector;
        bool bHasDestination = false;
};
