#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Utils/JupiterMathUtils.h"
#include "Data/AiData.h"
#include "CameraCommandSystem.generated.h"

class AActor;
class APlayerCamera;
class APlayerController;
class UCameraPreviewSystem;
class UCameraPatrolSystem;
class UCameraSpawnSystem;
class UUnitFormationComponent;
class UUnitOrderComponent;
class UUnitSelectionComponent;

/**
 * Manages unit command issuing, rotation previews, and interaction with order-related systems.
 */
UCLASS()
class JUPITERPLUGIN_API UCameraCommandSystem : public UObject
{
    GENERATED_BODY()

public:
    /** Captures owner references and cached components. */
    void Init(APlayerCamera* InOwner);

    /** Updates active command previews. */
    void Tick(float DeltaTime);

    /** Starts the command flow when the player presses the command action. */
    void HandleCommandActionStarted();

    /** Finalises the command when the player releases the command action. */
    void HandleCommandActionCompleted();

    /** Destroys all selected actors. */
    void HandleDestroySelected();

    /** Server-side actor destruction, routed from the owner pawn. */
    void HandleServerDestroyActor(const TArray<AActor*>& ActorsToDestroy);

    /** Triggered when the selection button is pressed to cache potential targets. */
    void NotifySelectionPressed(const FVector& GroundLocation);

    /** Wires the shared preview system used to display ghosts. */
    void SetPreviewSystem(UCameraPreviewSystem* InPreviewSystem) { PreviewSystem = InPreviewSystem; }

    /** Links the patrol system for rotation preview callbacks. */
    void SetPatrolSystem(UCameraPatrolSystem* InPatrolSystem) { PatrolSystem = InPatrolSystem; }

    /** Provides access to the spawn system for shared previews. */
    void SetSpawnSystem(UCameraSpawnSystem* InSpawnSystem) { SpawnSystem = InSpawnSystem; }

    /** Issues patrol orders with the provided radius. */
    void IssuePatrolCommand(float Radius);

private:
    /** Begins the rotation preview and ghost placement. */
    void StartCommandPreview();

    /** Clears the command preview state. */
    void StopCommandPreview();

    /** Tick helper to drive preview updates. */
    void UpdateCommandPreview(float CurrentTime);

    /** Attempts to activate the preview given the mouse position. */
    bool TryActivateCommandPreview(float CurrentTime, const FVector& MouseLocation);

    /** Ensures the preview mesh matches the current selection. */
    bool EnsurePreviewMeshForSelection();

    /** Builds preview transforms for the currently selected formation. */
    void BuildCommandPreviewTransforms(const FVector& CenterLocation, const FRotator& FacingRotation);

    /** Constructs the command payload sent to units. */
    FCommandData BuildCommandData(ECommandType Type, AActor* TargetActor = nullptr, float Radius = 0.f) const;

    /** Utility to retrieve the actor under the cursor. */
    AActor* GetActorUnderCursor() const;

    TWeakObjectPtr<APlayerCamera> Owner;
    TWeakObjectPtr<APlayerController> Player;
    TObjectPtr<UUnitSelectionComponent> SelectionComponent = nullptr;
    TObjectPtr<UUnitOrderComponent> OrderComponent = nullptr;
    TObjectPtr<UUnitFormationComponent> FormationComponent = nullptr;

    TWeakObjectPtr<UCameraPreviewSystem> PreviewSystem;
    TWeakObjectPtr<UCameraPatrolSystem> PatrolSystem;
    TWeakObjectPtr<UCameraSpawnSystem> SpawnSystem;

    UPROPERTY(EditAnywhere, Category="Commands")
    float RotationPreviewHoldTime = 1.f;

    FVector CommandLocation = FVector::ZeroVector;
    bool bCommandActionHeld = false;
    bool bCommandPreviewVisible = false;
    FRotationPreviewState CommandRotationPreview;
};

