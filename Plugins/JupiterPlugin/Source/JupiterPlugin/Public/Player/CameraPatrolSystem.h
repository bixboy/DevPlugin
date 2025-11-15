#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "UObject/Object.h"
#include "CameraPatrolSystem.generated.h"

class APlayerCamera;
class APlayerController;
class ASphereRadius;
class UCameraCommandSystem;
class UCameraSelectionSystem;
class UUnitPatrolComponent;

/**
 * Controls alternate command flows (patrol, attack-move) that rely on ALT modifiers and radius previews.
 */
UCLASS()
class JUPITERPLUGIN_API UCameraPatrolSystem : public UObject
{
    GENERATED_BODY()

public:
    /** Sets up references to the owning pawn and patrol component. */
    void Init(APlayerCamera* InOwner);

    /** Updates patrol radius visuals. */
    void Tick(float DeltaTime);

    /** Handles ALT being pressed to start patrol interactions. */
    void HandleAltPressed();

    /** Handles ALT being released to finish patrol interactions. */
    void HandleAltReleased();

    /** Processes held ALT input for radius adjustment. */
    void HandleAltHold(const FInputActionValue& Value);

    /** Updates patrol radius preview while the mouse wheel is used. */
    void HandlePatrolRadius(const FInputActionValue& Value);

    /** Processes left click events while ALT is engaged. */
    bool HandleLeftClick(EInputEvent InputEvent);

    /** Processes right mouse button press events for ALT commands. */
    bool HandleRightClickPressed(bool bAltDown);

    /** Processes right mouse button release events for ALT commands. */
    bool HandleRightClickReleased(bool bAltDown);

    /** Links the command system to forward patrol orders. */
    void SetCommandSystem(UCameraCommandSystem* InCommandSystem) { CommandSystem = InCommandSystem; }

    /** Links the selection system to access ground locations. */
    void SetSelectionSystem(UCameraSelectionSystem* InSelectionSystem) { SelectionSystem = InSelectionSystem; }

    /** Exposes the ALT key state to other systems. */
    bool IsAltPressed() const { return bAltPressed; }

private:
    /** Spawns or reuses the patrol radius actor. */
    void EnsureSphereRadius();

    /** Begins rendering the patrol radius preview at a location. */
    void StartSphereRadiusAtLocation(const FVector& Location);

    /** Stops rendering the patrol radius preview. */
    void StopSphereRadius(bool bResetState = true);

    TWeakObjectPtr<APlayerCamera> Owner;
    TWeakObjectPtr<APlayerController> Player;
    TObjectPtr<UUnitPatrolComponent> PatrolComponent = nullptr;

    UPROPERTY(EditAnywhere, Category="Patrol")
    TSubclassOf<ASphereRadius> SphereRadiusClass;

    UPROPERTY(EditAnywhere, Category="Patrol")
    float LeftMouseHoldThreshold = 0.15f;

    UPROPERTY()
    TObjectPtr<ASphereRadius> SphereRadius = nullptr;

    bool bAltPressed = false;
    bool bSphereRadiusEnabled = false;

    TWeakObjectPtr<UCameraCommandSystem> CommandSystem;
    TWeakObjectPtr<UCameraSelectionSystem> SelectionSystem;
};

