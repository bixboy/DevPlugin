#include "Units/CommanderHive.h"
#include "Components/SphereComponent.h"

// Setup
#pragma region Setup

ACommanderHive::ACommanderHive()
{
	PrimaryActorTick.bCanEverTick = true;

	AreaCommand = CreateDefaultSubobject<USphereComponent>("AreaCommand");
	AreaCommand->SetupAttachment(RootComponent);

	AreaCommand->OnComponentBeginOverlap.AddDynamic(this, &ACommanderHive::OnAreaCommandBeginOverlap);
	AreaCommand->OnComponentEndOverlap.AddDynamic(this, &ACommanderHive::OnAreaCommandEndOverlap);
}

void ACommanderHive::BeginPlay()
{
	Super::BeginPlay();
}

void ACommanderHive::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ACommanderHive::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

#pragma endregion

void ACommanderHive::UpdateMinions()
{
	TArray<AActor*> ValidActors;
    
	for (AActor* Minion : Minions)
	{
		if (Minion && IsValid(Minion))
		{
			ValidActors.Add(Minion);
		}
	}
	Minions = ValidActors;
}

void ACommanderHive::TakeDamage_Implementation(AActor* DamageOwner)
{
	Super::TakeDamage_Implementation(DamageOwner);

	if (!IsValid(CurrentTarget) || !Execute_GetIsInAttack(CurrentTarget)) CurrentTarget = DamageOwner;
	
	FCommandData CommandData = FCommandData(nullptr, FVector::ZeroVector, FRotator::ZeroRotator, ECommandType::CommandAttack, CurrentTarget);
	
	for (AActor* Minion: Minions)
	{
		if (IsValid(Minion) && Minion->Implements<USelectable>())
		{
			if(Execute_GetBehavior(Minion) != ECombatBehavior::Passive)
				ISelectable::Execute_CommandMove(Minion, CommandData);
		}
		else
		{
			UpdateMinions();	
		}
	}
}

void ACommanderHive::OnAreaCommandBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(OtherActor == this) return;
	
	if (OtherActor->Implements<USelectable>())
	{
		if (Execute_GetCurrentTeam(OtherActor) != CurrentTeam) return;
			
		UpdateMinions();
		Minions.AddUnique(OtherActor);
	}
}

void ACommanderHive::OnAreaCommandEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
										UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if(OtherActor == this) return;
	
	if (OtherActor->Implements<USelectable>())
	{
		if (Execute_GetCurrentTeam(OtherActor) != CurrentTeam) return;
		
		UpdateActorsInArea();
		
		if (Minions.Contains(OtherActor))
			Minions.Remove(OtherActor);
	}
}

