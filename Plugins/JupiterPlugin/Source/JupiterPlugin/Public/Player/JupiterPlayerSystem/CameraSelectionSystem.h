#pragma once
#include "CoreMinimal.h"
#include "CameraSystemBase.h"
#include "InputActionValue.h"
#include "UI/CustomButtonWidget.h"
#include "CameraSelectionSystem.generated.h"

class ASelectionBox;
class UInputMappingContext;
class UInputAction;
class UUnitSpatialGridSubsystem;
class UTooltipSubsystem;
class AActor;
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

public:
    // Tooltip Logic
    UFUNCTION(BlueprintCallable, Category = "Settings|Tooltip")
    void SetTooltipDelay(float NewDelay) { TooltipDelay = NewDelay; }

    UFUNCTION(BlueprintCallable, Category = "Settings|Tooltip")
    float GetTooltipDelay() const { return TooltipDelay; }

protected:
    void UpdateTooltipHover(float DeltaTime);

    UPROPERTY(EditDefaultsOnly, Category = "Settings|Tooltip")
    float TooltipDelay = 1.f;
    
    UPROPERTY(Transient)
    float CurrentHoverTime = 0.0f;

    UPROPERTY(Transient)
    bool bTooltipShown = false;
    
    UPROPERTY(Transient)
    TWeakObjectPtr<AActor> LastTooltipActor;

    UPROPERTY()
    TObjectPtr<UCustomButtonWidget> HoveredButton = nullptr;

public:	
	// --- Dependency Injection ---
	void SetCommandSystem(UCameraCommandSystem* InCmd) { CommandSystem = InCmd; }
	void SetPlacementSystem(UCameraPlacementSystem* InPlacement) { PlacementSystem = InPlacement; }

private:
	void FinalizeSelection();

	bool GetMouseHitOnTerrain(FHitResult& OutHit) const;
	AActor* GetHoveredActor() const;
    
	void StartBoxSelection();
	void UpdateBoxSelection();
	void EndBoxSelection();
    
	bool ShouldAddToSelection() const;

private:
	
	UPROPERTY()
	APlayerController* PC;
	
	bool bMouseGrounded = false;
	bool bBoxSelect = false;

	FVector ClickStartLocation = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	float LeftMouseHoldThreshold = 0.15f;
    
    UPROPERTY(EditDefaultsOnly, Category = "Settings")
    float DragStartThreshold = 20.0f;

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