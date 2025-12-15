#include "Player/JupiterPlayerSystem/CameraMovementSystem.h"
#include "Player/PlayerCamera.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/KismetMathLibrary.h"

#include "GameFramework/SpringArmComponent.h"


void UCameraMovementSystem::Tick(float DeltaTime)
{
    if (!GetOwner())
        return;

    APlayerCamera* Cam = GetOwner();

    // ----------------------------------------------------
    // 0. SMOOTH MOVE TO LOCATION
    // ----------------------------------------------------
    if (bIsMovingToLocation)
    {
        MoveToAlpha += DeltaTime / MoveToDuration;
        MoveToAlpha = FMath::Clamp(MoveToAlpha, 0.f, 1.f);

        // Smooth step interpolation for nicer feel
        const float SmoothAlpha = FMath::SmoothStep(0.f, 1.f, MoveToAlpha);
        const FVector NewLoc = FMath::Lerp(MoveToStartLocation, MoveToTargetLocation, SmoothAlpha);

        Cam->SetActorLocation(NewLoc);

        if (MoveToAlpha >= 1.f)
        {
            bIsMovingToLocation = false;
        }

        // Skip manual movement while animating to target
        return; 
    }

    // ----------------------------------------------------
    // 1. CAMERA MOVEMENT
    // ----------------------------------------------------
    if (!PendingMoveInput.IsZero())
    {
        FVector MoveVector = FVector::ZeroVector;

        MoveVector += Cam->GetSpringArm()->GetForwardVector() * PendingMoveInput.Y;
        MoveVector += Cam->GetSpringArm()->GetRightVector()   * PendingMoveInput.X;

        MoveVector *= Cam->CameraSpeed * DeltaTime;
        MoveVector.Z = 0.f;

        const FVector NextLocation = Cam->GetActorLocation() + MoveVector;
        Cam->SetActorLocation(NextLocation);

        UpdateTerrainFollow();
    }

    // ----------------------------------------------------
    // 2. ZOOM
    // ----------------------------------------------------
    if (PendingZoomInput != 0.f)
    {
        const float ZoomDelta = PendingZoomInput * 100.f;
        Cam->TargetZoom = FMath::Clamp(Cam->TargetZoom + ZoomDelta, Cam->MinZoom, Cam->MaxZoom);
    	
        PendingZoomInput = 0.f;
    }

    // ----------------------------------------------------
    // 3. ROTATION
    // ----------------------------------------------------
    if (bRotateEnabled)
    {
        if (PendingRotateH != 0.f)
        {
            Cam->TargetRotation = UKismetMathLibrary::ComposeRotators(
            	Cam->TargetRotation,
                FRotator(0.f, PendingRotateH * Cam->RotateSpeed * DeltaTime, 0.f)
            );
        }

        if (PendingRotateV != 0.f)
        {
            Cam->TargetRotation = UKismetMathLibrary::ComposeRotators(
            	Cam->TargetRotation,
            	FRotator(PendingRotateV * Cam->RotateSpeed * DeltaTime, 0.f, 0.f)
            );
        }
    }

    PendingRotateH = PendingRotateV = 0.f;
	
    if (Cam->CanEdgeScroll)
        ProcessEdgeScroll();
	
    ClampCameraPitch();
}


// ----------------------------------------------------
// Input Handlers
// ----------------------------------------------------

void UCameraMovementSystem::HandleMoveInput(const FInputActionValue& Value)
{
    PendingMoveInput = Value.Get<FVector2D>();
    // Cancel smooth move if user inputs manual movement
    if (!PendingMoveInput.IsZero())
    {
        bIsMovingToLocation = false;
    }
}

void UCameraMovementSystem::HandleMoveReleased(const FInputActionValue& Value)
{
	PendingMoveInput = FVector2D::ZeroVector;
}

void UCameraMovementSystem::HandleZoomInput(const FInputActionValue& Value)
{
    PendingZoomInput = Value.Get<float>();
}

void UCameraMovementSystem::HandleRotateHorizontal(const FInputActionValue& Value)
{
    PendingRotateH = Value.Get<float>();
}

void UCameraMovementSystem::HandleRotateVertical(const FInputActionValue& Value)
{
    PendingRotateV = Value.Get<float>();
}

void UCameraMovementSystem::HandleRotateToggle(const FInputActionValue& Value)
{
    bRotateEnabled = Value.Get<bool>();
}

// ----------------------------------------------------
// Helpers
// ----------------------------------------------------

void UCameraMovementSystem::ProcessEdgeScroll()
{
    APlayerCamera* Cam = GetOwner();
    if (!Cam || !GetWorldSafe())
        return;

    FVector2D MousePos = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorldSafe());
    const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(GetWorldSafe());

    MousePos *= UWidgetLayoutLibrary::GetViewportScale(GetWorldSafe());

    const float XNorm = MousePos.X / ViewportSize.X;
    const float YNorm = MousePos.Y / ViewportSize.Y;

    FVector2D EdgeInput = FVector2D::ZeroVector;

	// X NORM
    if (XNorm > 0.98f)
    {
	    EdgeInput.X = 1.f;
    }
    else if (XNorm < 0.02f)
    {
	    EdgeInput.X = -1.f;
    }

	// Y NORM
    if (YNorm > 0.98f)
    {
	    EdgeInput.Y = -1.f;
    }
    else if (YNorm < 0.02f)
    {
	    EdgeInput.Y = 1.f;
    }

    if (!EdgeInput.IsZero())
        PendingMoveInput += EdgeInput * Cam->EdgeScrollSpeed;
}

void UCameraMovementSystem::UpdateTerrainFollow()
{
    if (!GetWorldSafe() || !GetOwner())
        return;

    APlayerCamera* Cam = GetOwner();
    FVector Pos = Cam->GetActorLocation();

    FHitResult Hit;
    FVector Start = Pos + FVector(0,0,10000);
    FVector End   = Pos - FVector(0,0,10000);

    if (GetWorldSafe()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
    {
        FVector NewPos = Hit.ImpactPoint;
        NewPos.Z += 0;
    	
        Cam->SetActorLocation(FVector(Pos.X, Pos.Y, NewPos.Z));
    }
}

void UCameraMovementSystem::MoveToLocation(const FVector& TargetLocation, float Duration)
{
    if (!GetOwner()) return;

    MoveToStartLocation = GetOwner()->GetActorLocation();
    
    // Maintain Z height to avoid diving into ground, unless specific Z needed
    // Usually camera stays at a fixed height logic, but let's assume TargetLocation is on ground
    // and we want to keep current camera height relative to something?
    // User asked to move *camera* to this patrol. 
    // Usually that means centering the camera X/Y on the target, keeping Z or terrain adjust.
    // Let's assume TargetLocation passed in is the ground point to look at.
    // but the camera is effectively at Z height.
    // If we just lerp to TargetLocation (z=0), camera goes to ground.
    
    // Adjust target to be at current Camera Z? Or rely on TerrainFollow?
    // If TerrainFollow is active, setting X/Y is enough and Z will snap in next Tick?
    // In Tick, UpdateTerrainFollow is called after manual movement.
    
    MoveToTargetLocation = FVector(TargetLocation.X, TargetLocation.Y, MoveToStartLocation.Z);

    MoveToDuration = FMath::Max(0.1f, Duration);
    MoveToAlpha = 0.f;
    bIsMovingToLocation = true;
}

void UCameraMovementSystem::ClampCameraPitch()
{
    if (!GetOwner())
        return;

    APlayerCamera* Cam = GetOwner();

    float NewPitch = Cam->TargetRotation.Pitch;
	
    if (NewPitch < (Cam->RotatePitchMax * -1.f))
    {
	    NewPitch = (Cam->RotatePitchMax * -1.f);
    }
    else if (NewPitch > (Cam->RotatePitchMin * -1.f))
    {
	    NewPitch = (Cam->RotatePitchMin * -1.f);
    }

    Cam->TargetRotation = FRotator(NewPitch, Cam->TargetRotation.Yaw, 0.f);
}
