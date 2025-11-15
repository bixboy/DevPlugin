#include "Player/CameraMovementSystem.h"

#include "Engine/World.h"
#include "Player/PlayerCamera.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Utils/JupiterInputHelpers.h"
#include "Utils/JupiterTraceUtils.h"

void UCameraMovementSystem::Init(APlayerCamera* InOwner)
{
    Owner = InOwner;
    if (!Owner.IsValid())
    {
        return;
    }

    Player = Owner->GetPlayerController();
    SpringArm = Owner->GetSpringArm();
    Camera = Owner->GetCameraComponent();
    MovementComponent = Owner->GetMovementComponent();

    if (SpringArm)
    {
        TargetRotation = SpringArm->GetRelativeRotation();
        TargetZoom = SpringArm->TargetArmLength;
    }
}

void UCameraMovementSystem::Tick(float DeltaTime)
{
    if (!Owner.IsValid() || !SpringArm)
    {
        return;
    }

    UpdateEdgeScroll(DeltaTime);
    ClampRotation();

    const float InterpolatedZoom = FMath::FInterpTo(SpringArm->TargetArmLength, TargetZoom, DeltaTime, ZoomSpeed);
    SpringArm->TargetArmLength = InterpolatedZoom;

    const FRotator DesiredRotation = FMath::RInterpTo(SpringArm->GetRelativeRotation(), TargetRotation, DeltaTime, RotateSpeed);
    SpringArm->SetRelativeRotation(DesiredRotation);
}

void UCameraMovementSystem::HandleMoveInput(const FInputActionValue& Value)
{
    if (!Owner.IsValid())
    {
        return;
    }

    const FVector2D InputVector = Value.Get<FVector2D>();
    const float DeltaTime = Owner->GetWorld() ? Owner->GetWorld()->GetDeltaSeconds() : 0.f;
    ApplyMovementInput(InputVector, DeltaTime);
}

void UCameraMovementSystem::HandleZoomInput(const FInputActionValue& Value)
{
    const float InputValue = Value.Get<float>();
    if (FMath::IsNearlyZero(InputValue))
    {
        return;
    }

    TargetZoom = FMath::Clamp(TargetZoom + InputValue * 100.f, MinZoom, MaxZoom);
}

void UCameraMovementSystem::HandleRotateHorizontal(const FInputActionValue& Value)
{
    if (!bCanRotate)
    {
        return;
    }

    const float DeltaTime = Owner.IsValid() && Owner->GetWorld() ? Owner->GetWorld()->GetDeltaSeconds() : 0.f;
    const float RotationDelta = Value.Get<float>() * RotateSpeed * DeltaTime;
    TargetRotation = FRotator(TargetRotation.Pitch, TargetRotation.Yaw + RotationDelta, 0.f);
}

void UCameraMovementSystem::HandleRotateVertical(const FInputActionValue& Value)
{
    if (!bCanRotate)
    {
        return;
    }

    const float DeltaTime = Owner.IsValid() && Owner->GetWorld() ? Owner->GetWorld()->GetDeltaSeconds() : 0.f;
    const float RotationDelta = Value.Get<float>() * RotateSpeed * DeltaTime;
    TargetRotation = FRotator(TargetRotation.Pitch + RotationDelta, TargetRotation.Yaw, 0.f);
}

void UCameraMovementSystem::HandleRotateToggle(const FInputActionValue& Value)
{
    bCanRotate = Value.Get<bool>();
}

void UCameraMovementSystem::ApplyMovementInput(const FVector2D& InputVector, float DeltaTime)
{
    if (!Owner.IsValid() || !SpringArm || !Owner->GetWorld())
    {
        return;
    }

    FVector Movement = FVector::ZeroVector;
    Movement += SpringArm->GetForwardVector() * InputVector.Y;
    Movement += SpringArm->GetRightVector() * InputVector.X;
    Movement *= CameraSpeed * DeltaTime;
    Movement.Z = 0.f;

    const FVector DesiredLocation = Owner->GetActorLocation() + Movement;
    Owner->SetActorLocation(GetGroundedLocation(DesiredLocation));
}

void UCameraMovementSystem::UpdateEdgeScroll(float DeltaTime)
{
    if (!bEnableEdgeScroll || !Owner.IsValid() || !Player.IsValid())
    {
        return;
    }

    FVector2D MousePosition;
    if (!JupiterInputHelpers::TryGetNormalizedMousePosition(Player.Get(), MousePosition))
    {
        return;
    }

    FVector2D InputVector = FVector2D::ZeroVector;

    if (MousePosition.X >= 1.f - EdgeScrollThreshold)
    {
        InputVector.X = 1.f;
    }
    else if (MousePosition.X <= EdgeScrollThreshold)
    {
        InputVector.X = -1.f;
    }

    if (MousePosition.Y >= 1.f - EdgeScrollThreshold)
    {
        InputVector.Y = -1.f;
    }
    else if (MousePosition.Y <= EdgeScrollThreshold)
    {
        InputVector.Y = 1.f;
    }

    if (!InputVector.IsNearlyZero())
    {
        ApplyMovementInput(InputVector * EdgeScrollSpeed, DeltaTime);
    }
}

void UCameraMovementSystem::ClampRotation()
{
    TargetRotation.Pitch = FMath::Clamp(TargetRotation.Pitch, -RotatePitchMax, -RotatePitchMin);
    TargetRotation.Roll = 0.f;
}

FVector UCameraMovementSystem::GetGroundedLocation(const FVector& DesiredLocation) const
{
    if (!Owner.IsValid())
    {
        return DesiredLocation;
    }

    FVector ProjectedLocation = DesiredLocation;
    JupiterTraceUtils::ProjectPointToTerrain(Owner->GetWorld(), DesiredLocation, ProjectedLocation);
    return ProjectedLocation;
}

