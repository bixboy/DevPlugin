#include "Player/JupiterPlayerSystem/CameraMovementSystem.h"
#include "Player/PlayerCamera.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/SpringArmComponent.h"


void UCameraMovementSystem::Init(APlayerCamera* InOwner)
{
    Super::Init(InOwner);
    
    if (GetOwner())
    {
        CurrentTerrainHeight = GetOwner()->GetActorLocation().Z;
    }
}

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

        const float SmoothAlpha = FMath::SmoothStep(0.f, 1.f, MoveToAlpha);
        FVector NewLoc = FMath::Lerp(MoveToStartLocation, MoveToTargetLocation, SmoothAlpha);

        CurrentTerrainHeight = NewLoc.Z; 
        Cam->SetActorLocation(NewLoc);

        if (MoveToAlpha >= 1.f)
        {
            bIsMovingToLocation = false;
        }
    	
        return;
    }

    // ----------------------------------------------------
    // 1. INPUT GATHERING
    // ----------------------------------------------------
    FVector2D TotalMoveInput = PendingMoveInput;
    
    if (Cam->CanEdgeScroll)
    {
        FVector2D EdgeInput = GetEdgeScrollInput();
        if (!EdgeInput.IsZero())
        {
            TotalMoveInput += EdgeInput * Cam->EdgeScrollSpeed;
        }
    }

    if (TotalMoveInput.SizeSquared() > 1.0f)
    {
        TotalMoveInput.Normalize();
    }

    // ----------------------------------------------------
    // 2. MOVEMENT APPLICATION
    // ----------------------------------------------------
    if (!TotalMoveInput.IsZero())
    {
        FRotator CamRot = Cam->GetSpringArm()->GetTargetRotation();
        CamRot.Pitch = 0.f;
        CamRot.Roll = 0.f;

        const FVector Forward = FRotationMatrix(CamRot).GetUnitAxis(EAxis::X);
        const FVector Right   = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);

        FVector MoveDir = (Forward * TotalMoveInput.Y) + (Right * TotalMoveInput.X);
        
        // Vitesse ajustée par la hauteur (Plus on est haut, plus on bouge vite ?)
        // Optionnel : float HeightSpeedMod = FMath::GetMappedRangeValueClamped(..., Cam->TargetZoom, ...);
        
        FVector DesiredOffset = MoveDir * Cam->CameraSpeed * DeltaTime * 100.f; 
        Cam->AddActorWorldOffset(DesiredOffset);
    }

    // ----------------------------------------------------
    // 3. TERRAIN FOLLOW
    // ----------------------------------------------------
    UpdateTerrainFollow(DeltaTime);

    // ----------------------------------------------------
    // 4. ZOOM
    // ----------------------------------------------------
    if (FMath::Abs(PendingZoomInput) > 0.01f)
    {
        float ZoomSpeedFactor = Cam->TargetZoom * 0.15f; 
        float ZoomDelta = PendingZoomInput * ZoomSpeedFactor;
        
        Cam->TargetZoom = FMath::Clamp(Cam->TargetZoom + ZoomDelta, Cam->MinZoom, Cam->MaxZoom);
        
        PendingZoomInput = 0.f;
    }

    // ----------------------------------------------------
    // 5. ROTATION
    // ----------------------------------------------------
    if (bRotateEnabled)
    {
        float YawDelta = PendingRotateH * Cam->RotateSpeed * DeltaTime * 50.f;
        float PitchDelta = PendingRotateV * Cam->RotateSpeed * DeltaTime * 50.f;

        if (!FMath::IsNearlyZero(YawDelta))
        	Cam->TargetRotation.Yaw += YawDelta;
    	
        if (!FMath::IsNearlyZero(PitchDelta))
        	Cam->TargetRotation.Pitch += PitchDelta;

        ClampCameraPitch();
    }

    PendingRotateH = 0.f;
    PendingRotateV = 0.f;
}


// ----------------------------------------------------
// Helpers
// ----------------------------------------------------

FVector2D UCameraMovementSystem::GetEdgeScrollInput() const
{
    if (!GetWorldSafe()) return FVector2D::ZeroVector;

    FVector2D MousePos = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorldSafe());
    const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(GetWorldSafe());

    float DPIScale = UWidgetLayoutLibrary::GetViewportScale(GetWorldSafe());
    MousePos *= DPIScale;

    if (ViewportSize.X <= 0 || ViewportSize.Y <= 0)
    	return FVector2D::ZeroVector;

    const float XNorm = MousePos.X / ViewportSize.X;
    const float YNorm = MousePos.Y / ViewportSize.Y;

    FVector2D Edge = FVector2D::ZeroVector;

    const float Margin = 0.02f;

    if (XNorm > (1.0f - Margin))
    {
	    Edge.X = 1.f;
    }
    else if (XNorm < Margin)
    {
	    Edge.X = -1.f;
    }

    if (YNorm > (1.0f - Margin))
    {
	    Edge.Y = -1.f;
    }
    else if (YNorm < Margin)
    {
	    Edge.Y = 1.f;
    }

    return Edge;
}

void UCameraMovementSystem::UpdateTerrainFollow(float DeltaTime)
{
    if (!GetWorldSafe() || !GetOwner())
    	return;

    APlayerCamera* Cam = GetOwner();
    FVector CurrentPos = Cam->GetActorLocation();

    FVector TraceStart = FVector(CurrentPos.X, CurrentPos.Y, CurrentPos.Z + 10000.0f);
    FVector TraceEnd   = FVector(CurrentPos.X, CurrentPos.Y, -10000.0f);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Cam);

    bool bHit = GetWorldSafe()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params);

    if (bHit)
    {
        float TargetZ = Hit.ImpactPoint.Z;

        CurrentTerrainHeight = FMath::FInterpTo(CurrentTerrainHeight, TargetZ, DeltaTime, 5.0f);
        
        Cam->SetActorLocation(FVector(CurrentPos.X, CurrentPos.Y, CurrentTerrainHeight));
    }
}

void UCameraMovementSystem::MoveToLocation(const FVector& TargetLocation, float Duration)
{
    if (!GetOwner())
    	return;

    MoveToStartLocation = GetOwner()->GetActorLocation();
	
    MoveToTargetLocation = TargetLocation;
    
    MoveToDuration = FMath::Max(0.1f, Duration);
    MoveToAlpha = 0.f;
    bIsMovingToLocation = true;
}

void UCameraMovementSystem::ClampCameraPitch()
{
    if (!GetOwner())
    	return;

    APlayerCamera* Cam = GetOwner();
	
    float CurrentPitch = Cam->TargetRotation.Pitch;
    
    float MinP = -Cam->RotatePitchMax;
    float MaxP = -Cam->RotatePitchMin;

    Cam->TargetRotation.Pitch = FMath::Clamp(CurrentPitch, MinP, MaxP);
}

// ----------------------------------------------------
// Input Handlers
// ----------------------------------------------------

void UCameraMovementSystem::HandleMoveInput(const FInputActionValue& Value)
{
    PendingMoveInput = Value.Get<FVector2D>();
    
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