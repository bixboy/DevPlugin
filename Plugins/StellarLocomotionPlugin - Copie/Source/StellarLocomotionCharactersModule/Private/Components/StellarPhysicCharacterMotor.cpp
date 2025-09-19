// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/StellarPhysicCharacterMotor.h"

#include "Components/CapsuleComponent.h"
#include "Interfaces/CharacterInterface.h"

void UStellarPhysicCharacterMotor::Tick(const float& DeltaTime)
{
	Super::Tick(DeltaTime);

	// Your tick logic here
	if (_owner->Implements<UCharacterInterface>())
	{
		//GEngine->AddOnScreenDebugMessage(-1,1.f, FColor::Green,FString::SanitizeFloat(DeltaTime));

		ICharacterInterface::Execute_StellarUpdateRotation(_owner, _rotation, DeltaTime);
		ICharacterInterface::Execute_StellarUpdateUnrealPhysicVelocity(_owner, _movementVelocity, DeltaTime);
	}
	
	UpdateRotation(DeltaTime);
	UpdateMovements(DeltaTime);
	ApplyTransform(DeltaTime);

}

void UStellarPhysicCharacterMotor::UpdateRotation(float DeltaTime)
{
	FRotator DeltaRot(_rotation);
	FRotator ActorRotation = _owner->GetActorRotation();
	
	ActorRotation.Yaw += DeltaRot.Yaw;
	_owner->SetActorRotation(ActorRotation);
	
	_rotation = FQuat::Identity;
}

void UStellarPhysicCharacterMotor::UpdateMovements(float DeltaTime)
{
	FVector Force = _movementVelocity * MaxSpeed; // Adjust strength
	_bounds->AddForce(Force);
}

void UStellarPhysicCharacterMotor::ApplyTransform(float DeltaTime)
{
}
