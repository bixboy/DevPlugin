#include "Vehicles/Ship/ShipMaster.h"
#include "EnhancedInputComponent.h"
#include "Blueprint/UserWidget.h"
#include "Net/UnrealNetwork.h"
#include "Widgets/MouseSteeringShip.h"


#pragma region Setup

AShipMaster::AShipMaster()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicatingMovement(true);
}

void AShipMaster::BeginPlay()
{
	Super::BeginPlay();

	if (BaseVehicle)
	{
		BaseVehicle->SetSimulatePhysics(true);
		BaseVehicle->SetAngularDamping(1.f);
		BaseVehicle->SetEnableGravity(true);
	}

	OnEngineChangeDelegate.AddDynamic(this, &AShipMaster::EngineChange);

	if (HasAuthority())
	{
		LandingGears.Empty();
		TArray<USceneComponent*> AllComps;
		GetComponents<USceneComponent>(AllComps);

		for (USceneComponent* Comp : AllComps)
		{
			if (Comp->ComponentHasTag(TEXT("LandingGear")))
			{
				LandingGears.Add(Comp);
			}
		}
	}
}

void AShipMaster::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (auto* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInput->BindAction(BoostAction, ETriggerEvent::Started, this, &AShipMaster::Input_OnBoost);

		EnhancedInput->BindAction(ThrustAction, ETriggerEvent::Triggered, this, &AShipMaster::Input_Thrust);
		EnhancedInput->BindAction(ThrustAction, ETriggerEvent::Completed, this, &AShipMaster::Input_ThrustReleased);

		EnhancedInput->BindAction(PivoAction, ETriggerEvent::Triggered, this, &AShipMaster::Input_OnShipSideMove);
		EnhancedInput->BindAction(PivoAction, ETriggerEvent::Completed, this, &AShipMaster::Input_OnShipSideMove);
		
		EnhancedInput->BindAction(LiftAction, ETriggerEvent::Triggered, this, &AShipMaster::Input_OnShipLift);

		EnhancedInput->BindAction(LandingAction, ETriggerEvent::Started, this, &AShipMaster::Server_AttemptLanding);
	}
}

void AShipMaster::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShipMaster, FlightState);
}

#pragma endregion


void AShipMaster::EngineChange(bool bEngine)
{
	if (bEngine && ShipHudClass)
	{
		Add_MouseSteering();
	}
	else if (ShipHud)
	{
		Remove_MouseSteering();
	}
}


// ======= Update =======
#pragma region Update

void AShipMaster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority() && BaseVehicle && bEngineOn && FlightState == EShipFlightState::Flying)
	{
		HandleMouseSteering(DeltaTime);
	}

	if (!HasAuthority() || !BaseVehicle || !bEngineOn || FlightState == EShipFlightState::Landed)
	return;
	
	switch (FlightState)
	{
		case EShipFlightState::Landed:
			
			// rien à faire, on attend le StartTakeoff()
			break;

		case EShipFlightState::Landing:
			
			HandleLanding(DeltaTime);
			break;

		case EShipFlightState::TakingOff:
			
			HandleTakeoff(DeltaTime);
			break;

		case EShipFlightState::Flying:
			
			BaseVehicle->SetEnableGravity(false);
			//HandleMouseSteering(DeltaTime);
			HandleThrust(DeltaTime);
		
			// ===== Handle Rolling via SideInput =====
			if (FMath::Abs(TurnInput) > KINDA_SMALL_NUMBER)
			{
				float SpeedFactor = FMath::Clamp(CurrentThrust / MaxThrust, 0.f, 1.f);
				float RollFactor = FMath::Lerp(1.f, MaxRollSpeedMultiplier, SpeedFactor);
				float TorquePower = RollRotationPower * RollFactor;

				FVector RollTorque = (-TurnInput * TorquePower) * BaseVehicle->GetForwardVector();
				BaseVehicle->AddTorqueInDegrees(RollTorque, NAME_None, true);
			}

		/*
			else
			{
				// Movement Rotation
				float TargetRoll = SideInput * MaxBankAngle;

				FRotator CurrentRot = GetActorRotation();
				FRotator DesiredRot = FRotator(CurrentRot.Pitch, CurrentRot.Yaw, TargetRoll);
				FRotator NewRot = FMath::RInterpTo(CurrentRot, DesiredRot, DeltaTime, BankInterpSpeed);
				SetActorRotation(NewRot);
			}
		*/	
		
			break;
	}

	FVector Velocity = BaseVehicle->GetPhysicsLinearVelocity();
	float SpeedCmPerSec = Velocity.Size();

	float SpeedKmH = (SpeedCmPerSec / 100.0f) * 3.6f;

	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Green, FString::Printf(TEXT("Speed: %.1f km/h"), SpeedKmH));
}

void AShipMaster::HandleThrust(float DeltaTime)
{
	// Interpolate thrust
	float Target = FMath::Clamp(CurrentThrust + ThrustInput * (ThrustInput > 0 ? ThrustRateUp : ThrustRateDown) * DeltaTime, MinThrust, MaxThrust);
	CurrentThrust = FMath::FInterpTo(CurrentThrust, Target, DeltaTime, ThrustInterpSpeed);

	FVector ForwardVel = BaseVehicle->GetForwardVector() * CurrentThrust;
	
	float PitchDot = FVector::DotProduct(BaseVehicle->GetForwardVector(), FVector::UpVector);
	float Factor = PitchDot>0 ? FMath::Lerp(1.f, 0.8f, FMath::Abs(PitchDot)) : FMath::Lerp(1.f, 1.5f, FMath::Abs(PitchDot));
	
	ForwardVel *= Factor * (bBoosting ? BoostMultiplier : 1.f);

	FVector Final = ForwardVel + BaseVehicle->GetRightVector() * SideInput * SideSpeedMove;
	BaseVehicle->SetPhysicsLinearVelocity(Final);

	// Reverse Delay
	
	const float ThresholdZero = 10.f;
	if (ThrustInput < 0.f && FMath::Abs(CurrentThrust) < ThresholdZero && !bCanReverse)
	{
		ReverseHoldTime += DeltaTime;
		if (ReverseHoldTime >= ReverseDelay)
		{
			bCanReverse = true;
		}
	}
	else if (ThrustInput >= 0.f)
	{
		ReverseHoldTime = 0.f;
		bCanReverse    = false;
	}

	if (ThrustInput < 0.f && !bCanReverse)
	{
		CurrentThrust = FMath::Max(0.f, CurrentThrust);
		
		FVector vel = BaseVehicle->GetForwardVector() * CurrentThrust + BaseVehicle->GetRightVector() * SideInput * SideSpeedMove;
		BaseVehicle->SetPhysicsLinearVelocity(vel);
	}
}

// ====== Mouse Movement ======

void AShipMaster::HandleMouseSteering(float DeltaTime)
{
	if (!BaseVehicle) return;

	// 2) Récupère le PlayerController
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	// 3) Récupère la position souris
	float MouseX, MouseY;
	if (!PC->GetMousePosition(MouseX, MouseY)) return;

	// 4) Récupère la taille du viewport
	int32 ViewportX, ViewportY;
	PC->GetViewportSize(ViewportX, ViewportY);
	if (ViewportX == 0 || ViewportY == 0) return;

	// 5) Centre et offset
	FVector2D Center(static_cast<float>(ViewportX) * 0.5f, static_cast<float>(ViewportY) * 0.5f);
	FVector2D Offset(MouseX, MouseY);
	Offset -= Center;

	float Size = Offset.Size();

	// 6) Calcul des rayons
	float ScreenMin     = FMath::Min(static_cast<float>(ViewportX), static_cast<float>(ViewportY)) * 0.5f;
	float NeutralRadius = MoveNeutralZone * ScreenMin;
	float MaxRadius     = MaxSteeringRadius * ScreenMin;

	FVector2D Norm(0.f, 0.f);

	if (Size >= NeutralRadius)
	{
		FVector2D Clamped = (Size > MaxRadius) ? Offset.GetSafeNormal() * MaxRadius : Offset;
		Norm = Clamped / MaxRadius;
	}

	Server_SetMouseSteering(Norm);

	// 7) Mise à jour du HUD
	if (ShipHud)
	{
		ShipHud->UpdateMouseVisuals();
	}
}

void AShipMaster::Server_SetMouseSteering_Implementation(const FVector2D& NormalizedSteering)
{
	if (!BaseVehicle)
		return;
	
	if (NormalizedSteering.IsNearlyZero())
	{
		FVector AV = BaseVehicle->GetPhysicsAngularVelocityInDegrees();
		BaseVehicle->SetPhysicsAngularVelocityInDegrees( AV * FMath::Pow(0.5f, GetWorld()->GetDeltaSeconds() * DampingCoeff) );
	}
	else
	{
		FVector PitchT = NormalizedSteering.Y * MouseTorque * BaseVehicle->GetRightVector();
		FVector YawT   = NormalizedSteering.X * MouseTorque * BaseVehicle->GetUpVector();
		BaseVehicle->AddTorqueInDegrees(PitchT + YawT, NAME_None, true);
	}
}

FVector2D AShipMaster::GetMouseOffset()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return FVector2D::ZeroVector;	
	}
	
	float x,y;
	PC->GetMousePosition(x,y);

	int Vx,Vy;
	PC->GetViewportSize(Vx, Vy);
	
	FVector2D Center( Vx * 0.5f, Vy * 0.5f);
	
	return FVector2D(x,y) - Center;
}


void AShipMaster::Remove_MouseSteering()
{
	if (ShipHud)
	{
		OnMouseMoveDelegate.RemoveDynamic(ShipHud, &UMouseSteeringShip::UpdateMouseVisuals);
		ShipHud->RemoveFromParent();
		ShipHud = nullptr;	
	}
}

void AShipMaster::Add_MouseSteering()
{
	if (!ShipHud && ShipHudClass)
	{
		ShipHud = CreateWidget<UMouseSteeringShip>(GetWorld(), ShipHudClass);
		ShipHud->AddToViewport();
		ShipHud->SetupWidget(MoveNeutralZone, MaxSteeringRadius);

		OnMouseMoveDelegate.AddDynamic(ShipHud, &UMouseSteeringShip::UpdateMouseVisuals);
	}
}

void AShipMaster::Input_OnExit()
{
	Super::Input_OnExit();

	Remove_MouseSteering();
}

void AShipMaster::EnterVehicle(ACustomPlayerController* PC, USceneComponent* Seat)
{
	Super::EnterVehicle(PC, Seat);

	if (Seat == SeatDriver)
	{
		Add_MouseSteering();
	}
}

#pragma endregion


// ======= Movements =======
#pragma region Movements

// ====== Thrust Input ======
void AShipMaster::Input_Thrust(const FInputActionValue& InputActionValue)
{
	float Value = InputActionValue.Get<float>();
	Server_Thrust(Value);
}

void AShipMaster::Input_ThrustReleased()
{
	Server_ThrustReleased();
}

void AShipMaster::Server_ThrustReleased_Implementation()
{
	ReverseHoldTime = 0.f;
	bCanReverse    = false;

	Server_Thrust(0.f);
}

void AShipMaster::Server_Thrust_Implementation(float Value)
{
	if (!bEngineOn) return;
	
	ThrustInput = Value;

	if (FlightState == EShipFlightState::TakingOff && ThrustInput != 0.f)
	{
		FlightState = EShipFlightState::Flying;
		BaseVehicle->SetSimulatePhysics(true);
		BaseVehicle->SetEnableGravity(false);
	}
}

// ======= Left / Right =======
void AShipMaster::Input_OnShipSideMove(const FInputActionValue& InputActionValue)
{
	float YawInput = InputActionValue.Get<float>();

	Server_OnShipSideMove(YawInput);
}

void AShipMaster::Server_OnShipSideMove_Implementation(float Value)
{
	if (!bEngineOn || FlightState != EShipFlightState::Flying || !BaseVehicle) return;

	SideInput = FMath::Clamp(Value, -1.f, 1.f);
}


// ======= Up / Down =======
void AShipMaster::Input_OnShipLift(const FInputActionValue& InputActionValue)
{
	float YawInput = InputActionValue.Get<float>();

	Server_OnShipLift(YawInput);
}

void AShipMaster::Server_OnShipLift_Implementation(float LiftInput)
{
	if (!bEngineOn || !BaseVehicle || FlightState != EShipFlightState::Flying) return;

	FVector Force = FVector::UpVector * (LiftInput * UpDownSpeedMove);
	BaseVehicle->AddForce(Force, NAME_None, true);
}

#pragma endregion


// ======= Boost =======
#pragma region Boost

void AShipMaster::Input_OnBoost()
{
	Server_OnBoost();
}

void AShipMaster::Server_OnBoost_Implementation()
{
	if (bBoosting)
	{
		GetWorldTimerManager().ClearTimer(BoostTimerHandle);
	}

	bBoosting = true;

	GetWorldTimerManager().SetTimer(BoostTimerHandle, this, &AShipMaster::EndBoost, BoostDuration, false);
}

void AShipMaster::EndBoost()
{
	bBoosting = false;
}

#pragma endregion


// ======= Landing / Takeoff=======
#pragma region Landing / Takeoff

void AShipMaster::StartTakeoff()
{
	if (!BaseVehicle)
		return;
	
	if (FlightState != EShipFlightState::Landed) return;

	FlightState = EShipFlightState::TakingOff;
	TakeoffElapsed = 0.f;
	TakeoffStartLocation = GetActorLocation();

	BaseVehicle->SetSimulatePhysics(false);
	BaseVehicle->SetEnableGravity(false);

	if (!bEngineOn)
	{
		Input_Client_SwitchEngine();
	}
}

void AShipMaster::HandleTakeoff(float DeltaTime)
{
	TakeoffElapsed += DeltaTime;
	float Alpha = FMath::Clamp(TakeoffElapsed / TakeoffDuration, 0.f, 1.f);

	FVector TargetLoc = TakeoffStartLocation + FVector(0,0,TakeoffHeight);
	FVector NewLoc = FMath::Lerp(TakeoffStartLocation, TargetLoc, Alpha);
	SetActorLocation(NewLoc);

	if (Alpha >= 1.f)
	{
		FlightState = EShipFlightState::Flying;

		BaseVehicle->SetSimulatePhysics(true);
		BaseVehicle->SetEnableGravity(false);

		BaseVehicle->SetPhysicsLinearVelocity(FVector::ZeroVector);
	}
}


void AShipMaster::Server_AttemptLanding_Implementation()
{
	if (!BaseVehicle)
		return;

	if (FlightState == EShipFlightState::Landed || FlightState == EShipFlightState::Landing)
	{
		StartTakeoff();
		return;
	}

	if (bEngineOn && CurrentThrust <= MaxThrust * LandingMaxThrustFactor)
	{
		FVector Start = BaseVehicle->GetComponentLocation();
		FVector End = Start - FVector::UpVector * LandingDistanceThreshold * 1.5f;

		FVector Forward = BaseVehicle->GetForwardVector();
		FQuat OrientationQuat = FRotationMatrix::MakeFromX(Forward).ToQuat();

		FHitResult OutHit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		bool bHit = GetWorld()->SweepSingleByChannel(
			OutHit,
			Start,
			End,
			FRotator::ZeroRotator.Quaternion(),
			ECC_GameTraceChannel5,
			FCollisionShape::MakeBox( BaseVehicle->GetCollisionShape().GetExtent()),
			Params
		);

		float Dist = (Start.Z - OutHit.Location.Z);
		if (bHit && Dist <= LandingDistanceThreshold)
		{
			FlightState = EShipFlightState::Landing;
			
			CurrentThrust = 0.f;
			ThrustInput = 0.f;

			BaseVehicle->SetSimulatePhysics(true);
			BaseVehicle->SetPhysicsLinearVelocity(FVector::ZeroVector);

			InitialRotation = GetActorRotation();
			LandingStartZ = GetActorLocation().Z;
			
			TotalLandingDistance = LandingStartZ - (OutHit.Location.Z + LegContactDistance);
		}
	}
}

void AShipMaster::HandleLanding(float DeltaTime)
{
	if (ThrustInput > KINDA_SMALL_NUMBER)
    {
        StartTakeoff();
        return;
    }

    // --- Descente verticale continue ---
    FVector Loc = GetActorLocation();
    float CurrZ = Loc.Z;

    float DescStep = LandingSpeed * DeltaTime;
    float TentativeZ = CurrZ - DescStep;
	Loc.Z = TentativeZ;
	
    SetActorLocation(Loc);

    // --- Calcul ---
    float DoneDist = LandingStartZ - Loc.Z;
    float Alpha = (TotalLandingDistance > KINDA_SMALL_NUMBER) ?
    	FMath::Clamp(DoneDist / TotalLandingDistance, 0.f, 1.f): 1.f;

    // --- Take Normal ---
    FVector Pivot = BaseVehicle->GetComponentLocation();
    FVector End = Pivot - FVector::UpVector * (LandingDistanceThreshold * 2.f);
    FVector GroundNormal = FVector::UpVector;

	FHitResult Hit;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Pivot, End, ECC_GameTraceChannel5, FCollisionQueryParams(NAME_None, false, this)))
    {
        GroundNormal = Hit.Normal.GetSafeNormal();
    }

    // --- Rotation ---
	{
		FVector Forward = BaseVehicle->GetForwardVector();
		FVector TangentF = FVector::VectorPlaneProject(Forward, GroundNormal).GetSafeNormal();
		FRotator GroundAlignRot = FRotationMatrix::MakeFromXZ(TangentF, GroundNormal).Rotator();

		FQuat Q0 = InitialRotation.Quaternion();
		FQuat Q1 = GroundAlignRot.Quaternion();
		FQuat Qn = FQuat::Slerp(Q0, Q1, Alpha);
	
		SetActorRotation(Qn);	
	}

    // --- Landing Gears ---
	{
		bool bAllDown = true;
		for (int32 i = 0; i < LandingGears.Num(); ++i)
		{
			USceneComponent* Leg = LandingGears[i];
			FVector A = Leg->GetComponentLocation();
			FVector B = A - GroundNormal * LegContactDistance;
        
			FHitResult LegHit;
			if (!GetWorld()->LineTraceSingleByChannel(LegHit, A, B, ECC_GameTraceChannel5, FCollisionQueryParams(NAME_None, false, this)))
			{
				bAllDown = false;
			}
		}
		if (bAllDown)
		{
			FlightState = EShipFlightState::Landed;
			
			BaseVehicle->SetSimulatePhysics(false);
		}	
	}
}

#pragma endregion
