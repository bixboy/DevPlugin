#pragma once
#include "CoreMinimal.h"
#include "CameraSystemBase.h"
#include "InputActionValue.h"
#include "CameraMovementSystem.generated.h"


UCLASS()
class JUPITERPLUGIN_API UCameraMovementSystem : public UCameraSystemBase
{
	GENERATED_BODY()

public:

	virtual void Tick(float DeltaTime) override;

	// Input handlers from PlayerCamera
	void HandleMoveInput(const FInputActionValue& Value);
	void HandleMoveReleased(const FInputActionValue& Value);

	void HandleZoomInput(const FInputActionValue& Value);
	
	void HandleRotateHorizontal(const FInputActionValue& Value);
	void HandleRotateVertical(const FInputActionValue& Value);
	void HandleRotateToggle(const FInputActionValue& Value);

private:

	void ProcessEdgeScroll();
	void UpdateTerrainFollow();
	void ClampCameraPitch();

private:

	FVector2D PendingMoveInput = FVector2D::ZeroVector;
	float PendingZoomInput = 0.f;
	float PendingRotateH = 0.f;
	float PendingRotateV = 0.f;

	bool bRotateEnabled = false;
};
