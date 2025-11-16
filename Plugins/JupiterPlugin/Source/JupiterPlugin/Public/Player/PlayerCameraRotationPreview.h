#pragma once
#include "CoreMinimal.h"
#include "PlayerCameraRotationPreview.generated.h"


USTRUCT(BlueprintType)
struct FRotationPreviewState
{
	GENERATED_BODY()
	
	public:
        bool bHoldActive = false;
        bool bPreviewActive = false;
        float HoldStartTime = 0.f;
        FVector Center = FVector::ZeroVector;
        FVector InitialDirection = FVector::ZeroVector;
        FRotator BaseRotation = FRotator::ZeroRotator;
        FRotator CurrentRotation = FRotator::ZeroRotator;

        /** Resets the preview state using the provided facing rotation as baseline. */
        void Reset(const FRotator& InBaseRotation);

        /** Starts listening for a potential preview hold. */
        void BeginHold(float CurrentTime, const FVector& InitialCenter, const FRotator& InBaseRotation);

        /** Stops listening for a preview hold. */
        void StopHold();

        /** Attempts to activate the preview if the hold threshold has been reached. */
        bool TryActivate(float CurrentTime, float HoldThreshold, const FVector& ActivationCenter, const FVector& MouseLocation);

        /** Deactivates the preview and clears associated state. */
        void Deactivate();

        /** Updates the preview rotation using the current mouse location. */
        void UpdateRotation(const FVector& MouseLocation);

        /** Resolves a planar direction, falling back to the base rotation if needed. */
        static FVector ResolvePlanarDirection(const FVector& Direction, const FRotator& FallbackRotation);

        /** Computes a rotation delta using planar mouse movement. */
        static FRotator ComputeRotationFromMouseDelta(const FVector& Center, const FVector& InitialDirection, const FRotator& BaseRotation, const FVector& MouseLocation);
};

