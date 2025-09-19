// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StellarControllableClass.generated.h"

struct FInputBindingHandle;
class UInputAction;
class UInputMappingContext;

USTRUCT(BlueprintType)
struct FControllableCompressedTransform
{
	GENERATED_BODY()

	UPROPERTY()
	FVector_NetQuantize10 Position; // 1 decimal precision, 30 bits total

	UPROPERTY()
	FRotator Rotation; // Or quantized FQuat
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRepPlayerController, APlayerController*, NewPlayerController);


UCLASS()
class STELLARLOCOMOTIONPLUGIN_API AStellarControllableClass : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Transient, ReplicatedUsing = "OnReplicated_Environement")
	TObjectPtr<AActor> Environement;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, ReplicatedUsing = "OnReplicated_PlayerController")
	APlayerController* EntityPlayerController;

	UPROPERTY()
	APlayerController* SavedOldPlayerController = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inputs", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(BlueprintAssignable)
	FOnRepPlayerController OnRepPlayerController_Delegate;
	
protected:
	FVector MovementInput;
	FRotator RotationInput;

	UPROPERTY()
	TArray<uint32> StoredInputHandles;



	
public:
	// Sets default values for this actor's properties
	AStellarControllableClass();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#pragma region Ownership Functions
	virtual void SetOwner(AActor* NewOwner) override;
	virtual void OnRep_Owner() override;

	UFUNCTION(BlueprintPure)
	virtual APlayerController* GetController();

	
private:
	UFUNCTION(BlueprintCallable)
	virtual void PossessControllable(APlayerController* NewController);
	UFUNCTION(BlueprintCallable)
	virtual void UnPossessControllable();

	UFUNCTION(Client, Reliable)
	virtual void Client_Reliable_CallPossess(APlayerController* NewController);
	UFUNCTION(Client, Reliable)
	virtual void Client_Reliable_CallUnPossess();
	
	virtual void PossessOnClient(APlayerController* NewController);
	virtual void UnPossessOnClient(APlayerController* PreviousOwner);

	virtual void ClearInputBindings(UEnhancedInputComponent* EnhancedInput);
public:
	UFUNCTION(BlueprintCallable)
	void SetEnvironment(AActor* NewEnvironment);
	UFUNCTION()
	virtual void OnReplicated_Environement();
	UFUNCTION()
	virtual void OnReplicated_PlayerController();
#pragma endregion

#pragma region Transform Replication Functions

	UFUNCTION()
	void ReplicateCharacterTransform();

	UFUNCTION(Server, Unreliable)
	void Server_Unreliable_SendTransformPacket(FControllableCompressedTransform Packet);

	UFUNCTION(NetMulticast, Unreliable)
	void NetMulticast_Unreliable_ApplyTransformPacket(FControllableCompressedTransform Packet);
	
#pragma endregion

#pragma region Inputs Functions
	

protected:
	void AddControllerPitchInput(float Val);
	void AddControllerYawInput(float Val);
	void AddControllerRollInput(float Val);

	UFUNCTION(BlueprintCallable)
	void AddPitchInput(float Val);
	UFUNCTION(BlueprintCallable)
	void AddYawInput(float Val);
	UFUNCTION(BlueprintCallable)
	void AddRollInput(float Val);
	
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent);

#pragma endregion
};
