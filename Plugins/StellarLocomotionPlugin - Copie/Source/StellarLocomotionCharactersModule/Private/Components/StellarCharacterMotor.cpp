// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/StellarCharacterMotor.h"

#include "Components/CapsuleComponent.h"
#include "Interfaces/CharacterInterface.h"


void UStellarCharacterMotor::Tick(const float& DeltaTime)
{
	_isGrounded = IsCharacterGrounded();

	// if (_owner->Implements<UCharacterInterface>())
	// {
	// 	//GEngine->AddOnScreenDebugMessage(-1,1.f, FColor::Green,FString::SanitizeFloat(DeltaTime));
	// }
}

void UStellarCharacterMotor::InitCharacterMotor(AActor* owner, UCapsuleComponent* bounds)
{
	_owner = owner;
	_bounds = bounds;
}

class UWorld* UStellarCharacterMotor::GetWorld() const
{
	if(_owner) return _owner->GetWorld();
	else
	{
		return nullptr;
	}
}



void UStellarCharacterMotor::SetCharacterEnvironement(AActor* env)
{
	Environement = env;

	FString Msg = Environement ? "Environement is Valid" : "Environement is Invalid"; 
	UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
}


bool UStellarCharacterMotor::IsCharacterGrounded()
{
	const FVector Start = _owner->GetActorLocation();
	const FVector DownVector = -_owner->GetActorUpVector();
	const FVector End = Start + DownVector * 3.0f; // Use a configurable value instead of _skinWidth * 3

	const float CapsuleRadius = _bounds->GetUnscaledCapsuleRadius();
	const float CapsuleHalfHeight = _bounds->GetUnscaledCapsuleHalfHeight();
	const FQuat CapsuleRotation = _owner->GetActorQuat();

	FHitResult HitResult = CheckCapsuleCollision(Start, End, CapsuleRadius, CapsuleHalfHeight, CapsuleRotation, true);

	if (HitResult.bBlockingHit)
	{
		// Optionally add slope check:
		const float SlopeDot = FVector::DotProduct(HitResult.Normal, _owner->GetActorUpVector());
		const float SlopeAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(SlopeDot, -1.f, 1.f)));

		UE_LOG(LogTemp, Log, TEXT("Ground Hit Angle: %.2f degrees"), SlopeAngle);

		return SlopeAngle <= _maxSlopeAngle; // e.g., 45.f
	}

	return false;
}

FVector UStellarCharacterMotor::GetGlobalForwardVector()
{
	return Environement ? Environement->GetActorForwardVector() : FVector();
}

FVector UStellarCharacterMotor::GetGlobalRightVector()
{
	return Environement ? Environement->GetActorRightVector() : FVector();
}

FVector UStellarCharacterMotor::GetGlobalUpVector()
{
	return Environement ? Environement->GetActorUpVector() : FVector();
}

FVector UStellarCharacterMotor::GetCharacterForwardVector()
{
	return _owner ? _owner->GetActorForwardVector() : FVector();
}

FVector UStellarCharacterMotor::GetCharacterRightVector()
{
	return _owner ?  _owner->GetActorRightVector() : FVector();
}

FVector UStellarCharacterMotor::GetCharacterUpVector()
{
	return _owner ? _owner->GetActorUpVector() : FVector();
}



FHitResult UStellarCharacterMotor::CheckCapsuleCollision(FVector Start, FVector End, float CapsuleRadius,
	float CapsuleHalfHeight, FQuat CapsuleRotation, bool drawDebug)
{
	FCollisionShape CollisionShape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);
	
	// Set up collision query parameters
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(_owner); // Ignore self

	// Perform the sweep
	FHitResult HitResult;
	bool bHit = GetWorld()->SweepSingleByChannel(
		HitResult,
		Start,
		End,
		CapsuleRotation,
		ECC_Visibility, 
		CollisionShape,
		QueryParams
	);

	if(drawDebug)
	{
		// Visualize the sweep path
		FColor TraceColor = bHit ? FColor::Red : FColor::Green;
		float LifeTime = 0.5f; // Seconds

		// Draw capsule at start
		DrawDebugCapsule(GetWorld(), Start, CollisionShape.GetCapsuleHalfHeight(), CollisionShape.GetCapsuleRadius(),
						 CapsuleRotation, TraceColor, false, LifeTime);

		// Draw capsule at end
		DrawDebugCapsule(GetWorld(), End, CollisionShape.GetCapsuleHalfHeight(), CollisionShape.GetCapsuleRadius(),
						 CapsuleRotation, TraceColor, false, LifeTime);

		// Draw line between start and end
		DrawDebugLine(GetWorld(), Start, End, TraceColor, false, LifeTime, 0, 1.0f);

		// If we hit something, mark the hit location
		if (bHit)
		{
			DrawDebugPoint(GetWorld(), HitResult.ImpactPoint, 12.f, FColor::Yellow, false, LifeTime);
			DrawDebugString(GetWorld(), HitResult.ImpactPoint + FVector(0, 0, 20), TEXT("Hit"), nullptr, FColor::White, LifeTime);
		}
	}
	
	return HitResult;
}