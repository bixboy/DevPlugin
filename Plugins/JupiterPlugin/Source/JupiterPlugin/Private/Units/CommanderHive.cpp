#include "Units/CommanderHive.h"
#include "Components/SphereComponent.h"


// Setup
#pragma region Setup

ACommanderHive::ACommanderHive()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ACommanderHive::BeginPlay()
{
	Super::BeginPlay();
}

void ACommanderHive::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

#pragma endregion


void ACommanderHive::UpdateMinions()
{
	
}

void ACommanderHive::TakeDamage_Implementation(AActor* DamageOwner)
{
	Super::TakeDamage_Implementation(DamageOwner);
}

