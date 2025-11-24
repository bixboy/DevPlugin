#pragma once
#include "CoreMinimal.h"
#include "CameraSystemBase.h"
#include "InputActionValue.h"
#include "CameraSelectionSystem.generated.h"

class ASelectionBox;
class ASphereRadius;
class UCameraSpawnSystem;
class UCameraCommandSystem;
class UCameraPatrolSystem;


UCLASS()
class JUPITERPLUGIN_API UCameraSelectionSystem : public UCameraSystemBase
{
	GENERATED_BODY()

public:

	virtual void Init(APlayerCamera* InOwner) override;
	virtual void Tick(float DeltaTime) override;

	// Input handlers
	void HandleSelectionPressed();
	void HandleSelectionReleased();
	void HandleSelectionHold(const FInputActionValue& Value);
	void HandleSelectAll();

	// Links from PlayerCamera
	void SetCommandSystem(UCameraCommandSystem* InCmd) { CommandSystem = InCmd; }
	void SetSpawnSystem(UCameraSpawnSystem* InSpawn)  { SpawnSystem = InSpawn; }

private:
	void FinalizeSelection();

	// Helpers
	bool GetMouseHit(FHitResult& OutHit) const;
	AActor* GetHoveredActor() const;
	void StartBoxSelection();
	void EndBoxSelection();
	void ResetSelectionState();

private:
	bool bMouseGrounded = false;
	bool bBoxSelect = false;

	FVector ClickStartLocation = FVector::ZeroVector;

	float LeftMouseHoldThreshold = 0.15f;

	UPROPERTY()
	UCameraCommandSystem* CommandSystem = nullptr;
	
	UPROPERTY()
	UCameraSpawnSystem*  SpawnSystem = nullptr;

	UPROPERTY()
	ASelectionBox* SelectionBox = nullptr;
};
