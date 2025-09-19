// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StellarCharacterMotor.generated.h"

class UCapsuleComponent;

/**
 * 
 */
UCLASS()
class STELLARLOCOMOTIONCHARACTERSMODULE_API UStellarCharacterMotor : public UObject
{
	GENERATED_BODY()

protected:
	UPROPERTY()
	TObjectPtr<AActor> Environement;

	UPROPERTY()
	AActor* _owner;
	
	UPROPERTY()
	UCapsuleComponent* _bounds;
	
	UPROPERTY()
	TEnumAsByte<EMovementMode> _movementMode = EMovementMode::MOVE_None;


public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxSpeed = 3000.0f;
	
protected:
	UPROPERTY()
	bool _isGrounded;
	
	UPROPERTY()
	FVector _movementVelocity;
	
	UPROPERTY()
	FQuat _rotation;


	UPROPERTY()
	float _maxSlopeAngle = 45.0f;
public:
	UFUNCTION()
	virtual void Tick(const float& DeltaTime);


	virtual void InitCharacterMotor(AActor* owner, UCapsuleComponent* bounds);


	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsGrounded() const { return _isGrounded; }

	UFUNCTION()
	EMovementMode GetMovementMode() const { return _movementMode; }

	virtual class UWorld* GetWorld() const override;
	
	UFUNCTION()
	virtual void SetCharacterEnvironement(AActor* env);

	virtual bool IsCharacterGrounded();


	UFUNCTION()
	virtual FVector GetGlobalForwardVector();
	UFUNCTION()
	virtual FVector GetGlobalRightVector();
	UFUNCTION()
	virtual FVector GetGlobalUpVector();

	UFUNCTION()
	virtual FVector GetCharacterForwardVector();
	UFUNCTION()
	virtual FVector GetCharacterRightVector();
	UFUNCTION()
	virtual FVector GetCharacterUpVector();

protected:
	FHitResult CheckCapsuleCollision(FVector Start, FVector End, float CapsuleRadius, float CapsuleHalfHeight, FQuat CapsuleRotation, bool drawDebug = false);

};
