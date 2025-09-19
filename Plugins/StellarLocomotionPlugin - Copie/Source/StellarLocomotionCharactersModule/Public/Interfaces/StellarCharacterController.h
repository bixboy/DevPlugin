// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StellarCharacterController.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UStellarCharacterController : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class STELLARLOCOMOTIONCHARACTERSMODULE_API IStellarCharacterController
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void UpdateVelocity(FVector& gravityVelocity, FVector& movementVelocity, const float& deltaTime);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void UpdateRotation(FQuat& currentRotation, const float& deltaTime);
};
