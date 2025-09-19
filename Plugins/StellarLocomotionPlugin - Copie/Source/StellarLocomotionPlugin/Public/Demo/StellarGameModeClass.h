// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "StellarGameModeClass.generated.h"

/**
 * 
 */
UCLASS()
class STELLARLOCOMOTIONPLUGIN_API AStellarGameModeClass : public AGameMode
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> EntityCharacterClass;

public:
	virtual void PostLogin(APlayerController* NewPlayer) override;
	
	UFUNCTION(BlueprintCallable, Category = "Spawn")
	APlayerStart* FindRandomPlayerStartLocation();

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	AActor* GetBasePlayerPawn(APlayerController* TargetedController);
};
