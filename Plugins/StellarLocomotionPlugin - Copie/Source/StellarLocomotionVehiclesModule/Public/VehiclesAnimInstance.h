#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "VehiclesAnimInstance.generated.h"

class UCameraComponent;

UCLASS()
class STELLARLOCOMOTIONVEHICLESMODULE_API UVehiclesAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void UpdateTurretRotation(FRotator NewAngle, FName TurretName);

	UFUNCTION()
	void UpdateTurretAimLocation(FVector LookAtLocation, FName TurretName);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, meta=(BlueprintThreadSafe))
	FRotator GetTurretRotation(FName TurretName) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, meta=(BlueprintThreadSafe))
	FVector GetTurretAimLocation(FName TurretName) const;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FName, FRotator> TurretAngle;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FName, FVector> TurretAimLocation;
};
