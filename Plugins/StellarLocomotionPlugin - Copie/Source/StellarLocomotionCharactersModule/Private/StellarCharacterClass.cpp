// Fill out your copyright notice in the Description page of Project Settings.


#include "StellarCharacterClass.h"

#include "Components/CapsuleComponent.h"
#include "SkeletalMeshComponentBudgeted.h"
#include "Animation/AnimSubsystemInstance.h"

#include "Components/StellarCharacterMotor.h"
#include "Components/StellarPhysicCharacterMotor.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AStellarCharacterClass::AStellarCharacterClass()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->SetCapsuleHalfHeight(88.0f);
	Capsule->SetCapsuleRadius(34.0f);
	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Capsule->SetCollisionObjectType(ECC_Pawn);
	Capsule->SetCollisionResponseToAllChannels(ECR_Block);
	Capsule->SetSimulatePhysics(true);
	Capsule->SetEnableGravity(true);

	RootComponent = Capsule;
	
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponentBudgeted>(TEXT("SkeletalMesh"));

	SkeletalMeshComponent->SetupAttachment(RootComponent);
	SkeletalMeshComponent->SetRelativeLocation(FVector(0,0,-90.0f));
	SkeletalMeshComponent->SetRelativeRotation(FRotator(0,-90.0f,0));
}

void AStellarCharacterClass::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	CharacterMotor = NewObject<UStellarPhysicCharacterMotor>(this);
	CharacterMotor->InitCharacterMotor(this, Capsule);
}

// Called when the game starts or when spawned
void AStellarCharacterClass::BeginPlay()
{
	Super::BeginPlay();
	
	SetMaxSpeed(WalkForwardSpeed);

	if(HasAuthority())
	{
		CycleToOptimizedCharacter();
	}
}

void AStellarCharacterClass::OptimizingCycle()
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStellarCharacterClass::StaticClass(), FoundActors);

	float maxDistSqr = 2500.f * 2500.f;
	FoundActors.Remove(this);
	for (AActor* Actor : FoundActors)
	{
		AStellarCharacterClass* CurrentActor = Cast<AStellarCharacterClass>(Actor);
		if (CurrentActor)
		{
			//GEngine->AddOnScreenDebugMessage(-1,1.f, FColor::Purple,FString::SanitizeFloat(dist));
			float distSqr = FVector::DistSquared(GetActorLocation(), CurrentActor->GetActorLocation());
			
			if (distSqr > maxDistSqr && !CurrentActor->SkeletalMeshComponent->bRenderStatic)
			{
				CurrentActor->CycleToOptimizedCharacter();
			
			}else if(distSqr < maxDistSqr && CurrentActor->SkeletalMeshComponent->bRenderStatic)
			{
				CurrentActor->CycleToUnOptimizedCharacter();
			}
		}
	}
	
}

void AStellarCharacterClass::CycleToOptimizedCharacter()
{
	SkeletalMeshComponent->SetVisibility(false);
	SkeletalMeshComponent->SetComponentTickEnabled(false);
	SkeletalMeshComponent->bPauseAnims = true;
	SkeletalMeshComponent->bRenderStatic = true;

	//SkeletalMeshComponent->Deactivate();
}

void AStellarCharacterClass::CycleToUnOptimizedCharacter()
{
	SkeletalMeshComponent->SetVisibility(true);
	SkeletalMeshComponent->SetComponentTickEnabled(true);
	SkeletalMeshComponent->bPauseAnims = false;
	SkeletalMeshComponent->bRenderStatic = false;

	//SkeletalMeshComponent->Activate();
}

void AStellarCharacterClass::EnableAnimationBudget(bool bEnable)
{
	IConsoleVariable* AnimationBudgetCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("a.AnimationBudget.Enable"));
	if (AnimationBudgetCVar)
	{
		AnimationBudgetCVar->Set(bEnable ? 1 : 0);
	}
}

void AStellarCharacterClass::SetAnimationBudgetParameters(float BudgetMs, float MinQuality, float MaxTickRate, float MinDeltaTime)
{
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("a.AnimationBudget.BudgetInMs")))
		CVar->Set(BudgetMs);

	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("a.AnimationBudget.MinQuality")))
		CVar->Set(MinQuality);

	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("a.AnimationBudget.MaxTickRate")))
		CVar->Set(MaxTickRate);

	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("a.AnimationBudget.MinDeltaTime")))
		CVar->Set(MinDeltaTime);
}

void AStellarCharacterClass::OnReplicated_PlayerController()
{
	Super::OnReplicated_PlayerController();

	GEngine->AddOnScreenDebugMessage(-1,1.f, FColor::Purple,"ON REP PLAYER CONTROLLER");

	if(GetController() && GetController()->IsLocalPlayerController())
	{
		GEngine->AddOnScreenDebugMessage(-1,1.f, FColor::Purple,"SET AUTO REGISTER TO FALSE");

		SkeletalMeshComponent->SetAutoRegisterWithBudgetAllocator(false);

		// Call MyFunction once after 3 seconds
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AStellarCharacterClass::OptimizingCycle, 0.5f, true);
		
		//
		// SetAnimationBudgetParameters(
		// 0.3f,   // BudgetInMs
		// 0.0f,   // MinQuality
		// 10.0f,  // MaxTickRate
		// 0.016f  // MinDeltaTime
		// );
		//
		// EnableAnimationBudget(true);
	}else
	{
		
	}
	
	
	
}

// Called every frame
void AStellarCharacterClass::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Si il y a un controller et que c'est bien le controller qui commande ce controllable.
	// On Update.
	if(GetController() && GetController()->IsLocalPlayerController())
	{
		if(CharacterMotor) CharacterMotor->Tick(DeltaTime);
		//PostPhysicTick_Delegate.Execute(DeltaTime);
		ReplicateCharacterTransform();
	}
	// Si il n'y a pas de controller
	// Et qu'on est sur le serveur, on update
	// else if(!GetController() && HasAuthority())
	// {
	// 	CharacterMotor->Tick(DeltaTime);
	// 	//PostPhysicTick_Delegate.Execute(DeltaTime);
	// 	ReplicateCharacterTransform();
	// }
}

void AStellarCharacterClass::OnReplicated_Environement()
{
	Super::OnReplicated_Environement();

	if(GetOwner()) CharacterMotor->SetCharacterEnvironement(Environement);
}


void AStellarCharacterClass::SetMaxSpeed(const float& NewMaxSpeed, bool bSendRpc)
{
	if (CurrentMaxSpeed != NewMaxSpeed)
	{
		CurrentMaxSpeed = NewMaxSpeed;
	}
}

EMovementMode AStellarCharacterClass::GetMovementMode() const
{
	if(CharacterMotor)
	{
		return CharacterMotor->GetMovementMode();
	}else return EMovementMode::MOVE_None;
}

void AStellarCharacterClass::StellarUpdateVelocity_Implementation(FVector& gravityVelocity, FVector& movementVelocity,
                                                                  const float& deltaTime)
{
	ICharacterInterface::StellarUpdateVelocity_Implementation(gravityVelocity, movementVelocity, deltaTime);


	//GEngine->AddOnScreenDebugMessage(-1,1.f, FColor::Green,"UPDATE VELOCITY");

	const auto ForwardDirection{ CharacterMotor->GetCharacterForwardVector() };
	const auto RightDirection{ CharacterMotor->GetCharacterRightVector() };

	FVector MoveDir = (ForwardDirection * MovementInput.Y + RightDirection * MovementInput.X).GetSafeNormal();
	movementVelocity = MoveDir * CurrentMaxSpeed * deltaTime;
	
	if(CharacterMotor->IsGrounded())
	{
		gravityVelocity = FVector::ZeroVector;
	}else
	{
		const float TerminalVelocity = 4000.0f;

		gravityVelocity = CharacterMotor->GetGlobalUpVector() * -980.0f * deltaTime;
		gravityVelocity = gravityVelocity.GetClampedToMaxSize(TerminalVelocity);
	}
	
	MovementInput = FVector::ZeroVector;
}

void AStellarCharacterClass::StellarUpdateRotation_Implementation(FQuat& currentRotation, const float& deltaTime)
{
	ICharacterInterface::StellarUpdateRotation_Implementation(currentRotation, deltaTime);

	FRotator NewRotation = RotationInput * 45.0f * deltaTime;
	currentRotation = NewRotation.Quaternion();
	RotationInput = FRotator::ZeroRotator;
}

