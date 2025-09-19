// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RaycastSensor.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class ECastDirection : uint8
{
	Forward,
	Right,
	Up,
	Backward,
	Left,
	Down
};

UCLASS()
class STELLARLOCOMOTIONCHARACTERSMODULE_API URaycastSensor : public UObject
{
	GENERATED_BODY()


public:
	UPROPERTY()
	float CastLength = 1.0f;

	UPROPERTY()
	FVector Origin = FVector::ZeroVector;
	UPROPERTY()
	FTransform Tr;

	UPROPERTY()
	ECastDirection CastDirection;

	FHitResult HitInfo;

private:
	UPROPERTY()
	UWorld* World;

	UPROPERTY()
	AActor* Owner;
public:
	URaycastSensor(const FObjectInitializer& ObjectInitializer);
	void InitRaycastSensor(const FTransform& CharacterTransform, UWorld* CurrentWorld, AActor* CurrentOwner) { Tr = CharacterTransform, World = CurrentWorld, Owner = CurrentOwner; }
	virtual UWorld* GetWorld();
	
	void Cast();
	bool HasDetectionHit() const { return HitInfo.GetComponent() != nullptr; }
	float GetDistance() const { return HitInfo.Distance; }
	FVector GetNormal() const { return HitInfo.ImpactNormal; }
	FVector GetPositon() const { return HitInfo.ImpactPoint; }
	TWeakObjectPtr<UPrimitiveComponent> GetCollider() const { return HitInfo.Component; }
	FTransform GetTransform() const { return HitInfo.GetActor()->GetTransform(); }

	void SetCastDirection(const ECastDirection& Direction) { CastDirection = Direction; }
	void SetCastOrigin(const FVector& pos) { Origin = Tr.InverseTransformPosition(pos); }

	FVector GetCastDirection();
};
