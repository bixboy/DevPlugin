// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CharacterInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UCharacterInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class STELLARLOCOMOTIONCHARACTERSMODULE_API ICharacterInterface
{
	GENERATED_BODY()

	
	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void StellarUpdateUnrealPhysicVelocity(FVector& movementVelocity, const float& deltaTime);
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void StellarUpdateVelocity(FVector& gravityVelocity, FVector& movementVelocity, const float& deltaTime);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void StellarUpdateRotation(FQuat& currentRotation, const float& deltaTime);
};
