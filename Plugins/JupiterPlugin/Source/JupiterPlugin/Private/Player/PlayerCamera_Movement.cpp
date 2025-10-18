#include "JupiterPlugin/Public/Player/PlayerCamera.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"

void APlayerCamera::Input_OnMove(const FInputActionValue& ActionValue)
{
        const FVector2d InputVector = ActionValue.Get<FVector2D>();

        FVector MovementVector = FVector::ZeroVector;
        MovementVector += SpringArm->GetForwardVector() * InputVector.Y;
        MovementVector += SpringArm->GetRightVector() * InputVector.X;
        MovementVector *= CameraSpeed * GetWorld()->GetDeltaSeconds();
        MovementVector.Z = 0.f;

        FVector NextPosition = GetActorLocation() + MovementVector;
        SetActorLocation(NextPosition);
        GetTerrainPosition(NextPosition);
}

void APlayerCamera::Input_Zoom(const FInputActionValue& ActionValue)
{
        const float Value = ActionValue.Get<float>();

        if (Value == 0.f)
        {
                return;
        }

        const float Zoom = Value * 100.f;
        TargetZoom = FMath::Clamp(Zoom + TargetZoom, MinZoom, MaxZoom);
}

void APlayerCamera::Input_RotateHorizontal(const FInputActionValue& ActionValue)
{
        const float Value = ActionValue.Get<float>() * RotateSpeed * GetWorld()->GetDeltaSeconds();

        if (Value == 0.f || !CanRotate)
        {
                return;
        }

        TargetRotation = UKismetMathLibrary::ComposeRotators(TargetRotation, FRotator(0.f, Value, 0.f));
}

void APlayerCamera::Input_RotateVertical(const FInputActionValue& ActionValue)
{
        const float Value = ActionValue.Get<float>() * RotateSpeed * GetWorld()->GetDeltaSeconds();

        if (Value == 0.f || !CanRotate)
        {
                return;
        }

        TargetRotation = UKismetMathLibrary::ComposeRotators(TargetRotation, FRotator(Value, 0.f, 0.f));
}

void APlayerCamera::Input_EnableRotate(const FInputActionValue& ActionValue)
{
        const bool Value = ActionValue.Get<bool>();
        CanRotate = Value;
}

void APlayerCamera::GetTerrainPosition(FVector& TerrainPosition) const
{
        FHitResult Hit;
        FCollisionQueryParams CollisionParams;

        FVector StartLocation = TerrainPosition;
        StartLocation.Z += 10000.f;
        FVector EndLocation = TerrainPosition;
        EndLocation.Z -= 10000.f;

        if (UWorld* WorldContext = GetWorld())
        {
                if (WorldContext->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, ECC_Visibility, CollisionParams))
                {
                        TerrainPosition = Hit.ImpactPoint;
                }
        }
}

void APlayerCamera::EdgeScroll()
{
        if (!CanEdgeScroll)
        {
                return;
        }

        if (UWorld* WorldContext = GetWorld())
        {
                FVector2D MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(WorldContext);
                const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(WorldContext);
                MousePosition = MousePosition * UWidgetLayoutLibrary::GetViewportScale(WorldContext);

                MousePosition.X = MousePosition.X / ViewportSize.X;
                MousePosition.Y = MousePosition.Y / ViewportSize.Y;

                // Right/Left
                if (MousePosition.X > 0.98f && MousePosition.X > 1.f)
                {
                        Input_OnMove(EdgeScrollSpeed);
                }
                else if (MousePosition.X < 0.02f && MousePosition.X < 0.f)
                {
                        Input_OnMove(-EdgeScrollSpeed);
                }

                // Forward/Backward
                if (MousePosition.Y > 0.98f && MousePosition.Y > 1.f)
                {
                        Input_OnMove(-EdgeScrollSpeed);
                }
                else if (MousePosition.Y < 0.02f && MousePosition.Y < 0.f)
                {
                        Input_OnMove(EdgeScrollSpeed);
                }
        }
}

void APlayerCamera::CameraBounds()
{
        float NewPitch = TargetRotation.Pitch;
        if (TargetRotation.Pitch < (RotatePitchMax * -1.f))
        {
                NewPitch = (RotatePitchMax * -1.f);
        }
        else if (TargetRotation.Pitch > (RotatePitchMin * -1.f))
        {
                NewPitch = (RotatePitchMin * -1.f);
        }

        TargetRotation = FRotator(NewPitch, TargetRotation.Yaw, 0.f);
}

