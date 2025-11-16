#pragma once

#include "CoreMinimal.h"
#include "CameraSystemBase.h"
#include "Player/PlayerCameraRotationPreview.h"
#include "CameraPreviewSystem.generated.h"

class APreviewPoseMesh;
class APlayerCamera;
class UCameraCommandSystem;
class UCameraSpawnSystem;

UCLASS()
class JUPITERPLUGIN_API UCameraPreviewSystem : public UCameraSystemBase
{
	GENERATED_BODY()

public:
	virtual void Init(APlayerCamera* InOwner) override;
	virtual void Tick(float DeltaTime) override;

	
	bool EnsurePreviewActor();
	void HidePreview();
	bool IsPreviewVisible() const { return bPreviewVisible; }

	// Spawn preview
	bool ShowSkeletalPreview(USkeletalMesh* Mesh, const FVector& Scale, int32 Count);
	bool ShowStaticPreview(UStaticMesh* Mesh, const FVector& Scale, int32 Count);

	void UpdateInstances(const TArray<FTransform>& Transforms);
	void SetupInstances(const TArray<FTransform>& BaseTransforms);

	bool HasPreviewActor() const { return PreviewActor != nullptr; }

	bool IsPlacementValid() const { return bPreviewVisible; }

	// Links
	void SetSpawnSystem(UCameraSpawnSystem* InSpawn) { SpawnSystem = InSpawn; }

private:

	// Internal
	bool InternalCreatePreviewActor(TSubclassOf<APreviewPoseMesh> PreviewClass);

private:

	UPROPERTY()
	APreviewPoseMesh* PreviewActor = nullptr;

	UPROPERTY()
	UCameraSpawnSystem* SpawnSystem = nullptr;

	bool bPreviewVisible = false;

	TArray<FTransform> CachedTransforms;
};
