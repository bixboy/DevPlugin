#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "UObject/Object.h"
#include "CameraMovementSystem.generated.h"

class APlayerCamera;
class APlayerController;
class UCameraComponent;
class USpringArmComponent;
class UFloatingPawnMovement;

/**
 * Handles all camera locomotion concerns including translation, rotation and zoom.
 */
UCLASS()
class JUPITERPLUGIN_API UCameraMovementSystem : public UObject
{
    GENERATED_BODY()

public:
    /** Captures the owning pawn and required components. */
    void Init(APlayerCamera* InOwner);

    /** Smoothly interpolates camera properties every frame. */
    void Tick(float DeltaTime);

    /** Processes WASD input and translates the camera. */
    void HandleMoveInput(const FInputActionValue& Value);

    /** Adjusts the camera zoom distance. */
    void HandleZoomInput(const FInputActionValue& Value);

    /** Rotates the camera yaw while rotation is enabled. */
    void HandleRotateHorizontal(const FInputActionValue& Value);

    /** Rotates the camera pitch while rotation is enabled. */
    void HandleRotateVertical(const FInputActionValue& Value);

    /** Toggles whether rotation input should be consumed. */
    void HandleRotateToggle(const FInputActionValue& Value);

private:
    /** Applies movement along the camera plane given input direction. */
    void ApplyMovementInput(const FVector2D& InputVector, float DeltaTime);

    /** Simulates RTS-style edge scrolling. */
    void UpdateEdgeScroll(float DeltaTime);

    /** Keeps camera rotation and roll within configured ranges. */
    void ClampRotation();

    /** Projects the desired location onto the terrain surface. */
    FVector GetGroundedLocation(const FVector& DesiredLocation) const;

    TWeakObjectPtr<APlayerCamera> Owner;
    TWeakObjectPtr<APlayerController> Player;
    TObjectPtr<USpringArmComponent> SpringArm = nullptr;
    TObjectPtr<UCameraComponent> Camera = nullptr;
    TObjectPtr<UFloatingPawnMovement> MovementComponent = nullptr;

    UPROPERTY(EditAnywhere, Category="Camera")
    float CameraSpeed = 20.f;

    UPROPERTY(EditAnywhere, Category="Camera")
    float EdgeScrollSpeed = 2.f;

    UPROPERTY(EditAnywhere, Category="Camera")
    float EdgeScrollThreshold = 0.02f;

    UPROPERTY(EditAnywhere, Category="Camera")
    float RotateSpeed = 2.f;

    UPROPERTY(EditAnywhere, Category="Camera")
    float RotatePitchMin = 10.f;

    UPROPERTY(EditAnywhere, Category="Camera")
    float RotatePitchMax = 80.f;

    UPROPERTY(EditAnywhere, Category="Camera")
    float ZoomSpeed = 2.f;

    UPROPERTY(EditAnywhere, Category="Camera")
    float MinZoom = 500.f;

    UPROPERTY(EditAnywhere, Category="Camera")
    float MaxZoom = 4000.f;

    UPROPERTY(EditAnywhere, Category="Camera")
    bool bEnableEdgeScroll = false;

    FRotator TargetRotation = FRotator(-50.f, 0.f, 0.f);
    float TargetZoom = 3000.f;
    bool bCanRotate = false;
};

