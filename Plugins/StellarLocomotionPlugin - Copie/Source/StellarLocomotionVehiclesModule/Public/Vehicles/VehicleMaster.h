#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "StellarControllableClass.h"
#include "Camera/CameraComponent.h"
#include "Data/VehicleData.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Interface/VehiclesInteractions.h"
#include "VehicleMaster.generated.h"

class USeatComponent;
class UVehiclePlayerMesh;
class UInputAction;
class UInputMappingContext;
class UVehiclesAnimInstance;
class ACameraVehicle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVehicleMoveDelegate, float, NewForwardInput, float, NewRightInput);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMouseMoveDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEngineChangeDelegate, bool, bEngine);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnterDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnExitDelegate);

USTRUCT()
struct FSeat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Settings|Seats")
	USeatComponent* SeatComponent = nullptr;

	UPROPERTY()
	APlayerController* OccupantController = nullptr;

	UPROPERTY()
	APawn* OriginalPawn = nullptr;
};

UCLASS()
class STELLARLOCOMOTIONVEHICLESMODULE_API AVehicleMaster : public AStellarControllableClass, public IVehiclesInteractions
{
	GENERATED_BODY()

public:
	AVehicleMaster();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	

	UFUNCTION()
	virtual bool Interact_Implementation(ACustomPlayerController* PlayerInteract, USceneComponent* ChosenSeat) override;

	UFUNCTION()
	virtual void OutOfVehicle_Implementation(ACustomPlayerController* PlayerController) override;

	UFUNCTION()
	virtual void Input_OnExit();
	

#pragma region --- Input Setup ---
	
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, Category="Settings|Vehicle")
	UInputMappingContext* NewMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Settings|Vehicle|Inputs", Meta=(DisplayThumbnail=false))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Settings|Vehicle|Inputs", Meta=(DisplayThumbnail=false))
	TObjectPtr<UInputAction> ExitAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Settings|Vehicle|Inputs", Meta=(DisplayThumbnail=false))
	TObjectPtr<UInputAction> SwitchViewModeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Settings|Vehicle|Inputs", Meta=(DisplayThumbnail=false))
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Settings|Vehicle|Inputs", Meta=(DisplayThumbnail=false))
	TObjectPtr<UInputAction> SwitchEngine;
	
#pragma endregion
	

#pragma region --- Components ---
	
	// Variables
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess=true))
	UStaticMeshComponent* BaseVehicle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Components", meta=(AllowPrivateAccess=true))
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess=true))
	UCameraComponent* MainCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess=true))
	USkeletalMeshComponent* SkeletalBaseVehicle;

	UPROPERTY()
	UVehiclesAnimInstance* AnimInstance;
	
#pragma endregion
	

#pragma region --- Camera System ---
	
	// Variables
	UPROPERTY(EditAnywhere, Category="Settings|Camera")
	float CameraDistance = 700.f;

	UPROPERTY(EditAnywhere, Category="Settings|Camera")
	float Sensitivity = 1.5f;

	UPROPERTY(Replicated, EditAnywhere, Category="Settings|Camera")
	bool bCanRotateCamera = true;

	UPROPERTY()
	FRotator CameraRotationOffset;

	UPROPERTY()
	FOnMouseMoveDelegate OnMouseMoveDelegate;
	

	// Function
	UFUNCTION()
	virtual void SwitchViewModeVehicle_Implementation(ACustomPlayerController* PlayerController) override;
	
	UFUNCTION()
	void Input_OnUpdateCameraRotation(const FInputActionValue& InputActionValue);

	UFUNCTION()
	void Input_SwitchViewMode();

	UFUNCTION(Server, Reliable)
	void Server_SwitchViewMode(APlayerController* PC);

	UFUNCTION(Client, Reliable)
	void Client_SwitchViewMode(APlayerController* PC, APawn* OriginalPawn);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	ACameraVehicle* GetAttachedCamera(FName ParentName);
	
#pragma endregion
	

#pragma region --- Seating System ---
	
	// Variables
	UPROPERTY(EditAnywhere, Category="Settings|Seats")
	TArray<USeatComponent*> Seats;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings|Seats")
	USceneComponent* SeatDriver;

	UPROPERTY()
	TArray<FSeat> SeatOccupancy;

	
	// Functions
	UFUNCTION()
	void SetupSeats();
	
	UFUNCTION()
	virtual void EnterVehicle(ACustomPlayerController* PC, USceneComponent* Seat);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnEnterSeat(ACharacter* Char, USceneComponent* Seat);

	UFUNCTION()
	APawn* ExitVehicle(ACustomPlayerController* PC);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnExitSeat(ACharacter* Char);

	UFUNCTION()
	FVector FindClosestExitLocation(APlayerController* PC);

public:
	UPROPERTY()
	FOnEnterDelegate OnEnterDelegate;

	UPROPERTY()
	FOnExitDelegate OnExitDelegate;
#pragma endregion
	

#pragma region --- Movement System ---
	
	// Functions
	UFUNCTION()
	void Input_OnMove(const FInputActionValue& InputActionValue);

	UFUNCTION(Server, Reliable)
	void Server_OnMove(float InForward, float InTurn);

	
	// Variables
	UPROPERTY(Replicated)
	float ForwardInput = 0.f;

	UPROPERTY(Replicated)
	float TurnInput = 0.f;

	UPROPERTY(EditAnywhere, Category="Settings|Movement")
	float MaxSpeed = 3000.f;

public:
	UPROPERTY(BlueprintAssignable, Category="MovementSystem")
	FOnVehicleMoveDelegate OnVehicleMove;

	UFUNCTION()
	float GetForwardInput() const;

	UFUNCTION()
	float GetTurnInput() const;
#pragma endregion
	

#pragma region --- Engine & Settings ---
protected:
	
	// Engine on/off
	UFUNCTION(Server, Reliable, BlueprintCallable)
	virtual void Server_SwitchEngine(bool OnOff);

	UFUNCTION(client, Reliable)
	void Input_Client_SwitchEngine();
	

	// Variables
	UPROPERTY(Replicated, EditAnywhere, Category="Settings|Vehicle")
	bool bEngineOn = false;

	UPROPERTY(EditAnywhere, Category="Settings|Vehicle")
	int MaxSeat;

	UPROPERTY()
	int CurrentSeats;

	UPROPERTY(EditAnywhere, Category="Settings|Vehicle")
	float OutOfVehicleOffset = 50.f;

	UPROPERTY(EditAnywhere, Category="Settings|Vehicle")
	bool bHaveTurret = false;

	UPROPERTY(Replicated, BlueprintReadOnly)
	APlayerController* CurrentDriver;

public:
	UPROPERTY()
	FOnEngineChangeDelegate OnEngineChangeDelegate;
	
#pragma endregion
	

#pragma region --- Audio/Sounds ---
protected:
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Audio")
	UAudioComponent* StartEngineAudio;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Audio")
	UAudioComponent* EngineAudio;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Audio")
	UAudioComponent* MovementAudio;

	UPROPERTY(EditAnywhere, Category="Settings|Sounds")
	USoundBase* SoundEngineOn;
	UPROPERTY(EditAnywhere, Category="Settings|Sounds")
	USoundBase* SoundEngineOff;
	UPROPERTY(EditAnywhere, Category="Settings|Sounds")
	USoundBase* SoundIdleLoop;
	UPROPERTY(EditAnywhere, Category="Settings|Sounds")
	USoundBase* SoundStartAcceleration;
	UPROPERTY(EditAnywhere, Category="Settings|Sounds")
	USoundBase* SoundDriveLoop;
	UPROPERTY(EditAnywhere, Category="Settings|Sounds")
	USoundBase* SoundBrakeLoop;
	

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlaySound(USoundBase* Sound, UAudioComponent* AudioComp);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StopSound(UAudioComponent* AudioComp);

	UFUNCTION()
	bool GetSoundIsPlaying(USoundBase* Sound, UAudioComponent* AudioComp);
	
#pragma endregion

	
#pragma region --- Roles System ---

	// Functions
	UFUNCTION()
	void AssignRole(APawn* Player, EVehiclePlaceType RoleName);

	UFUNCTION()
	void ReleaseRole(APawn* Player);

	UFUNCTION(BlueprintCallable)
	APawn* GetPlayerByRole(EVehiclePlaceType RoleName) const;

	UFUNCTION()
	EVehiclePlaceType GetRoleByPlayer(const APawn* Player) const;

	
	// Variables
	UPROPERTY(Replicated)
	TArray<FVehicleRole> VehicleRoles;

	
#pragma endregion
	

#pragma region --- Turret System ---
protected:
	
	// Functions
	UFUNCTION()
	void InitializeTurrets();

	UFUNCTION(BlueprintCallable, NetMulticast, Reliable)
	void Multicast_SetTurretRotation(ACameraVehicle* Camera, UStaticMeshComponent* TurretMesh, FRotator TurretAngle, FVector AimLocation);

	UFUNCTION(BlueprintCallable)
	void ApplyTurretRotation(float RotationSpeed, float DeltaTime, FTurrets& TurretEntry);

	UFUNCTION(BlueprintCallable)
	void ApplyLookAtRotation(float RotationSpeed, float DeltaTime, FTurrets& TurretEntry);

	
	// Variables
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> SmTurrets;

	UPROPERTY()
	FRotator CurrentAngle;

	UPROPERTY(EditAnywhere, Category="Settings|Turret", meta=(EditCondition="bHaveTurret", EditConditionHides))
	bool bUseLookAtRotation = false;

	UPROPERTY(EditAnywhere, Category="Settings|Turret", meta=(EditCondition="bHaveTurret", EditConditionHides))
	float TurretRotationSpeed = 0.07f;

	UPROPERTY(EditAnywhere, Category="Settings|Turret", meta=(EditCondition="bHaveTurret", EditConditionHides))
	float MaxYawRotation = 120;

	UPROPERTY(EditAnywhere, Category="Settings|Turret", meta=(EditCondition="bHaveTurret", EditConditionHides))
	float MaxPitchRotation = 20;

	UPROPERTY(EditAnywhere, Category="Settings|Turret", meta=(EditCondition="bHaveTurret", EditConditionHides))
	float RotationThreshold = 0.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Components", meta=(AllowPrivateAccess=true))
	TArray<ACameraVehicle*> Turrets;

	UPROPERTY()
	TArray<FTurrets> SeatTurrets;
	
#pragma endregion
};