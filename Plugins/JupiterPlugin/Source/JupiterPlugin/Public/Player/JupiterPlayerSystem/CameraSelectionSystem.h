#pragma once
#include "CoreMinimal.h"
#include "CameraSystemBase.h"
#include "InputActionValue.h"
#include "CameraSelectionSystem.generated.h"

class ASelectionBox;
class UCameraSpawnSystem;
class UCameraCommandSystem;


UCLASS()
class JUPITERPLUGIN_API UCameraSelectionSystem : public UCameraSystemBase
{
	GENERATED_BODY()

public:
	virtual void Init(APlayerCamera* InOwner) override;
	virtual void Tick(float DeltaTime) override;

	// --- Input Handlers ---
	void HandleSelectionPressed();
	void HandleSelectionReleased();
	void HandleSelectionHold(const FInputActionValue& Value);
	void HandleSelectAll();

	// --- Dependency Injection ---
	void SetCommandSystem(UCameraCommandSystem* InCmd) { CommandSystem = InCmd; }
	void SetSpawnSystem(UCameraSpawnSystem* InSpawn) { SpawnSystem = InSpawn; }

private:
	void FinalizeSelection();

	// Helpers
	bool GetMouseHitOnTerrain(FHitResult& OutHit) const;
	AActor* GetHoveredActor() const;
    
	// Box Logic
	void StartBoxSelection();
	void UpdateBoxSelection();
	void EndBoxSelection();
    
	// Logic Utils
	bool ShouldAddToSelection() const;

private:
	bool bMouseGrounded = false;
	bool bBoxSelect = false;

	FVector ClickStartLocation = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	float LeftMouseHoldThreshold = 0.15f;

	// Systems
	UPROPERTY()
	TObjectPtr<UCameraCommandSystem> CommandSystem;
    
	UPROPERTY()
	TObjectPtr<UCameraSpawnSystem> SpawnSystem;

	UPROPERTY()
	TObjectPtr<ASelectionBox> SelectionBox;
};