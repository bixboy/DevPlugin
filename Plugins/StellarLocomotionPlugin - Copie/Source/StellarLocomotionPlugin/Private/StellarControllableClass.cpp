// Fill out your copyright notice in the Description page of Project Settings.


#include "StellarControllableClass.h"


#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "InputMappingContext.h"


// Sets default values
AStellarControllableClass::AStellarControllableClass()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	EntityPlayerController = nullptr;
}

// Called when the game starts or when spawned
void AStellarControllableClass::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AStellarControllableClass::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AStellarControllableClass::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, EntityPlayerController, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Environement, Parameters);
}


#pragma region Ownership Functions
void AStellarControllableClass::SetOwner(AActor* NewOwner)
{
	if(!NewOwner)
	{
		UnPossessControllable();
	}
	
	Super::SetOwner(NewOwner);
	
	if(NewOwner)
	{
		APlayerController* PlayerController = Cast<APlayerController>(NewOwner);
		PossessControllable(PlayerController);

	}else
	{
		//UnPossessControllable();
	}
	
}

void AStellarControllableClass::OnRep_Owner()
{
	Super::OnRep_Owner();
}

APlayerController* AStellarControllableClass::GetController()
{
	if(EntityPlayerController)
	{
		return EntityPlayerController;
	}
	
	return nullptr;
}



void AStellarControllableClass::PossessControllable(APlayerController* NewController)
{
	if(!HasAuthority()) return;
	if(!NewController) return;

	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, "Call Poossess");

	Client_Reliable_CallPossess(NewController);
	EntityPlayerController = NewController;
	NewController->SetViewTarget(this);
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, EntityPlayerController, this)
}

void AStellarControllableClass::UnPossessControllable()
{
	if(!HasAuthority()) return;

	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, "Call Unpossess");

	Client_Reliable_CallUnPossess();
	EntityPlayerController = nullptr;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, EntityPlayerController, this)
}



void AStellarControllableClass::Client_Reliable_CallPossess_Implementation(APlayerController* NewController)
{
	PossessOnClient(NewController);
}

void AStellarControllableClass::Client_Reliable_CallUnPossess_Implementation()
{
	UnPossessOnClient(SavedOldPlayerController);
}

void AStellarControllableClass::PossessOnClient(APlayerController* NewController)
{
	APlayerController* PC = NewController;
	//EntityPlayerController = PC;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, "PossessOnClient");

	
	if(PC && PC->IsLocalController())
	{
		//Keep a back up of Controller to use when doing unpossess work
		SavedOldPlayerController = PC;
		
		auto* NewPlayer{ PC };
		if (IsValid(NewPlayer))
		{
			NewPlayer->InputYawScale_DEPRECATED = 1.0f;
			NewPlayer->InputPitchScale_DEPRECATED = 1.0f;
			NewPlayer->InputRollScale_DEPRECATED = 1.0f;

			auto* InputSubsystem{ ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(NewPlayer->GetLocalPlayer()) };
			if (IsValid(InputSubsystem))
			{
				FModifyContextOptions Options;
				Options.bNotifyUserSettings = true;

				InputSubsystem->AddMappingContext(InputMappingContext, 0, Options);
			}
		}
	
		SetupPlayerInputComponent(PC->InputComponent);
	}
	
}

void AStellarControllableClass::UnPossessOnClient(APlayerController* PreviousOwner)
{
	if(!SavedOldPlayerController) return;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, "UnpossessOnClient");

	auto* OldPlayer{ SavedOldPlayerController };
	if (IsValid(OldPlayer))
	{
		auto* InputSubsystem{ ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(OldPlayer->GetLocalPlayer()) };
		if (IsValid(InputSubsystem))
		{
			FModifyContextOptions Options;
			Options.bNotifyUserSettings = true;
			
			//Remove mapping context
			InputSubsystem->RemoveMappingContext(InputMappingContext, Options);
		}


		//Clear All Binded Inputs to the old Controller
		auto* EnhancedInput{ Cast<UEnhancedInputComponent>(PreviousOwner->InputComponent) };
		if (IsValid(EnhancedInput))
		{
			ClearInputBindings(EnhancedInput);
			// EnhancedInput->ClearActionValueBindings();
			// EnhancedInput->ClearActionBindings();
			// EnhancedInput->ClearActionEventBindings();
		}
	}

	SavedOldPlayerController = nullptr;
}

void AStellarControllableClass::ClearInputBindings(UEnhancedInputComponent* EnhancedInput)
{
	for(uint32 handle : StoredInputHandles)
	{
		EnhancedInput->RemoveBindingByHandle(handle);
	}
}


void AStellarControllableClass::SetEnvironment(AActor* NewEnvironment)
{
	Environement = NewEnvironment;
	OnReplicated_Environement();
}

void AStellarControllableClass::OnReplicated_Environement()
{
	
}


void AStellarControllableClass::OnReplicated_PlayerController()
{
	OnRepPlayerController_Delegate.Broadcast(EntityPlayerController);
	
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "OnREP PLAYER CONTROLLER");
}
#pragma endregion




#pragma region Transform Replication
void AStellarControllableClass::ReplicateCharacterTransform()
{
	FControllableCompressedTransform packet = FControllableCompressedTransform();
	packet.Position = GetActorLocation();
	packet.Rotation = GetActorRotation();
	Server_Unreliable_SendTransformPacket(packet);
}

void AStellarControllableClass::Server_Unreliable_SendTransformPacket_Implementation(FControllableCompressedTransform Packet)
{
	SetActorLocation(Packet.Position);
	SetActorRotation(Packet.Rotation);
	
	NetMulticast_Unreliable_ApplyTransformPacket(Packet);
}


void AStellarControllableClass::NetMulticast_Unreliable_ApplyTransformPacket_Implementation(FControllableCompressedTransform Packet)
{
	if(GetOwner()) return;
	SetActorLocation(Packet.Position);
	SetActorRotation(Packet.Rotation);
}

#pragma endregion





#pragma region Inputs Functions

void AStellarControllableClass::AddControllerPitchInput(float Val)
{
	if (Val != 0.f && GetController() && GetController()->IsLocalPlayerController())
	{
		AddPitchInput(Val);
	}
}

void AStellarControllableClass::AddControllerYawInput(float Val)
{
	if (Val != 0.f && GetController() && GetController()->IsLocalPlayerController())
	{
		AddYawInput(Val);
	}
}

void AStellarControllableClass::AddControllerRollInput(float Val)
{
	if (Val != 0.f && GetController() && GetController()->IsLocalPlayerController())
	{
		AddRollInput(Val);
	}
}


void AStellarControllableClass::AddPitchInput(float Val)
{	
	RotationInput.Pitch += !GetController()->IsLookInputIgnored() ? Val * 1.0f : 0.0f;
}

void AStellarControllableClass::AddYawInput(float Val)
{
	RotationInput.Yaw += !GetController()->IsLookInputIgnored() ? Val *  1.0f : 0.0f;
}

void AStellarControllableClass::AddRollInput(float Val)
{
	RotationInput.Roll += !GetController()->IsLookInputIgnored() ? Val * 1.0f : 0.0f;
}


// Called to bind functionality to input
void AStellarControllableClass::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// if (GEngine)
	// {
	// 	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("SET PLAYER INPUT COMPONENT"));
	// }
	
}


#pragma endregion
