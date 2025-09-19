// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "StellarCharacterAnimInstance.generated.h"

class AStellarCharacterClass;
/**
 * 
 */
UCLASS(Blueprintable)
class STELLARLOCOMOTIONCHARACTERSMODULE_API UStellarCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Owner", Transient)
	TObjectPtr<AStellarCharacterClass> Character;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TEnumAsByte<EMovementMode> CurrentMovementMode = EMovementMode::MOVE_None;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FRotator PitchPerBone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	float Direction;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	float Speed;




	virtual void NativeInitializeAnimation() override;

	virtual void NativeBeginPlay() override;

	virtual void NativeUpdateAnimation(float DeltaTime) override;

protected:
	void GetSpeed();
	void GetDirection();


	UFUNCTION(BlueprintCallable)
	void SetSpineRotation(const float& SpineRotation);
};
