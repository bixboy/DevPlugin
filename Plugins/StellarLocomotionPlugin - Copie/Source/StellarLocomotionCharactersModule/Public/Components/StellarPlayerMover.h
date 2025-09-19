// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UtilitiesClasses/RaycastSensor.h"
#include "StellarPlayerMover.generated.h"

class UCapsuleComponent;
/**
 * 
 */
UCLASS()
class STELLARLOCOMOTIONCHARACTERSMODULE_API UStellarPlayerMover : public UObject
{
	GENERATED_BODY()

	// COLLIDER SETTINGS
	UPROPERTY()
	float stepHeightRatio = 0.1f;

	UPROPERTY()
	float colliderHeight = 2.0f;

	UPROPERTY()
	float colliderThickness = 1.0f;

	
	UPROPERTY()
	FVector colliderOffset = FVector::ZeroVector;

	UPROPERTY()
	UCapsuleComponent* rigidBody;

	UPROPERTY()
	FTransform Tr;

	UPROPERTY()
	URaycastSensor* sensor;
	
	UPROPERTY()
	bool isGrounded;

	UPROPERTY()
	float baseSensorRange;

	UPROPERTY()
	FVector currentGroundAdjustmentVelocity;

	UPROPERTY()
	int currentLayer;


	// SENSOR SETTINGS
	UPROPERTY()
	bool isInDebugMode;
};