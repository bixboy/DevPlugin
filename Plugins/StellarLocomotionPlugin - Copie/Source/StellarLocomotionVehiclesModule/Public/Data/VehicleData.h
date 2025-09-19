#pragma once
#include "CoreMinimal.h"
#include "CameraVehicle.h"
#include "VehicleData.generated.h"


UENUM(BlueprintType)
enum class EVehiclePlaceType : uint8
{
	Driver,
	Gunner,
	Passenger,
	None
};

USTRUCT(BlueprintType)
struct FVehicleRole
{
	GENERATED_BODY()

	UPROPERTY()
	APawn* Player = nullptr;

	UPROPERTY()
	EVehiclePlaceType RoleName;

	bool operator==(const FVehicleRole& Other) const
	{
		return	Player == Other.Player &&
				RoleName == Other.RoleName;
	}
};

USTRUCT(BlueprintType)
struct FTurrets
{
	GENERATED_BODY()

	UPROPERTY()
	ACameraVehicle* Camera = nullptr;

	UPROPERTY()
	UStaticMeshComponent* TurretMesh = nullptr;

	UPROPERTY(BlueprintReadWrite)
	USceneComponent* SeatOwner = nullptr;

	UPROPERTY()
	APlayerController* Occupant = nullptr;

	UPROPERTY(BlueprintReadWrite)
	float AccumulatedYaw = 0.f;
	
	UPROPERTY(BlueprintReadWrite)
	float AccumulatedPitch = 0.f;

	UPROPERTY(BlueprintReadWrite)
	FVector CurrentLookAtLocation = FVector::ZeroVector;

	FRotator LastTurretBaseRot = FRotator::ZeroRotator;

	float CurrentLookAtDistance = 0.f;

	UPROPERTY()
	float BaseYaw = 0.f;

	UPROPERTY()
	float BasePitch = 0.f;
	
	bool operator == (const FTurrets& Other) const
	{
		return Camera           == Other.Camera &&
			   TurretMesh       == Other.TurretMesh &&
			   AccumulatedYaw   == Other.AccumulatedYaw && 
			   AccumulatedPitch == Other.AccumulatedPitch &&
			   SeatOwner        == Other.SeatOwner;
	}

	bool operator != (const FTurrets& Other) const
	{
		return Camera           != Other.Camera &&
			   TurretMesh       != Other.TurretMesh &&
			   AccumulatedYaw   != Other.AccumulatedYaw && 
			   AccumulatedPitch != Other.AccumulatedPitch &&
			   SeatOwner        != Other.SeatOwner;
	}
};

USTRUCT(BlueprintType)
struct FAnimationLeg
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leg")
	FName ControlName = "None";

	UPROPERTY(editAnywhere, BlueprintReadWrite, Category = "Leg")
	FVector Location;

	UPROPERTY(BlueprintReadWrite, Category = "Leg")
	FVector InitialOffset;

	UPROPERTY(editAnywhere, BlueprintReadWrite, Category = "Leg")
	bool bPlanted = false;

	UPROPERTY(editAnywhere, BlueprintReadWrite, Category = "Leg")
	FName ToeName;

	bool operator==(const FAnimationLeg& Other) const
	{
		return	ControlName == Other.ControlName &&
				Location == Other.Location;
	}
};
