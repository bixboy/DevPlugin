// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StellarCharacterClass.h"
#include "Components/TimelineComponent.h"
#include "StellarPlayerCharacterClass.generated.h"

class USkeletalMeshComponentBudgeted;
struct FInputActionValue;
class UInputAction;

UCLASS()
class STELLARLOCOMOTIONCHARACTERSMODULE_API AStellarPlayerCharacterClass : public AStellarCharacterClass
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "_Components")
	TObjectPtr<UCameraComponent> PlayerCamera;


#pragma region Movement Fields

	UPROPERTY(BlueprintReadOnly)
	bool IsCrouched;
	
#pragma endregion
	
#pragma region Inputs Fields
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inputs", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> LookMouseAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inputs", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inputs", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inputs", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inputs", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> WalkAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inputs", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inputs", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> JumpAction;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", Meta = (ClampMin = 0, ForceUnits = "x"))
	float LookUpMouseSensitivity{ 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", Meta = (ClampMin = 0, ForceUnits = "x"))
	float LookRightMouseSensitivity{ 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", Meta = (ClampMin = 0, ForceUnits = "deg/s"))
	float LookUpRate{ 90.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs", Meta = (ClampMin = 0, ForceUnits = "deg/s"))
	float LookRightRate{ 240.0f };

#pragma endregion


#pragma region FOV Fields

	UPROPERTY(BlueprintReadOnly, Category = "RPG Character Fields|FOV")
	UCurveFloat* FOV_CurveFloat;
	
	FTimeline FovTimeline;
	
	UPROPERTY(BlueprintReadWrite)
	FTimerHandle FOVTimerHandle;

	UPROPERTY(BlueprintReadWrite)
	float TargetFOV{0.0f};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG Character Fields|FOV", Meta = (ClampMin = 0, ForceUnits = "°"))
	float BaseFOV{110.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG Character Fields|FOV", Meta = (ClampMin = 0, ForceUnits = "°"))
	float JogFOV{111.5f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG Character Fields|FOV", Meta = (ClampMin = 0, ForceUnits = "°"))
	float SprintFOV{113.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG Character Fields|FOV", Meta = (ClampMin = 0, ForceUnits = "°"))
	float CrouchFOV{108.5f};
	
#pragma endregion

	
public:
	// Sets default values for this actor's properties
	AStellarPlayerCharacterClass();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void OnReplicated_PlayerController() override;

#pragma region Inputs Functions
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void Input_OnLookMouse(const FInputActionValue& ActionValue);

	virtual void Input_OnLook(const FInputActionValue& ActionValue);

	virtual void Input_OnMove(const FInputActionValue& ActionValue);

	virtual void Input_OnSprint(const FInputActionValue& ActionValue);
	
	virtual void Input_OnWalk();
	
	virtual void Input_OnCrouch();
	
	virtual void Input_OnJump(const FInputActionValue& ActionValue);

#pragma endregion


#pragma region FOV Functions

public:
	UFUNCTION()
	void SetFieldOfView(float newFov);

	UFUNCTION()
	void FOVTimelineProgress(float Value);

#pragma endregion


#pragma region IStellarCharacterController
	virtual void StellarUpdateVelocity_Implementation(FVector& gravityVelocity, FVector& movementVelocity, const float& deltaTime) override;
	virtual void StellarUpdateRotation_Implementation(FQuat& currentRotation, const float& deltaTime) override;
#pragma endregion
};
