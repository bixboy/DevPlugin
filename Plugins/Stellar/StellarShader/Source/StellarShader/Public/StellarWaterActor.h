// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "StellarWaterActor.generated.h"

class UStellarWaterComponent;

/**
 * A ready-to-use water plane actor with the StellarWaterComponent attached.
 * Generates a highly tessellated procedural mesh so WPO works perfectly out of the box.
 */
UCLASS()
class STELLARSHADER_API AStellarWaterActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AStellarWaterActor();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	/** Total width of the water plane in cm (default 100m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Geometry", meta = (ClampMin = "100", UIMin = "1000", UIMax = "100000"))
	float WaterSizeX = 10000.0f;

	/** Total length of the water plane in cm (default 100m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Geometry", meta = (ClampMin = "100", UIMin = "1000", UIMax = "100000"))
	float WaterSizeY = 10000.0f;

	/** Distance between vertices in cm (lower = more geometry, smoother waves) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Geometry", meta = (ClampMin = "10", UIMin = "50", UIMax = "1000"))
	float VertexDistance = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	TObjectPtr<UProceduralMeshComponent> WaterMesh;

	/** Water shader controller — configure all water parameters here */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	TObjectPtr<UStellarWaterComponent> WaterComponent;
};
