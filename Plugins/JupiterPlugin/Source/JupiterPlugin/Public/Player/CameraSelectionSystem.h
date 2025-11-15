#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "UObject/Object.h"
#include "CameraSelectionSystem.generated.h"

class APlayerCamera;
class APlayerController;
class ASelectionBox;
class ASoldierRts;
class UCameraCommandSystem;
class UCameraSpawnSystem;
class UCameraPatrolSystem;
class UUnitSelectionComponent;
class UUnitSpawnComponent;

/**
 * Responsible for all unit selection behaviours including single, box, and double-click selection.
 */
UCLASS()
class JUPITERPLUGIN_API UCameraSelectionSystem : public UObject
{
    GENERATED_BODY()

public:
    /** Initialises references to the owning pawn and selection components. */
    void Init(APlayerCamera* InOwner);

    /** Updates box selection visuals and ground hit tests. */
    void Tick(float DeltaTime);

    /** Begins a selection interaction from mouse input. */
    void HandleSelectionPressed();

    /** Completes a selection interaction and applies results. */
    void HandleSelectionReleased();

    /** Maintains box selection while the mouse is held. */
    void HandleSelectionHold(const FInputActionValue& Value);

    /** Selects every controllable unit. */
    void HandleSelectAll();

    /** Registers the command system dependency. */
    void SetCommandSystem(UCameraCommandSystem* InCommandSystem) { CommandSystem = InCommandSystem; }

    /** Registers the spawn system dependency. */
    void SetSpawnSystem(UCameraSpawnSystem* InSpawnSystem) { SpawnSystem = InSpawnSystem; }

    /** Registers the patrol system dependency. */
    void SetPatrolSystem(UCameraPatrolSystem* InPatrolSystem) { PatrolSystem = InPatrolSystem; }

    /** Returns the last valid ground hit under the cursor. */
    FVector GetLastGroundLocation() const { return LastGroundLocation; }

private:
    /** Spawns the selection box actor when required. */
    void EnsureSelectionBox();

    /** Tracks the cursor projection on the terrain. */
    void UpdateMouseGroundLocation();

    /** Retrieves any actor under the cursor. */
    AActor* GetActorUnderCursor() const;

    /** Helper to expose whether the cursor is over valid ground. */
    bool IsMouseGrounded() const { return bMouseGrounded; }

    /** Clears the current selection. */
    void ClearCurrentSelection();

    /** Starts the box selection workflow. */
    void BeginBoxSelection();

    /** Ends the box selection workflow. */
    void EndBoxSelection();

    /** Selects all units contained within the current camera bounds. */
    void SelectActorsInCameraBounds();

    /** Gathers actors of the requested class currently visible in the viewport. */
    template <typename T>
    void GatherActorsInView(TSubclassOf<T> ActorClass, TArray<T*>& OutActors) const;

    TWeakObjectPtr<APlayerCamera> Owner;
    TWeakObjectPtr<APlayerController> Player;
    TObjectPtr<UUnitSelectionComponent> SelectionComponent = nullptr;
    TObjectPtr<UUnitSpawnComponent> SpawnComponent = nullptr;

    UPROPERTY(EditAnywhere, Category="Selection")
    float LeftMouseHoldThreshold = 0.15f;

    UPROPERTY(EditAnywhere, Category="Selection")
    TSubclassOf<ASelectionBox> SelectionBoxClass;

    UPROPERTY()
    TObjectPtr<ASelectionBox> SelectionBox = nullptr;

    bool bMouseGrounded = false;
    bool bBoxSelecting = false;
    FVector LastGroundLocation = FVector::ZeroVector;

    TWeakObjectPtr<UCameraCommandSystem> CommandSystem;
    TWeakObjectPtr<UCameraSpawnSystem> SpawnSystem;
    TWeakObjectPtr<UCameraPatrolSystem> PatrolSystem;
};

