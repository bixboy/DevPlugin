#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "UObject/Object.h"
#include "Utils/JupiterMathUtils.h"
#include "Data/AiData.h"
#include "CameraSpawnSystem.generated.h"

class APlayerCamera;
class APlayerController;
class UCameraPreviewSystem;
class UCameraCommandSystem;
class UUnitSelectionComponent;
class UUnitSpawnComponent;
class ASoldierRts;

/**
 * Handles spawn mode activation, placement previews, and issuing spawn commands.
 */
UCLASS()
class JUPITERPLUGIN_API UCameraSpawnSystem : public UObject
{
    GENERATED_BODY()

public:
    /** Caches required components from the owning pawn. */
    void Init(APlayerCamera* InOwner);

    /** Updates spawn previews and rotation state. */
    void Tick(float DeltaTime);

    /** Toggles spawn mode when the spawn input is triggered. */
    void HandleSpawnInput();

    /** Called when selections start to cancel spawn mode if necessary. */
    void OnSelectionInteractionStarted();

    /** Registers the preview system dependency. */
    void SetPreviewSystem(UCameraPreviewSystem* InPreviewSystem) { PreviewSystem = InPreviewSystem; }

    /** Registers the command system dependency. */
    void SetCommandSystem(UCameraCommandSystem* InCommandSystem) { CommandSystem = InCommandSystem; }

    /** Exposes whether the system is currently in spawn mode. */
    bool IsInSpawnMode() const { return bIsInSpawnMode; }

private:
    /** Binds to spawn component delegates to react to user configuration changes. */
    void BindSpawnComponentDelegates();

    /** Spawns the preview units for the provided unit class. */
    void ShowUnitPreview(TSubclassOf<class ASoldierRts> NewUnitClass);

    /** Updates the preview instanced mesh transforms. */
    void RefreshPreviewInstances();

    /** Clears spawn state and hides previews. */
    void ResetSpawnState();

    /** Keeps the preview formation following the cursor. */
    void UpdatePreviewFollowMouse(float CurrentTime);

    /** Recomputes formation transforms based on the mouse position. */
    void UpdatePreviewTransforms(const FVector& CenterLocation, const FRotator& FacingRotation);

    UFUNCTION()
    void HandleSpawnCountChanged(int32 NewCount);

    UFUNCTION()
    void HandleSpawnFormationChanged(ESpawnFormation NewFormation);

    UFUNCTION()
    void HandleCustomFormationDimensionsChanged(FIntPoint NewDimensions);

    UFUNCTION()
    void HandleUnitClassChanged(TSubclassOf<class ASoldierRts> NewUnitClass);

    TWeakObjectPtr<APlayerCamera> Owner;
    TWeakObjectPtr<APlayerController> Player;
    TObjectPtr<UUnitSelectionComponent> SelectionComponent = nullptr;
    TObjectPtr<UUnitSpawnComponent> SpawnComponent = nullptr;

    TWeakObjectPtr<UCameraPreviewSystem> PreviewSystem;
    TWeakObjectPtr<UCameraCommandSystem> CommandSystem;

    UPROPERTY(EditAnywhere, Category="Spawn")
    float RotationPreviewHoldTime = 1.f;

    bool bIsInSpawnMode = false;
    FRotationPreviewState SpawnRotationPreview;
};

