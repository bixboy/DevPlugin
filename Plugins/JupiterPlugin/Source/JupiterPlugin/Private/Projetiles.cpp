#include "Projetiles.h"

AProjetiles::AProjetiles()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AProjetiles::SetOwnerActor(AActor* NewOwner)
{
	OwnerActor = NewOwner;
}

void AProjetiles::BeginPlay()
{
	Super::BeginPlay();
}


