// Fill out your copyright notice in the Description page of Project Settings.


#include "Demo/StellarGameModeClass.h"

#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"


void AStellarGameModeClass::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if(NewPlayer)
	{
		GetBasePlayerPawn(NewPlayer);
	}
}

APlayerStart* AStellarGameModeClass::FindRandomPlayerStartLocation()
{
	// The class of actors you want to retrieve
	TSubclassOf<APlayerStart> ActorClass = APlayerStart::StaticClass();

	TArray<AActor*> FoundActors;

	// Retrieve all actors of the specified class
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ActorClass, FoundActors);

	int Index = FMath::RandRange(0, FoundActors.Num() - 1);

	if(IsValid(FoundActors[Index]))
	{
		return Cast<APlayerStart>(FoundActors[Index]);
	}else
	{
		return nullptr;
	}
}


AActor* AStellarGameModeClass::GetBasePlayerPawn(APlayerController* TargetedController)
{
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = nullptr;
	SpawnParameters.Instigator = TargetedController->GetInstigator();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	APlayerStart* Spawn = FindRandomPlayerStartLocation();
	const FVector SpawnLoc = Spawn->GetActorLocation();
	const FRotator SpawnRot = Spawn->GetActorRotation();
	
	TObjectPtr<AActor> PawnActor = nullptr;
	if(Spawn)
	{
		PawnActor = Cast<AActor>(GetWorld()->SpawnActor(EntityCharacterClass, &SpawnLoc, &SpawnRot, SpawnParameters));
	}else
	{
		PawnActor = Cast<AActor>(GetWorld()->SpawnActor(EntityCharacterClass, nullptr, nullptr, SpawnParameters));
		PawnActor->SetActorLocation(FindRandomPlayerStartLocation()->GetActorLocation());
	}

	PawnActor->SetOwner(TargetedController);
	//TargetedController->SetViewTarget(PawnActor);

	return PawnActor;
}