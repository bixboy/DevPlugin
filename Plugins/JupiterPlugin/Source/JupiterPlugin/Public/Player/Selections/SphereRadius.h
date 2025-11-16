#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SphereRadius.generated.h"

class UUnitSelectionComponent;
class USphereComponent;
class UDecalComponent;

UCLASS()
class JUPITERPLUGIN_API ASphereRadius : public AActor
{
	GENERATED_BODY()

public:
	ASphereRadius();
	virtual void Tick(float DeltaTime) override;

	void Start(FVector Position, FRotator Rotation);

	void End();

	float GetRadius() const { return CurrentRadius; }

protected:
	virtual void BeginPlay() override;

	float ComputeRadius();

private:

	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USphereComponent* SphereComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UDecalComponent* DecalComponent;

	// --- Cached gameplay references ---
	UPROPERTY()
	UUnitSelectionComponent* SelectionComponent = nullptr;

	// --- Internal state ---
	UPROPERTY()
	FVector StartLocation = FVector::ZeroVector;

	UPROPERTY()
	FRotator StartRotation = FRotator::ZeroRotator;

	UPROPERTY()
	float CurrentRadius = 0.f;

	UPROPERTY()
	bool bActive = false;
};
