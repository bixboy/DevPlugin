#pragma once
#include "CoreMinimal.h"
#include "Vehicles/VehicleMaster.h"
#include "HoverVehicles.generated.h"

class UNiagaraComponent;
class UHoverPointComponent;


UCLASS()
class STELLARLOCOMOTIONVEHICLESMODULE_API AHoverVehicles : public AVehicleMaster
{
	GENERATED_BODY()

public:
	AHoverVehicles();
	
	virtual void Tick(float DeltaTime) override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	/*- Vehicle Variables -*/
	UPROPERTY()
	TArray<UHoverPointComponent*> HoverPoints;

	UPROPERTY(EditAnywhere, Category = "Settings|Hover")
	float MaxSurfaceAngle = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Vehicle|Inputs",  Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> BoostAction;

	// Oscillation
	UPROPERTY(EditAnywhere, Category = "Settings|Hover|Oscillation")
	float OscillationFrequency = 3.0f; 
	UPROPERTY(EditAnywhere, Category = "Settings|Hover|Oscillation")
	float OscillationAmplitude = 2000.0f;
	UPROPERTY(EditAnywhere, Category = "Settings|Hover|Oscillation")
	float OscillationSmoothing = 10.0f;

	// Move Force
	UPROPERTY(EditAnywhere, Category = "Settings|Hover|Movement|Speed")
	float MoveForce = 2000.0f;
	UPROPERTY(EditAnywhere, Category = "Settings|Hover|Movement|Speed")
	float ReversMoveForceFactor = 0.5f;

	// Speed
	UPROPERTY(EditAnywhere, Category = "Settings|Hover|Movement|Speed")
	float MaxForwardSpeed = 5000.f;
	UPROPERTY(EditAnywhere, Category = "Settings|Hover|Movement|Speed")
	float MaxReverseSpeed = 200.f;

	UPROPERTY(EditAnywhere, Category = "Settings|Hover|Movement|Speed")
	float BreakForce = 1250.f;

	// Boost
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Settings|Hover|Movement|Boost")
	bool bBoostActive = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Hover|Movement|Boost")
	float BoostForceMultiplier = 2.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Hover|Movement|Boost")
	float BoostSpeedMultiplier = 1.5f;

	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Hover|Movement|Boost|Camera")
	float NormalFOV = 90.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Hover|Movement|Boost|Camera")
	float BoostFOV = 110.f;
	

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Hover|Movement|Boost|Camera")
	float FOVInterpSpeed_Deactivation = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Hover|Movement|Boost|Camera")
	float FOVInterpSpeed_Activation = 1.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Hover|Movement|Boost")
	float BoostDuration = 3.0f;

	FTimerHandle TimerHandle_SpeedLineFade;
	FTimerHandle TimerHandle_Boost;
	FTimerHandle TimerHandle_FOV;

	// VFX
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Hover|VFX")
	UMaterial* SpeedLineMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* SpeedLineInstance;

	UPROPERTY(Replicated)
	bool bSpeedLineFadingOut = false;
	
	UPROPERTY(Replicated)
	float SpeedLineFadeTime = 0.f;

	// ---------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UNiagaraComponent* DustComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Hover|VFX")
	float DustSpawnThreshold = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Hover|VFX")
	float DustRadius = 50.f;

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Hover|VFX")
	float DustSize = 100.f;

	// Break Factor
	UPROPERTY(EditAnywhere, Category = "Settings|Hover|Movement|Break")
	float BrakePitchMultiplier = 1.5f;
	UPROPERTY(EditAnywhere, Category = "Settings|Hover|Movement|Break")
	float MaxBrakePitchForce = 5000.f;

	// Rotation
	UPROPERTY(EditAnywhere, Category = "Settings|Hover|Movement|Turn")
	float TurnForce = 150.0f;
	UPROPERTY(EditAnywhere, Category = "Settings|Hover|Movement|Turn")
	float MaxTiltAngle = 30.f;

	// Friction
	UPROPERTY(EditAnywhere, Category = "Settings|Hover|Movement|Speed")
	float FrictionFactor = 2.f;
	UPROPERTY(EditAnywhere, Category = "Settings|Hover|Movement|Turn")
	float RotationFrictionFactor = 0.1f;

	UPROPERTY()
	float OscillationValue = 0.0f;
	
	UPROPERTY()
	float CurrentTiltAngle = 0.0f;

	UPROPERTY(Replicated)
	FRotator BaseRotation;
	
	/*- Function -*/

	virtual void Server_SwitchEngine(bool OnOff) override;
	
	UFUNCTION()
	void Hovering(float DeltaTime);

	UFUNCTION()
	void HandleManualRotation(float DeltaTime);

	UFUNCTION()
	void Movement(float DeltaTime);

	UFUNCTION()
	void Input_OnBoost();

	UFUNCTION(NetMulticast, reliable)
	void Multicast_UpdateSpeedLineFade();

	UFUNCTION(Server, Reliable)
	void Server_StartBoost();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_UpdateCameraFOV();

	UFUNCTION(Server, Reliable)
	void Server_EndBoost();

	UFUNCTION()
	void Frictions();
	
	UFUNCTION()
	float TraceGroundAtLocation(const UHoverPointComponent* HoverPointComp, FHitResult& HitResult, FLinearColor TraceColor = FLinearColor::Red);
	float TraceGroundAtLocation(FHitResult& HitResult);
	
	// Utilities
	UFUNCTION()
	void ApplyForce(const FVector Force);

	UFUNCTION()
	void ApplyForceAtLocation(const FVector Force, const FVector Location);

	UFUNCTION()
	void UpdateVehicleRotation(const FRotator& NewRotation);

	UFUNCTION()
	void AddTorque(const FVector& NewVector);

	void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
};
