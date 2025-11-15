#pragma once

#include "CoreMinimal.h"
#include "JupiterMathUtils.generated.h"

/**
 * Tracks the state of a rotation preview gesture (hold, activate, update).
 */
USTRUCT(BlueprintType)
struct JUPITERPLUGIN_API FRotationPreviewState
{
    GENERATED_BODY()

    /** Whether the hold interaction is currently active. */
    UPROPERTY()
    bool bHoldActive = false;

    /** Whether the preview is currently visible. */
    UPROPERTY()
    bool bPreviewActive = false;

    /** Time at which the hold began. */
    UPROPERTY()
    float HoldStartTime = 0.f;

    /** Cached center of the preview formation. */
    UPROPERTY()
    FVector Center = FVector::ZeroVector;

    /** Cached initial direction from the center to the mouse. */
    UPROPERTY()
    FVector InitialDirection = FVector::ZeroVector;

    /** Rotation at the beginning of the hold. */
    UPROPERTY()
    FRotator BaseRotation = FRotator::ZeroRotator;

    /** Current rotation applied to the preview. */
    UPROPERTY()
    FRotator CurrentRotation = FRotator::ZeroRotator;

    /** Resets the state to the provided base rotation. */
    void Reset(const FRotator& InBaseRotation);

    /** Starts tracking the hold gesture. */
    void BeginHold(float CurrentTime, const FVector& InitialCenter, const FRotator& InBaseRotation);

    /** Stops tracking the hold gesture without activating preview. */
    void StopHold();

    /** Attempts to activate the preview if the hold duration threshold is met. */
    bool TryActivate(float CurrentTime, float HoldThreshold, const FVector& ActivationCenter, const FVector& MouseLocation);

    /** Cancels the preview entirely. */
    void Deactivate();

    /** Updates the preview rotation based on the current mouse position. */
    void UpdateRotation(const FVector& MouseLocation);

    /** Projects a direction onto the XY plane, using fallback rotation when necessary. */
    static FVector ResolvePlanarDirection(const FVector& Direction, const FRotator& FallbackRotation);

    /** Computes a new rotation from the delta between the initial and current mouse directions. */
    static FRotator ComputeRotationFromMouseDelta(const FVector& Center, const FVector& InitialDirection, const FRotator& BaseRotation, const FVector& MouseLocation);
};

namespace JupiterMathUtils
{
    /** Converts a viewport coordinate into normalized [0,1] space. */
    JUPITERPLUGIN_API FVector2D NormalizeViewportCoordinates(const FVector2D& MousePosition, const FVector2D& ViewportSize);
}

