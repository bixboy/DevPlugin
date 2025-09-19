// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StellarCharacterMotor.h"
#include "UObject/Object.h"
#include "StellarPhysicCharacterMotor.generated.h"

/**
 * 
 */
UCLASS()
class STELLARLOCOMOTIONCHARACTERSMODULE_API UStellarPhysicCharacterMotor : public UStellarCharacterMotor
{
	GENERATED_BODY()


public:
	virtual void Tick(const float& DeltaTime) override;


	UFUNCTION()
	void UpdateRotation(float DeltaTime);
	UFUNCTION()
	void UpdateMovements(float DeltaTime);
	UFUNCTION()
	void ApplyTransform(float DeltaTime);
};
