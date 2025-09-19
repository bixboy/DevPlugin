#include "Vehicles/Walker/WalkerVehicles.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"


AWalkerVehicles::AWalkerVehicles()
{
	PrimaryActorTick.bCanEverTick = true;

	SkeletalBaseVehicle = CreateDefaultSubobject<USkeletalMeshComponent>("SkeletalBaseVehicle");
	SkeletalBaseVehicle->SetupAttachment(RootComponent);

	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>("MovementComponent");
}

void AWalkerVehicles::BeginPlay()
{
	Super::BeginPlay();

	OnVehicleMove.AddDynamic(this, &AWalkerVehicles::WalkerMoving);

	if (bUseProceduralWalk && !Legs.IsEmpty())
	{
		for (FAnimationLeg& Leg : Legs)
		{
			FHitResult Hit;
			FTransform BoneTransform = SkeletalBaseVehicle->GetBoneTransform(Leg.ToeName, ERelativeTransformSpace::RTS_World);

			
			FVector Location = GetCenterOfWalker(BoneTransform.GetLocation(), 90.f, -90.f, Hit, 90.f);
			if (Hit.bBlockingHit)
			{
				Leg.Location = Location;
				Leg.InitialOffset = Location - GetActorLocation();
			}
		}
		bLegsIsReady = true;
		LastLocation = GetActorLocation();
	}
}

void AWalkerVehicles::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bUseProceduralWalk)
	{
		FHitResult Hit;
		FVector Location = GetCenterOfWalker(GetActorLocation(), 300.f, -100.f, Hit, FootHeight);
		if (Hit.bBlockingHit)
		{
			WalkerCenter = 	Location;
			WalkerCenter = FVector(WalkerCenter.X, WalkerCenter.Y, WalkerCenter.Z -200.f);
		}
		else
		{
			FVector ActorLocation = GetActorLocation();
			WalkerCenter = FVector(ActorLocation.X, ActorLocation.Y, ActorLocation.Z + SkeletalBaseVehicle->GetRelativeLocation().Z);
		}

		if (GetForwardInput() != 0 || GetTurnInput() != 0)
		{
			UpdateLegsPosition();	
		}	
	}
}

void AWalkerVehicles::WalkerMoving(float FowardInput, float RightInput)
{
	float DeltaTime = GetWorld()->GetDeltaSeconds();

	// 1. Gérer la rotation en Yaw
	if (FMath::Abs(RightInput) > KINDA_SMALL_NUMBER)
	{
		float RotationSpeed = 20.f;
		FRotator CurrentRotation = GetActorRotation();
		CurrentRotation.Yaw += RightInput * RotationSpeed * DeltaTime;
		SetActorRotation(CurrentRotation);
	}

	// 2. Gérer le déplacement avant/arrière
	if (FMath::Abs(ForwardInput) > KINDA_SMALL_NUMBER)
	{
		FVector MovementDirection = UKismetMathLibrary::GetForwardVector(GetActorRotation());
		MovementDirection.Normalize();

		if (MovementComponent)
		{
			MovementComponent->AddInputVector(MovementDirection * ForwardInput * MaxSpeed);
		}
	}
}

void AWalkerVehicles::UpdateLegsPosition()
{
	if (bUseProceduralWalk && !Legs.IsEmpty())
	{
		for (FAnimationLeg& Leg : Legs)	
		{
			if (!Leg.bPlanted)
			{
				FHitResult Hit;
				
				FRotator ActorRot = FRotator(0.f, GetActorRotation().Yaw, 0.f);
				FVector Offset = -ActorRot.RotateVector(Leg.InitialOffset);
				
				FVector StartLocation = Offset + GetActorLocation();
				FVector Location = GetCenterOfWalker(StartLocation, 500, -90, Hit, FootHeight);

				if (Hit.bBlockingHit)
				{
					// Location
					float Distance = FVector::Dist(LastLocation, GetActorLocation());
					Leg.Location = FVector(Location.X, Location.Y, Location.Z + GetLegHeight(Distance));
				}
				else
				{
					Location = GetCenterOfWalker(Hit.TraceEnd, WalkerCenter, Hit);
					Leg.Location = Location;
				}
			}
		}
		
		if (FVector::Dist(LastLocation, GetActorLocation()) > MaxStep)
		{
			for (FAnimationLeg& Leg : Legs)
			{
				FHitResult Hit;
				
				FRotator ActorRot = FRotator(0.f, GetActorRotation().Yaw, 0.f);
				FVector Offset = -ActorRot.RotateVector(Leg.InitialOffset);
				
				FVector StartLocation = Offset + GetActorLocation();
				GetCenterOfWalker(StartLocation, 500, -90, Hit, FootHeight);

				if (Hit.bBlockingHit)
				{
					Leg.bPlanted = !Leg.bPlanted;

					// Location
					float Distance = FVector::Dist(LastLocation, GetActorLocation());
					Leg.Location =  Hit.Location + FVector(0.f, 0.f, GetLegHeight(Distance));
				}
				else
				{
					Leg.bPlanted = !Leg.bPlanted;
				}
			}

			LastLocation = GetActorLocation();
		}
	}
}

float AWalkerVehicles::GetLegHeight(float Distance)
{
	float Value = UKismetMathLibrary::FClamp(Distance / MaxStep, 0.0f, 1.0f);
	return (1.f - FMath::Pow(Value, 3.f)) * StepHeight;
}

#pragma region SphereTrace

FVector AWalkerVehicles::GetCenterOfWalker(FVector Location, float Upper, float Lower, FHitResult& OutHit, float Radius)
{
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	QueryParams.AddIgnoredActor(this);

	FVector Start = FVector(Location.X, Location.Y, Location.Z + Upper);
	FVector End   = FVector(Location.X, Location.Y, Location.Z + Lower);

	bool bHit = GetWorld()->SweepSingleByChannel(
		OutHit,
		Start,
		End,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(Radius),
		QueryParams
	);

	if (bHit)
	{
		return OutHit.Location;
	}

	DrawDebugLine(GetWorld(), Start, End, FColor::Orange, true, 10.f);

	return Location;
}

FVector AWalkerVehicles::GetCenterOfWalker(FVector Start, FVector End, FHitResult& OutHit)
{
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	QueryParams.AddIgnoredActor(this);
	
	float Radius = 60.f;

	bool bHit = GetWorld()->SweepSingleByChannel(
		OutHit,
		Start,
		End,
		FQuat::Identity,
		ECollisionChannel::ECC_Visibility,
		FCollisionShape::MakeSphere(Radius),
		QueryParams
	);

	if (bHit)
	{
		return OutHit.Location;
	}

	FVector ActorLocation = GetActorLocation();
	return ActorLocation;
}

#pragma endregion

