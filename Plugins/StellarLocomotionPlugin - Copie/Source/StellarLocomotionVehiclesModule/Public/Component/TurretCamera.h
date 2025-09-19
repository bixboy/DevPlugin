#pragma once
#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "TurretCamera.generated.h"

class ACameraVehicle;


UCLASS( ClassGroup = (Custom), meta = ( BlueprintSpawnableComponent ) )
class STELLARLOCOMOTIONVEHICLESMODULE_API UTurretCamera : public UCameraComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Turret")
	int32 AssociateSeatIndex = 0;

	UPROPERTY(EditAnywhere, Category="Turret")
	FName SkeletalSocketName = "Turret";

	UFUNCTION()
	ACameraVehicle* SpawnCamera(TSubclassOf<ACameraVehicle> CameraClass);
};
