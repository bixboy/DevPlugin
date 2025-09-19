#include "Component/TurretCamera.h"
#include "CameraVehicle.h"


ACameraVehicle* UTurretCamera::SpawnCamera(TSubclassOf<ACameraVehicle> CameraClass)
{
	return  GetWorld()->SpawnActor<ACameraVehicle>(GetComponentLocation(), GetComponentRotation());
}
