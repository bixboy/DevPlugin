#pragma once
#include "CoreMinimal.h"
#include "CameraSystemBase.h"
#include "CameraPreviewSystem.generated.h"

class APreviewPoseMesh;
class APlayerCamera;
class UCameraSpawnSystem;


UCLASS()
class JUPITERPLUGIN_API UCameraPreviewSystem : public UCameraSystemBase
{
	GENERATED_BODY()

public:
	virtual void Init(APlayerCamera* InOwner) override;
    
	virtual void Cleanup();

	virtual void Tick(float DeltaTime) override;

	// --- State ---
	bool EnsurePreviewActor();
	void HidePreview();
	bool IsPreviewVisible() const { return bPreviewVisible; }
	bool HasPreviewActor() const { return PreviewActor != nullptr; }
	APreviewPoseMesh* GetPreviewActor() const { return PreviewActor; }

	// --- Logic ---
	bool IsPlacementValid() const;

	// --- Visuals ---
	bool ShowSkeletalPreview(USkeletalMesh* Mesh, const FVector& Scale, int32 Count);
	bool ShowStaticPreview(UStaticMesh* Mesh, const FVector& Scale, int32 Count);

	void UpdateInstances(const TArray<FTransform>& Transforms);
	void SetPreviewTransform(const FVector& Location, const FRotator& Rotation);

	// --- Dependency Injection ---
	void SetSpawnSystem(UCameraSpawnSystem* InSpawn) { SpawnSystem = InSpawn; }

private:
	bool InternalCreatePreviewActor(TSubclassOf<APreviewPoseMesh> PreviewClass);

	UPROPERTY()
	TObjectPtr<APreviewPoseMesh> PreviewActor;

	UPROPERTY()
	TObjectPtr<UCameraSpawnSystem> SpawnSystem;

	bool bPreviewVisible = false;
};