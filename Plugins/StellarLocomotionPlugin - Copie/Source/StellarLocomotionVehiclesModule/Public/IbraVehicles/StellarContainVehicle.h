// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StellarControllableClass.h"
#include "StellarContainVehicle.generated.h"

class UBoxComponent;

UCLASS()
class STELLARLOCOMOTIONVEHICLESMODULE_API AStellarContainVehicle : public AStellarControllableClass
{
	GENERATED_BODY()


public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient)
	USceneComponent* VehicleRoot;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient)
	UBoxComponent* TriggerBox;

	UPROPERTY(BlueprintReadOnly, Transient)
	AStellarControllableClass* CollisionInstance = nullptr;
public:
	// Sets default values for this actor's properties
	AStellarContainVehicle();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
						UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
						bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
					  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
