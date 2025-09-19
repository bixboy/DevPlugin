// Fill out your copyright notice in the Description page of Project Settings.


#include "Animations/StellarCharacterAnimInstance.h"

#include "KismetAnimationLibrary.h"
#include "StellarCharacterClass.h"


void UStellarCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UStellarCharacterAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	AActor* Owner = GetOwningActor();
	if(Owner)
	{
		Character = Cast<AStellarCharacterClass>(Owner);
	}

	// USkeletalMeshComponent* SKM = GetOwningComponent();
	// if(SKM)
	// {
	// 	FTransform SocketTransform = SKM->GetSocketTransform("Camera", RTS_ParentBoneSpace);
	// 	CamSocketLocation = SocketTransform.GetLocation();
	// 	CamSocketRotation = SocketTransform.GetRotation().Rotator();
	//
	// 	// GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, CamSocketLocation.ToString());
	// 	// GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, CamSocketRotation.ToString());
	//
	// }
}

void UStellarCharacterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if(!Character) return;
	
	GetSpeed();
	GetDirection();
	CurrentMovementMode = Character->GetMovementMode();
}



void UStellarCharacterAnimInstance::GetSpeed()
{
	Speed = Character->GetVelocity().Length();
}

void UStellarCharacterAnimInstance::GetDirection()
{
	Speed = Character->GetVelocity().Length();

	Direction = UKismetAnimationLibrary::CalculateDirection(Character->GetVelocity(), Character->GetActorRotation());
}

void UStellarCharacterAnimInstance::SetSpineRotation(const float& SpineRotation)
{
	PitchPerBone.Roll = SpineRotation;
}