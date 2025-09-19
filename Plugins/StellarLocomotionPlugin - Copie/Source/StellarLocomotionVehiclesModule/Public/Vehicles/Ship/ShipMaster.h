#pragma once
#include "CoreMinimal.h"
#include "Vehicles/VehicleMaster.h"
#include "ShipMaster.generated.h"


class UMouseSteeringShip;

UENUM()
enum class EShipFlightState : uint8 {
	Landed,
	Landing,
	TakingOff,
	Flying
};

UCLASS()
class STELLARLOCOMOTIONVEHICLESMODULE_API AShipMaster : public AVehicleMaster
{
	GENERATED_BODY()
	

public:
	AShipMaster();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Tick(float DeltaTime) override;
	

	UFUNCTION()
	void EngineChange(bool bEngine);
	
	UFUNCTION()
	void HandleThrust(float DeltaTime);

	UFUNCTION()
	void HandleMouseSteering(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void Server_SetMouseSteering(const FVector2D& NormalizedSteering);

	UFUNCTION()
	void Remove_MouseSteering();

	UFUNCTION()
	void Add_MouseSteering();

	virtual void Input_OnExit() override;
	virtual void EnterVehicle(ACustomPlayerController* PC, USceneComponent* Seat) override;

	UFUNCTION()
	FVector2D GetMouseOffset();
	
	UFUNCTION()
	void Input_Thrust(const FInputActionValue& InputActionValue);

	UFUNCTION()
	void Input_ThrustReleased();

	UFUNCTION(Server, Reliable)
	void Server_ThrustReleased();

	UFUNCTION(Server, Reliable)
	void Server_Thrust(float Value);

	UFUNCTION()
	void Input_OnShipSideMove(const FInputActionValue& InputActionValue);

	UFUNCTION(Server, Reliable)
	void Server_OnShipSideMove(float Value);

	UFUNCTION()
	void Input_OnShipLift(const FInputActionValue& InputActionValue);

	UFUNCTION(Server, Reliable)
	void Server_OnShipLift(float LiftInput);

	UFUNCTION()
	void Input_OnBoost();

	UFUNCTION(Server, Reliable)
	void Server_OnBoost();

	UFUNCTION()
	void EndBoost();

	UFUNCTION(Server, Reliable)
	void Server_AttemptLanding();

	UFUNCTION()
	void HandleLanding(float DeltaTime);

	UFUNCTION()
	void StartTakeoff();

	UFUNCTION()
	void HandleTakeoff(float DeltaTime);

	
	// ===== UI =====
	UPROPERTY(EditAnywhere, Category = "Settings|Ship|Ref")
	TSubclassOf<UMouseSteeringShip> ShipHudClass;

	UPROPERTY()
	UMouseSteeringShip* ShipHud;

	
	// ===== Inputs =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Ship|Input",  Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> BoostAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Ship|Input",  Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> ThrustAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Ship|Input",  Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> PivoAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Ship|Input",  Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> LiftAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Ship|Input",  Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> LandingAction;
	

	// ===== Thrust =====
	UPROPERTY(EditAnywhere, Category="Settings|Ship|Thrust")
	float MaxThrust = 10000.f;

	UPROPERTY(EditAnywhere, Category="Settings|Ship|Thrust")
	float MinThrust = -1000.f;
	
	UPROPERTY(EditAnywhere, Category="Settings|Ship|Thrust")
	float ThrustRateUp = 1000.f;

	UPROPERTY(EditAnywhere, Category="Settings|Ship|Thrust")
	float ThrustRateDown = 800.f;

	UPROPERTY(EditAnywhere, Category="Settings|Ship|Thrust")
	float ThrustInterpSpeed = 100.f;
	
	UPROPERTY(EditAnywhere, Category="Settings|Ship|Thrust")
	float ReverseDelay = 0.5f;

	float ReverseHoldTime = 0.f;

	bool  bCanReverse = false;

	float CurrentThrust = 0.f;
	float ThrustInput = 0.f;


	// ===== Speed Factor =====
	UPROPERTY(EditAnywhere, Category="Settings|Ship|SpeedFactor")
	float MinUpSpeedFactor = 0.8f;

	UPROPERTY(EditAnywhere, Category="Settings|Ship|SpeedFactor")
	float MaxDownSpeedFactor = 1.5f;

	
	// ===== Movement =====
	UPROPERTY(EditAnywhere, Category="Settings|Ship|Movement")
	float MaxRollSpeedMultiplier = 3.f;

	UPROPERTY(EditAnywhere, Category = "Settings|Ship|Movement")
	float RollRotationPower = 70.f;

	UPROPERTY(EditAnywhere, Category = "Settings|Ship|Movement")
	float SideSpeedMove = 100.f;

	UPROPERTY(EditAnywhere, Category = "Settings|Ship|Movement")
	float UpDownSpeedMove = 7000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings|Ship|Movement|Mouse")
	float MouseTorque = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings|Ship|Movement|Mouse")
	float MoveNeutralZone = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings|Ship|Movement|Mouse")
	float MaxSteeringRadius = 0.8f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings|Ship|Movement|Mouse", meta = (ClampMin="1.5", UIMin="1.5", ClampMax="4", UIMax="4"))
	float DampingCoeff = 1.8f;

	UPROPERTY(EditAnywhere, Category="Settings|Ship|Movement|Mouse")
	float MaxBankAngle = 45.f;
	
	UPROPERTY(EditAnywhere, Category="Settings|Ship|Movement|Mouse")
	float BankInterpSpeed = 3.f;

	UPROPERTY(VisibleAnywhere, Replicated) 
	EShipFlightState FlightState = EShipFlightState::Landed;

	float SideInput = 0.f;


	// ===== Boost =====
	UPROPERTY(EditAnywhere, Category="Settings|Ship|Boost")
	float BoostMultiplier = 2.0f;
	
	UPROPERTY(EditAnywhere, Category="Settings|Ship|Boost", meta = (ClampMin="0.1", UIMin="0.1", UIMax="10.0"))
	float BoostDuration = 3.0f;

	bool bBoosting;
	FTimerHandle BoostTimerHandle;

	
	// ===== Take off =====

	UPROPERTY(EditAnywhere, Category="Settings|Ship|Takeoff")
	float TakeoffDuration = 2.f;

	UPROPERTY(EditAnywhere, Category="Settings|Ship|Takeoff")
	float TakeoffHeight = 300.f;

	// Internes
	float TakeoffElapsed = 0.f;
	FVector  TakeoffStartLocation;
	

	// ===== Landing =====
	UPROPERTY(EditAnywhere, Category="Settings|Ship|Landing")
	float LandingDistanceThreshold = 500.f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.1", ClampMax = "1"), Category="Settings|Ship|Landing")
	float LandingMaxThrustFactor = 0.1f;

	UPROPERTY(EditAnywhere, Category="Settings|Ship|Landing")
	float LandingSpeed = 200.f;

	UPROPERTY(EditAnywhere, Category="Settings|Ship|Landing")
	float LandingRotationSpeed = 90.f;

	UPROPERTY(EditAnywhere, Category="Settings|Ship|Landing")
	float LegContactDistance = 5.f;
	
	UPROPERTY()
	TArray<USceneComponent*> LandingGears;
	
	UPROPERTY()
	FRotator InitialRotation;

	UPROPERTY()
	float LandingStartZ;

	UPROPERTY()
	float TotalLandingDistance;
	

	UPROPERTY()
	FVector ForwardVelocity;

	UPROPERTY()
	FVector UpWardVelocity;

	UPROPERTY(EditAnywhere, Category = "Settings|Ship")
	float GravityThreshold = 1000.f;
};
