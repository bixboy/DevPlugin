#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CameraPreviewSystem.generated.h"

class APlayerCamera;
class APlayerController;
class APreviewPoseMesh;
class USkeletalMesh;
class UStaticMesh;

/**
 * Shared preview actor pool for commands and spawn mode ghost meshes.
 */
UCLASS()
class JUPITERPLUGIN_API UCameraPreviewSystem : public UObject
{
    GENERATED_BODY()

public:
    /** Captures owner references. */
    void Init(APlayerCamera* InOwner);

    /** Updates preview actor validation each frame. */
    void Tick(float DeltaTime);

    /** Displays a skeletal mesh preview with instanced scaling. */
    bool ShowSkeletalPreview(USkeletalMesh* Mesh, const FVector& Scale, int32 InstanceCount);

    /** Displays a static mesh preview with instanced scaling. */
    bool ShowStaticPreview(UStaticMesh* Mesh, const FVector& Scale, int32 InstanceCount);

    /** Applies transform updates to the instanced preview mesh. */
    void UpdateInstances(const TArray<FTransform>& InstanceTransforms);

    /** Hides the preview actor entirely. */
    void HidePreview();

    /** Ensures the preview actor exists, spawning it if necessary. */
    bool EnsurePreviewActor();

    /** Returns whether the preview actor is currently allocated. */
    bool HasPreviewActor() const { return PreviewActor != nullptr; }

    /** Returns whether the preview is being shown. */
    bool IsPreviewVisible() const { return bPreviewVisible; }

    /** Validates whether the preview placement is acceptable. */
    bool IsPlacementValid() const;

    /** Provides direct access to the preview actor. */
    APreviewPoseMesh* GetPreviewActor() const { return PreviewActor; }

private:
    /** Spawns the preview actor instance. */
    void SpawnPreviewActor();

    TWeakObjectPtr<APlayerCamera> Owner;
    TWeakObjectPtr<APlayerController> Player;

    UPROPERTY(EditAnywhere, Category="Preview")
    TSubclassOf<APreviewPoseMesh> PreviewActorClass;

    UPROPERTY()
    TObjectPtr<APreviewPoseMesh> PreviewActor = nullptr;

    bool bPreviewVisible = false;
};

