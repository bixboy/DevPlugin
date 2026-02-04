#pragma once
#include "CoreMinimal.h"
#include "CameraSystemBase.h"
#include "InputActionValue.h"
#include "CameraSelectionSystem.generated.h"

class ASelectionBox;
class UCameraPlacementSystem;
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
	
	// --- Control Groups ---
	void HandleControlGroupInput(const FInputActionValue& Value);
	void HandleRecallGroup(int32 Index);
	void HandleSetGroup(int32 Index);
	void HandleClearGroup(int32 Index);

	// --- Dependency Injection ---
	void SetCommandSystem(UCameraCommandSystem* InCmd) { CommandSystem = InCmd; }
	void SetPlacementSystem(UCameraPlacementSystem* InPlacement) { PlacementSystem = InPlacement; }

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
    
    UPROPERTY(EditDefaultsOnly, Category = "Settings")
    float DragStartThreshold = 20.0f; // Pixels

    FVector2D ClickScreenLocation;

	// Systems
	UPROPERTY()
	TObjectPtr<UCameraCommandSystem> CommandSystem;
    
	UPROPERTY()
	TObjectPtr<UCameraPlacementSystem> PlacementSystem;

	UPROPERTY()
	TObjectPtr<ASelectionBox> SelectionBox;

    // --- Patrol Drag State ---
    bool bIsDraggingPatrol = false;
    FGuid DraggedPatrolID;
    int32 DraggedPointIndex = -1;
    FVector DraggingPlaneLocation;

    bool TryStartPatrolDrag();
    void UpdatePatrolDrag();
    void EndPatrolDrag();
};