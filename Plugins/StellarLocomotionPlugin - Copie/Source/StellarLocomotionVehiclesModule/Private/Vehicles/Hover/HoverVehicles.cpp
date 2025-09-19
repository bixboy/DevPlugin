#include "Vehicles/Hover/HoverVehicles.h"
#include "NiagaraComponent.h"
#include "EnhancedInputComponent.h"
#include "Component/HoverPointComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"


//--------------------------- Setup Functions ---------------------------
#pragma region Setup Functions

AHoverVehicles::AHoverVehicles()
{
	PrimaryActorTick.bCanEverTick = true;
	
	bReplicates = true;

	BaseVehicle->SetLinearDamping(0.5f);
	BaseVehicle->SetAngularDamping(0.5f);

	DustComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComp"));
	DustComponent->SetupAttachment(BaseVehicle);
}

void AHoverVehicles::BeginPlay()
{
	Super::BeginPlay();

	SetReplicateMovement(true);
	GetComponents(UHoverPointComponent::StaticClass(), HoverPoints, true);
}

void AHoverVehicles::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (DustComponent)
	{
		DustComponent->SetVariableFloat(FName("User.DustRadius"), DustRadius);
		DustComponent->SetVariableFloat(FName("User.DustSize"), DustSize);
	}

	if (MainCamera && SpeedLineMaterial)
	{
		SpeedLineInstance = UMaterialInstanceDynamic::Create(SpeedLineMaterial, this);
        
		MainCamera->PostProcessBlendWeight = 1.f;
		MainCamera->PostProcessSettings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, SpeedLineInstance));
		
		SpeedLineInstance->SetScalarParameterValue(TEXT("Opacity"), 0.f);
	}
}

void AHoverVehicles::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (auto* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInput->BindAction(BoostAction, ETriggerEvent::Started, this, &AHoverVehicles::Input_OnBoost);
	}
}

void AHoverVehicles::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHoverVehicles, BaseRotation);

	DOREPLIFETIME(AHoverVehicles, bSpeedLineFadingOut);
	DOREPLIFETIME(AHoverVehicles, SpeedLineFadeTime);
	DOREPLIFETIME(AHoverVehicles, bBoostActive);
}

#pragma endregion

void AHoverVehicles::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (HasAuthority() && bEngineOn)
	{
		Hovering(DeltaTime);
		HandleManualRotation(DeltaTime);
	}
}

void AHoverVehicles::Server_SwitchEngine(bool OnOff)
{
	Super::Server_SwitchEngine(OnOff);

	if (OnOff)
	{
		DustComponent->Activate();
	}
	else
	{
		DustComponent->Deactivate();
	}
}

//--------------------------- Hovering ---------------------------
void AHoverVehicles::Hovering(float DeltaTime)
{
	 if (!bEngineOn) return;

    bool bIsOnGround = false;
    FVector AverageHoverForce = FVector::ZeroVector;
    int32 HoverPointCount = 0;

    FVector SumContactLocations = FVector::ZeroVector;
    int32 ContactCount = 0;

    for (const UHoverPointComponent* HoverPointComp : HoverPoints)
    {
        FHitResult HitResult;
        float CurrentDistance = TraceGroundAtLocation(HoverPointComp, HitResult);

        if (HitResult.bBlockingHit)
        {
            bIsOnGround = true;
            SumContactLocations += HitResult.Location;
            ContactCount++;

            // --- Calcul de la force de sustentation ---
            const float DistanceError = HoverPointComp->HoverPoint.FloatingDistance - CurrentDistance;
            const float VelocityZ = BaseVehicle->GetPhysicsLinearVelocity().Z;
            const float SpringForce = DistanceError * HoverPointComp->HoverPoint.SpringStiffness;
            const float DampingForce = -VelocityZ * HoverPointComp->HoverPoint.DampingFactor;

            OscillationValue = FMath::FInterpTo(
                OscillationValue,
                FMath::Sin(GetWorld()->TimeSeconds * OscillationFrequency) * OscillationAmplitude,
                DeltaTime,
                OscillationSmoothing * 2.f
            );

            FVector SuspensionForce = FVector(0,0, SpringForce + DampingForce + OscillationValue);
            ApplyForceAtLocation(SuspensionForce, HoverPointComp->GetComponentLocation());

            AverageHoverForce += SuspensionForce;
            HoverPointCount++;
        }
    }

    // --- VFX Dust---
	if (bIsOnGround && DustComponent)
	{
		FVector Center = SumContactLocations / ContactCount;
		DustComponent->SetWorldLocation(Center);
		
		FHitResult AvgHit;
		TraceGroundAtLocation(nullptr, AvgHit, FLinearColor::Yellow);
		
		float CurrDist = AvgHit.bBlockingHit ? AvgHit.Distance : DustSpawnThreshold;
		float Intensity = FMath::Clamp(1.f - (CurrDist / DustSpawnThreshold), 0.f, 1.f);

		DustComponent->SetVariableFloat(FName("User.Intensity"), Intensity);
		DustComponent->Activate();
	}
	else if (DustComponent)
	{
		DustComponent->Deactivate();
	}

    // --- Applique la force moyenne ---
    if (bIsOnGround && HoverPointCount > 0)
    {
        AverageHoverForce /= HoverPointCount;
        for (const UHoverPointComponent* HoverPointComp : HoverPoints)
        {
            BaseVehicle->AddImpulseAtLocation(AverageHoverForce, HoverPointComp->GetComponentLocation());
        }
    }
    else if (!bIsOnGround)
    {
        FVector GravityForce = FVector(0,0, GetWorld()->GetDefaultGravityZ()*2.5f);
        for (const UHoverPointComponent* HoverPointComp : HoverPoints)
        {
            ApplyForceAtLocation(GravityForce, HoverPointComp->GetComponentLocation());
        }
    }

    Movement(DeltaTime);
    Frictions();
}

void AHoverVehicles::HandleManualRotation(float DeltaTime)
{
	FVector AngularVelocity = BaseVehicle->GetPhysicsAngularVelocityInDegrees();
    
	if (AngularVelocity.Size() < 0.1f)
	{
		AngularVelocity = FVector::ZeroVector;
	}
    
	if (!FMath::IsNearlyZero(TurnInput))
	{
		FVector DesiredTorque = FVector(0.f, 0.f, TurnInput * TurnForce);
		AddTorque(DesiredTorque);
	}
    
	FVector StabilizingTorque = -AngularVelocity * 4.f;
	AddTorque(StabilizingTorque);
}

//--------------------------- Movement ---------------------------
#pragma region Movement

void AHoverVehicles::Movement(float DeltaTime)
{
 /*--------------- Mouvement ---------------*/
    float CurrentSpeed = BaseVehicle->GetPhysicsLinearVelocity().Size();
    FVector ForwardVector = GetActorForwardVector();
    FVector Velocity = BaseVehicle->GetPhysicsLinearVelocity();
    FVector VelocityDirection = Velocity.GetSafeNormal();
    
    float EffectiveMaxForwardSpeed = bBoostActive ? MaxForwardSpeed * BoostSpeedMultiplier : MaxForwardSpeed;
    float EffectiveMoveForce = bBoostActive ? MoveForce * BoostForceMultiplier : MoveForce;

    // Avancer
    if (ForwardInput > 0 && CurrentSpeed < EffectiveMaxForwardSpeed)
    {
        FVector ForwardForce = ForwardVector * ForwardInput * EffectiveMoveForce;
        ApplyForce(ForwardForce);
    }
    else if (ForwardInput < 0) // Reculer
    {
        if (FVector::DotProduct(VelocityDirection, ForwardVector) > 0)
        {
            FVector BrakeForce = -VelocityDirection * BreakForce;
            ApplyForce(BrakeForce);

            if (CurrentSpeed < 20.f && CurrentSpeed < MaxReverseSpeed)
            {
                FVector ReverseForce = ForwardVector * ForwardInput * MoveForce * ReversMoveForceFactor;
                ApplyForce(ReverseForce);
            }
        }
        else if (CurrentSpeed < MaxReverseSpeed)
        {
        	if (bBoostActive) Server_EndBoost();
        	
            FVector ReverseForce = ForwardVector * ForwardInput * MoveForce * ReversMoveForceFactor;
            ApplyForce(ReverseForce);
        }

        FVector FrontLocation = HoverPoints[0]->GetComponentLocation();
        float PitchForceMagnitude = FMath::Clamp(CurrentSpeed * BrakePitchMultiplier, 0.f, MaxBrakePitchForce);
        FVector PitchForce = FVector(0.f, 0.f, PitchForceMagnitude);
    	
        ApplyForceAtLocation(PitchForce, FrontLocation);
    }

    /*--------------- Rotation ---------------*/
    FVector Torque = FVector(0, 0, GetTurnInput() * TurnForce);
    AddTorque(Torque);
    {
        float SpeedFactor = FMath::Clamp(CurrentSpeed / EffectiveMaxForwardSpeed, 0.f, 1.f);
        float TargetTiltAngle = FMath::Clamp(GetTurnInput() * MaxTiltAngle * SpeedFactor, -MaxTiltAngle, MaxTiltAngle);
    	
        CurrentTiltAngle = FMath::FInterpTo(CurrentTiltAngle, TargetTiltAngle, DeltaTime, 3.f);
    	
        FRotator NewRotation = GetActorRotation();
        NewRotation.Roll = CurrentTiltAngle;
    	
        UpdateVehicleRotation(NewRotation);
    }
}

void AHoverVehicles::Input_OnBoost()
{
	Server_StartBoost();
}

void AHoverVehicles::Server_StartBoost_Implementation()
{
	if (bBoostActive || !bEngineOn) return;
	
	bBoostActive = true;
	bSpeedLineFadingOut = false;
	SpeedLineFadeTime = 0.f;
	
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_SpeedLineFade, this, &AHoverVehicles::Multicast_UpdateSpeedLineFade, 0.02f, true);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_FOV, this, &AHoverVehicles::Multicast_UpdateCameraFOV, 0.02f, true);
	
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_Boost, this, &AHoverVehicles::Server_EndBoost, BoostDuration, false);
}

void AHoverVehicles::Multicast_UpdateSpeedLineFade_Implementation()
{
	if (!SpeedLineInstance) return;

	SpeedLineFadeTime += GetWorld()->GetDeltaSeconds();
	float Alpha = FMath::Clamp(SpeedLineFadeTime / 0.5f, 0.f, 1.f);
	if (bSpeedLineFadingOut) Alpha = 1.f - Alpha;

	SpeedLineInstance->SetScalarParameterValue(TEXT("Opacity"), Alpha);

	if (SpeedLineFadeTime >= 0.5f)
	{
		SpeedLineInstance->SetScalarParameterValue(TEXT("Opacity"), bSpeedLineFadingOut ? 0.f : 1.f);
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_SpeedLineFade);
	}
}

void AHoverVehicles::Multicast_UpdateCameraFOV_Implementation()
{
	if (UCameraComponent* Camera = MainCamera)
	{
		float TargetFOV = bBoostActive ? BoostFOV : NormalFOV;
		float InterpSpeed = bBoostActive ? FOVInterpSpeed_Activation : FOVInterpSpeed_Deactivation;
        
		float CurrentFOV = Camera->FieldOfView;
		float NewFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, GetWorld()->GetDeltaSeconds(), InterpSpeed);
		Camera->SetFieldOfView(NewFOV);
        
		if (FMath::IsNearlyEqual(NewFOV, TargetFOV, 0.5f))
		{
			Camera->SetFieldOfView(TargetFOV);
			GetWorld()->GetTimerManager().ClearTimer(TimerHandle_FOV);
		}
	}
}

void AHoverVehicles::Server_EndBoost_Implementation()
{
	bBoostActive = false;
	
	bSpeedLineFadingOut = true;
	SpeedLineFadeTime = 0.f;
	
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_SpeedLineFade, this, &AHoverVehicles::Multicast_UpdateSpeedLineFade, 0.02f, true);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_FOV, this, &AHoverVehicles::Multicast_UpdateCameraFOV, 0.02f, true);
}

void AHoverVehicles::Frictions()
{
	// Movement Friction
	if (FMath::IsNearlyZero(ForwardInput))
	{
		FVector CurrentVelocity = BaseVehicle->GetPhysicsLinearVelocity();
		FVector FrictionForce = -CurrentVelocity * FrictionFactor;
		ApplyForce(FrictionForce);
	}

	// Rotation Friction
	if (FMath::IsNearlyZero(TurnInput))
	{
		FVector CurrentAngularVelocity = BaseVehicle->GetPhysicsAngularVelocityInDegrees();

		FVector AngularFriction = -CurrentAngularVelocity * RotationFrictionFactor;
		AddTorque(AngularFriction);
	}

	// Anti patinage
	FVector CurrentVelocity = BaseVehicle->GetPhysicsLinearVelocity();
	FVector ForwardVector = GetActorForwardVector();

	FVector LateralVelocity = CurrentVelocity - (FVector::DotProduct(CurrentVelocity, ForwardVector) * ForwardVector);

	FVector LateralFrictionForce = -LateralVelocity * RotationFrictionFactor;
	ApplyForce(LateralFrictionForce);
}

//----------------------- Replication -----------------------
void AHoverVehicles::ApplyForce(const FVector Force)
{
	if(BaseVehicle)
		BaseVehicle->AddForce(Force, NAME_None, true);
}

void AHoverVehicles::ApplyForceAtLocation(const FVector Force, const FVector Location)
{
	if(BaseVehicle)
		BaseVehicle->AddImpulseAtLocation(Force, Location);
}

void AHoverVehicles::UpdateVehicleRotation(const FRotator& NewRotation)
{
	SetActorRotation(NewRotation);
	
}

void AHoverVehicles::AddTorque(const FVector& NewVector)
{
	if(BaseVehicle)
		BaseVehicle->AddTorqueInDegrees(NewVector, NAME_None, true);
}

#pragma endregion

// Trace Ground
float AHoverVehicles::TraceGroundAtLocation(const UHoverPointComponent* HoverPointComp, FHitResult& OutHitResult, FLinearColor TraceColor)
{
	float FloatingDistance;
	FVector StartLocation;
	FVector EndLocation;
	
	if (HoverPointComp)
	{
		FloatingDistance = HoverPointComp->HoverPoint.FloatingDistance;
		StartLocation =  HoverPointComp->GetComponentLocation();
		EndLocation = StartLocation + FVector(0, 0,  FloatingDistance * -2.f);	
	}
	else
	{
		FloatingDistance = 300.f;
		StartLocation =	GetActorLocation();
		EndLocation = StartLocation + FVector(0, 0,  FloatingDistance * -2.f);	
	}
	
	const bool bHit = UKismetSystemLibrary::LineTraceSingle(
		GetWorld(),
		StartLocation,
		EndLocation,
		UEngineTypes::ConvertToTraceType(ECC_Camera),
		false,
		TArray<AActor*>(),
		EDrawDebugTrace::ForOneFrame,
		OutHitResult,
		true,
		TraceColor,
		FLinearColor::Green,
		1.0f
	);

	if (bHit)
	{
		return OutHitResult.Distance;
	}

	return FloatingDistance * 2.f;
}

float AHoverVehicles::TraceGroundAtLocation(FHitResult& HitResult)
{
	return TraceGroundAtLocation(nullptr, HitResult);
}
