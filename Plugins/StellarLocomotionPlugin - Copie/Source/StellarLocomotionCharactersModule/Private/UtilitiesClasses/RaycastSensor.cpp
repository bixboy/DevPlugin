// Fill out your copyright notice in the Description page of Project Settings.


#include "UtilitiesClasses/RaycastSensor.h"

URaycastSensor::URaycastSensor(const FObjectInitializer& ObjectInitializer)
{
	
}

UWorld* URaycastSensor::GetWorld()
{
	return World;
}

void URaycastSensor::Cast()
{
	FVector worldOrigin = Tr.TransformPosition(Origin);
	FVector worldEnd = worldOrigin + GetCastDirection() * CastLength;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner); // Ignore the owner

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitInfo,             // Result
		worldOrigin,              // Start location
		worldEnd,                // End location
		ECC_Visibility,     // Collision channel
		Params              // Query params
	);
}

FVector URaycastSensor::GetCastDirection()
{
	switch (CastDirection)
	{
		case ECastDirection::Forward: return Tr.GetUnitAxis(EAxis::X);
		case ECastDirection::Right:   return Tr.GetUnitAxis(EAxis::Y);
		case ECastDirection::Up:      return Tr.GetUnitAxis(EAxis::Z);
		case ECastDirection::Backward: return -Tr.GetUnitAxis(EAxis::X);
		case ECastDirection::Left:   return -Tr.GetUnitAxis(EAxis::Y);
		case ECastDirection::Down:      return -Tr.GetUnitAxis(EAxis::Z);
		default: return FVector::ZeroVector;
	}
}
