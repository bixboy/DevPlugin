#include "Utils/JupiterMathUtils.h"

namespace
{
    FVector NormalizePlanarVector(const FVector& InVector, const FRotator& FallbackRotation)
    {
        FVector Result = InVector;
        Result.Z = 0.f;
        if (Result.Normalize())
        {
            return Result;
        }

        Result = FallbackRotation.Vector();
        Result.Z = 0.f;
        if (Result.Normalize())
        {
            return Result;
        }

        Result = FVector::XAxisVector;
        Result.Z = 0.f;
        Result.Normalize();
        return Result;
    }
}

void FRotationPreviewState::Reset(const FRotator& InBaseRotation)
{
    bHoldActive = false;
    bPreviewActive = false;
    HoldStartTime = 0.f;
    Center = FVector::ZeroVector;
    InitialDirection = FVector::ZeroVector;
    BaseRotation = InBaseRotation;
    CurrentRotation = InBaseRotation;
}

void FRotationPreviewState::BeginHold(float CurrentTime, const FVector& InitialCenter, const FRotator& InBaseRotation)
{
    bHoldActive = true;
    bPreviewActive = false;
    HoldStartTime = CurrentTime;
    Center = InitialCenter;
    BaseRotation = InBaseRotation;
    CurrentRotation = InBaseRotation;
    InitialDirection = FVector::ZeroVector;
}

void FRotationPreviewState::StopHold()
{
    bHoldActive = false;
}

bool FRotationPreviewState::TryActivate(float CurrentTime, float HoldThreshold, const FVector& ActivationCenter, const FVector& MouseLocation)
{
    if (!bHoldActive || bPreviewActive)
    {
        return bPreviewActive;
    }

    if (CurrentTime - HoldStartTime < HoldThreshold)
    {
        return false;
    }

    Center = ActivationCenter;
    BaseRotation = CurrentRotation;
    InitialDirection = ResolvePlanarDirection(MouseLocation - Center, BaseRotation);
    bPreviewActive = true;
    return true;
}

void FRotationPreviewState::Deactivate()
{
    bHoldActive = false;
    bPreviewActive = false;
    Center = FVector::ZeroVector;
    InitialDirection = FVector::ZeroVector;
    BaseRotation = FRotator::ZeroRotator;
    CurrentRotation = FRotator::ZeroRotator;
}

void FRotationPreviewState::UpdateRotation(const FVector& MouseLocation)
{
    if (!bPreviewActive)
    {
        return;
    }

    CurrentRotation = ComputeRotationFromMouseDelta(Center, InitialDirection, BaseRotation, MouseLocation);
}

FVector FRotationPreviewState::ResolvePlanarDirection(const FVector& Direction, const FRotator& FallbackRotation)
{
    return NormalizePlanarVector(Direction, FallbackRotation);
}

FRotator FRotationPreviewState::ComputeRotationFromMouseDelta(const FVector& Center, const FVector& InitialDirection, const FRotator& BaseRotation, const FVector& MouseLocation)
{
    const FVector NormalizedInitial = NormalizePlanarVector(InitialDirection, BaseRotation);
    const FVector CurrentDirection = NormalizePlanarVector(MouseLocation - Center, BaseRotation);

    const float InitialAngle = FMath::Atan2(NormalizedInitial.Y, NormalizedInitial.X);
    const float CurrentAngle = FMath::Atan2(CurrentDirection.Y, CurrentDirection.X);
    const float DeltaDegrees = FMath::RadiansToDegrees(CurrentAngle - InitialAngle);

    FRotator Result = BaseRotation;
    Result.Yaw = FMath::UnwindDegrees(Result.Yaw + DeltaDegrees);
    Result.Pitch = 0.f;
    Result.Roll = 0.f;
    return Result;
}

namespace JupiterMathUtils
{
    FVector2D NormalizeViewportCoordinates(const FVector2D& MousePosition, const FVector2D& ViewportSize)
    {
        if (ViewportSize.X <= 0.f || ViewportSize.Y <= 0.f)
        {
            return FVector2D::ZeroVector;
        }

        return FVector2D(MousePosition.X / ViewportSize.X, MousePosition.Y / ViewportSize.Y);
    }
}

