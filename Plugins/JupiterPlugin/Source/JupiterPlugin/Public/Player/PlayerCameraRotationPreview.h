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

        void Reset(const FRotator& InBaseRotation);

        void BeginHold(float CurrentTime, const FVector& InitialCenter, const FRotator& InBaseRotation);

        void StopHold();

        bool TryActivate(float CurrentTime, float HoldThreshold, const FVector& ActivationCenter, const FVector& MouseLocation);

        void Deactivate();

        void UpdateRotation(const FVector& MouseLocation);

        static FVector ResolvePlanarDirection(const FVector& Direction, const FRotator& FallbackRotation);

        static FRotator ComputeRotationFromMouseDelta(const FVector& Center, const FVector& InitialDirection, const FRotator& BaseRotation, const FVector& MouseLocation);
};

