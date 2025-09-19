// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StellarCharacterMotor.h"
#include "UObject/Object.h"
#include "StellarCustomCharacterMotor.generated.h"

class UCapsuleComponent;

DECLARE_DYNAMIC_DELEGATE_OneParam(FStellarPostPhysicTick, float, DeltaTime);

/**
 * 
 */
UCLASS()
class STELLARLOCOMOTIONCHARACTERSMODULE_API UStellarCustomCharacterMotor : public UStellarCharacterMotor
{
	GENERATED_BODY()
	
	UPROPERTY()
	FTransform InitialRelativeTransform;

	UPROPERTY()
	FTransform CurrentOffsetTransform;
	
	UPROPERTY()
	FTransform LocalOffsetTransform;

	UPROPERTY()
	FTransform SimulatedChildTransform;

	UPROPERTY()
	FTransform FinalTransform;

	// UPROPERTY()
	// FBoxSphereBounds _boundingBox;
	
	UPROPERTY()
	FVector _gravityVelocity;
	
	UPROPERTY()
	FVector _finalVelocity;
	
	UPROPERTY()
	int _maxBounces = 5;
	
	UPROPERTY()
	float _maxStepHeight = 25.0f;
	
	UPROPERTY()
	float _skinWidth = 1.5f;

	UPROPERTY()
	float StepHeight = 40.0f;

	UPROPERTY()
	float StepCheckDistance = 5.0f;



public:
	virtual void Tick(const float& DeltaTime) override;
	
	virtual void SetCharacterEnvironement(AActor* env) override;

	
	UFUNCTION()
	void UpdateTransform(float DeltaTime);
	UFUNCTION()
	void UpdateRotation(float DeltaTime);
	UFUNCTION()
	void UpdateMovements(float DeltaTime);
	UFUNCTION()
	void ApplyTransform(float DeltaTime);

	virtual FVector GetGlobalForwardVector() override;
	virtual FVector GetGlobalRightVector() override;
	virtual FVector GetGlobalUpVector() override; 

	virtual FVector GetCharacterForwardVector() override;
	virtual FVector GetCharacterRightVector() override;
	virtual FVector GetCharacterUpVector() override;

	
	UFUNCTION()
	void Dev_UpdateMovement(FVector MovementInput);

	
public:
	virtual bool IsCharacterGrounded() override;

	FVector CollideAndSlide(FVector vel, FVector initialVel, FVector pos, int depth, bool gravityPass);
	FVector HomeMadeCollideAndSlide(FVector vel, FVector initialVel, FVector pos, int depth, bool gravityPass);
	
	FVector ProjectAndScale(FVector vec, FVector normal);
};
