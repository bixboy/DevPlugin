// Fill out your copyright notice in the Description page of Project Settings.


#include "ExempleCharacter/StellarPlayerCharacterClass.h"

#include "EnhancedInputComponent.h"
#include "SkeletalMeshComponentBudgeted.h"
#include "Camera/CameraComponent.h"


// Sets default values
AStellarPlayerCharacterClass::AStellarPlayerCharacterClass()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(FName{ TEXTVIEW("PlayerCamera") });
	PlayerCamera->SetupAttachment(SkeletalMeshComponent, "Camera");
}

// Called when the game starts or when spawned
void AStellarPlayerCharacterClass::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AStellarPlayerCharacterClass::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AStellarPlayerCharacterClass::OnReplicated_PlayerController()
{
	Super::OnReplicated_PlayerController();
}
#pragma region Inputs

// Called to bind functionality to input
void AStellarPlayerCharacterClass::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	if(GetController())
	{
		//GetWorldTimerManager().SetTimer(ReplicateTransformTimer, this, &AEntityCharacterClass::ReplicateCharacterTransform, 0.04f, true);
	}
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("SET PLAYER INPUT COMPONENT"));
	}
	
	auto* EnhancedInput{ Cast<UEnhancedInputComponent>(PlayerInputComponent) };
	if (IsValid(EnhancedInput))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("VALID ENHANCED INPUT COMPONENT"));

		StoredInputHandles.Add(EnhancedInput->BindAction(LookMouseAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnLookMouse).GetHandle());
		StoredInputHandles.Add(EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnLook).GetHandle());
		StoredInputHandles.Add(EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnMove).GetHandle());
		StoredInputHandles.Add(EnhancedInput->BindAction(SprintAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnSprint).GetHandle());
		StoredInputHandles.Add(EnhancedInput->BindAction(WalkAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnWalk).GetHandle());
		StoredInputHandles.Add(EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnCrouch).GetHandle());
		StoredInputHandles.Add(EnhancedInput->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnJump).GetHandle());
		
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::FromInt(EnhancedInput->GetActionEventBindings().Num()));
	}
}

void AStellarPlayerCharacterClass::Input_OnLookMouse(const FInputActionValue& ActionValue)
{
	const FVector2f Value{ ActionValue.Get<FVector2D>() };
	
	
	// if (GEngine)
	// {
	// 	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("LOOK MOUSE"));
	// }
	
	AddControllerPitchInput(Value.Y * LookUpMouseSensitivity);
	AddControllerYawInput(Value.X * LookRightMouseSensitivity);
}

void AStellarPlayerCharacterClass::Input_OnLook(const FInputActionValue& ActionValue)
{
	const FVector2f Value{ ActionValue.Get<FVector2D>() };
	
	AddControllerPitchInput(Value.Y * LookUpRate);
	AddControllerYawInput(Value.X * LookRightRate);
}

void AStellarPlayerCharacterClass::Input_OnMove(const FInputActionValue& ActionValue)
{
	// if (GEngine)
	// {
	// 	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("MOVE"));
	// }
	 const auto Value{ ActionValue.Get<FVector2D>() };

	 MovementInput.Y = Value.Y;
	 MovementInput.X = Value.X;
	
}

void AStellarPlayerCharacterClass::Input_OnSprint(const FInputActionValue& ActionValue)
{
	
	// if(IsValid(AC_Stats))
	// {
	//
	// 	if (ActionValue.Get<bool>() && AC_Stats->GetStatCurrentValue(EStatsTypes::STAMINA) > 15)
	// 	{
	// 		SetMaxSpeed(SprintSpeed, true);
	// 		AC_Stats->AddValueToStatByTimer(EStatsTypes::STAMINA, -SprintStaminaConsumption, 1.0f);
	// 		SetFieldOfView(SprintFOV);
	// 	}
	//
	// 	if (!ActionValue.Get<bool>())
	// 	{
	// 		SetMaxSpeed(WalkForwardSpeed, true);
	// 		AC_Stats->AddValueToStatByTimer(EStatsTypes::STAMINA, SprintStaminaConsumption, 2.0f);
	// 		SetFieldOfView(BaseFOV);
	// 	}
	// }


	if (ActionValue.Get<bool>())
	{
		SetMaxSpeed(SprintSpeed, true);
		SetFieldOfView(SprintFOV);
	}else
	{
		SetMaxSpeed(WalkForwardSpeed, true);
		SetFieldOfView(BaseFOV);
	}
	
}


void AStellarPlayerCharacterClass::Input_OnWalk()
{
	if(CurrentMaxSpeed == WalkForwardSpeed)
	{
		SetMaxSpeed(JogForwardSpeed, true);
		SetFieldOfView(JogFOV);

	}else
	{
		SetMaxSpeed(WalkForwardSpeed, true);
		SetFieldOfView(BaseFOV);
	}
}


void AStellarPlayerCharacterClass::Input_OnCrouch()
{
	//SetCrouch(!GetCharacterMovement()->IsCrouching(), true);
	
	
	// if (GetDesiredStance() == AlsStanceTags::Standing)
	// {
	// 	SetDesiredStance(AlsStanceTags::Crouching);
	// }
	// else if (GetDesiredStance() == AlsStanceTags::Crouching)
	// {
	// 	SetDesiredStance(AlsStanceTags::Standing);
	// }
}

void AStellarPlayerCharacterClass::Input_OnJump(const FInputActionValue& ActionValue)
{
	if (ActionValue.Get<bool>())
	{
		// if (StopRagdolling())
		// {
		// 	return;
		// }
		//
		// if (StartMantlingGrounded())
		// {
		// 	return;
		// }
		//
		// if (GetStance() == AlsStanceTags::Crouching)
		// {
		// 	SetDesiredStance(AlsStanceTags::Standing);
		// 	return;
		// }

		//Jump();
	}
	else
	{
		//StopJumping();
	}
}
#pragma endregion





#pragma region FOV Functions

void AStellarPlayerCharacterClass::SetFieldOfView(float newFov)
{
	if(!GetWorld() || GetLocalRole() != ROLE_AutonomousProxy) return;
	
	if(!FOV_CurveFloat)
	{
		FOV_CurveFloat = NewObject<UCurveFloat>(this, UCurveFloat::StaticClass());
	
		FOV_CurveFloat->FloatCurve.SetKeyInterpMode(FOV_CurveFloat->FloatCurve.AddKey(0.f, 0.f), ERichCurveInterpMode::RCIM_Cubic);
		FOV_CurveFloat->FloatCurve.SetKeyInterpMode(FOV_CurveFloat->FloatCurve.AddKey(1.4f, 1.f), ERichCurveInterpMode::RCIM_Cubic);
	}
	
	if (FOV_CurveFloat && !FovTimeline.IsPlaying())
	{
		if (FovTimeline.GetTimelineLength() == 0.0f)
		{
			FOnTimelineFloat ProgressFunction;
			ProgressFunction.BindUFunction(this, FName("FOVTimelineProgress"));
			FovTimeline.AddInterpFloat(FOV_CurveFloat, ProgressFunction);
			FovTimeline.SetLooping(false);

			// Set the timeline length mode to use the last keyframe
			FovTimeline.SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);
		}
	}

	TargetFOV = newFov;
	FovTimeline.PlayFromStart();
}


void AStellarPlayerCharacterClass::FOVTimelineProgress(float Value)
{
	PlayerCamera->SetFieldOfView(FMath::Lerp(PlayerCamera->FieldOfView, TargetFOV, Value));
}

#pragma endregion


#pragma region IStellarCharacterController

void AStellarPlayerCharacterClass::StellarUpdateVelocity_Implementation(FVector& gravityVelocity, FVector& movementVelocity,
	const float& deltaTime)
{
	Super::StellarUpdateVelocity_Implementation(gravityVelocity, movementVelocity, deltaTime);

	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, "UPDATE VELOCITY");
}

void AStellarPlayerCharacterClass::StellarUpdateRotation_Implementation(FQuat& currentRotation, const float& deltaTime)
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, "UPDATE Rotate");

	FRotator NewRotation = RotationInput * 45.0f * deltaTime;
	currentRotation = NewRotation.Quaternion();

	FRotator DeltaRot(NewRotation);
	FRotator ViewRotation = PlayerCamera->GetRelativeRotation();
	ViewRotation.Pitch = FMath::Clamp(ViewRotation.Pitch + DeltaRot.Pitch, -85.f, 85.f); // Clamp pitch
	PlayerCamera->SetRelativeRotation(ViewRotation);

	RotationInput = FRotator::ZeroRotator;
}

#pragma endregion