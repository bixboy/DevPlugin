#include "VehiclesAnimInstance.h"

void UVehiclesAnimInstance::UpdateTurretRotation(FRotator NewAngle, FName TurretName)
{
	if (!TurretAngle.Contains(TurretName))
	{
		TurretAngle.Add(TurretName, NewAngle);
	}
	else
	{
		TurretAngle[TurretName] = NewAngle;
	}
}

void UVehiclesAnimInstance::UpdateTurretAimLocation(FVector LookAtLocation, FName TurretName)
{
	if (!TurretAimLocation.Contains(TurretName))
	{
		TurretAimLocation.Add(TurretName, LookAtLocation);
	}
	else
	{
		TurretAimLocation[TurretName] = LookAtLocation;
	}
}

FRotator UVehiclesAnimInstance::GetTurretRotation(FName TurretName) const
{
	if (TurretAngle.IsEmpty() || !TurretAngle.Contains(TurretName)) return FRotator::ZeroRotator;
	
	return TurretAngle[TurretName];
}

FVector UVehiclesAnimInstance::GetTurretAimLocation(FName TurretName) const
{
	if (TurretAimLocation.IsEmpty() || !TurretAimLocation.Contains(TurretName)) return FVector::ZeroVector;
	
	return TurretAimLocation[TurretName];
}


