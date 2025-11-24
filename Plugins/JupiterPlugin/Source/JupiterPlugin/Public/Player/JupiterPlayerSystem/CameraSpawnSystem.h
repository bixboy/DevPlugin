#pragma once

#include "CoreMinimal.h"
#include "CameraSystemBase.h"
#include "Player/PlayerCameraRotationPreview.h"
#include "CameraSpawnSystem.generated.h"

class APreviewPoseMesh;
class UCameraPreviewSystem;
class UCameraCommandSystem;
class UUnitSpawnComponent;
class ASoldierRts;

UCLASS()
class JUPITERPLUGIN_API UCameraSpawnSystem : public UCameraSystemBase
{
	GENERATED_BODY()

public:
	virtual void Init(APlayerCamera* InOwner) override;
	virtual void Tick(float DeltaTime) override;

	void HandleSpawnInput();	
	void RefreshPreviewInstances();
	void ResetSpawnState();

	void SetPreviewSystem(UCameraPreviewSystem* InPreview) { PreviewSystem = InPreview; }
	void SetCommandSystem(UCameraCommandSystem* InCmd) { CommandSystem = InCmd; }

private:
	void ShowUnitPreview(TSubclassOf<ASoldierRts> NewUnitClass);
	void UpdatePreviewFollowMouse(float CurrentTime);
	void UpdatePreviewTransforms(const FVector& Center, const FRotator& Facing);

	bool EnsurePreviewActor();
	int32 GetEffectiveSpawnCount() const;

	// Event callbacks
	UFUNCTION()
	void OnUnitClassChanged(TSubclassOf<ASoldierRts> NewUnitClass);

	UFUNCTION()
	void OnSpawnCountChanged(int32 NewCount);

	UFUNCTION()
	void OnFormationChanged(ESpawnFormation NewFormation);

private:

	UPROPERTY()
	UCameraPreviewSystem* PreviewSystem = nullptr;

	UPROPERTY()
	UCameraCommandSystem* CommandSystem = nullptr;

	FRotationPreviewState SpawnRotationPreview;

	bool bIsInSpawnMode = false;
	float RotationPreviewHoldTime = 0.25f;
};
