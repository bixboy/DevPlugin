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
	virtual void Init(APlayerCamera* InOwner) override;

	// --- Input Handlers ---
	void HandleMoveInput(const FInputActionValue& Value);
	void HandleMoveReleased(const FInputActionValue& Value);
	void HandleZoomInput(const FInputActionValue& Value);
	void HandleRotateHorizontal(const FInputActionValue& Value);
	void HandleRotateVertical(const FInputActionValue& Value);
	void HandleRotateToggle(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void MoveToLocation(const FVector& TargetLocation, float Duration = 1.0f);

private:
	// Helpers
	FVector2D GetEdgeScrollInput() const;
	void UpdateTerrainFollow(float DeltaTime);
	void ClampCameraPitch();

private:
	// --- Input State ---
	FVector2D PendingMoveInput = FVector2D::ZeroVector;
	float PendingZoomInput = 0.f;
	float PendingRotateH = 0.f;
	float PendingRotateV = 0.f;
	bool bRotateEnabled = false;

	// --- Smooth Terrain Data ---
	float CurrentTerrainHeight = 0.f;

	// --- Smooth Move To Location Data ---
	bool bIsMovingToLocation = false;
	FVector MoveToStartLocation = FVector::ZeroVector;
	FVector MoveToTargetLocation = FVector::ZeroVector;
	float MoveToAlpha = 0.f;
	float MoveToDuration = 1.f;
};