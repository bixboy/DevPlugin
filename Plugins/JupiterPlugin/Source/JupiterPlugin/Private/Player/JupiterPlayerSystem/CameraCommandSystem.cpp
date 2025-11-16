#include "Player/JupiterPlayerSystem/CameraCommandSystem.h"
#include "Player/PlayerCamera.h"
#include "Components/UnitOrderComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "Interfaces/Selectable.h"
#include "Player/Selections/SphereRadius.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"


// ------------------------------------------------------------
// INIT
// ------------------------------------------------------------
void UCameraCommandSystem::Init(APlayerCamera* InOwner)
{
    Super::Init(InOwner);

    if (!GetWorldSafe() || !SphereRadiusClass)
        return;

    // Spawn patrol sphere
    FActorSpawnParameters Params;
    Params.Owner = GetOwner();
    Params.Instigator = GetOwner();

    SphereRadius = GetWorldSafe()->SpawnActor<ASphereRadius>(SphereRadiusClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);
    if (SphereRadius)
    {
        SphereRadius->SetOwner(GetOwner());
        SphereRadius->SetActorHiddenInGame(true);
    }
}

// ------------------------------------------------------------
// TICK : rotation preview + patrol radius
// ------------------------------------------------------------
void UCameraCommandSystem::Tick(float DeltaTime)
{
    // -------------------------
    // Rotation preview
    // -------------------------
    if (bIsCommandHeld)
    {
        FHitResult Hit;
        if (GetMouseHitOnTerrain(Hit))
        {
            const FVector MouseLocation = Hit.Location;
            const float CurrentTime = GetWorldSafe()->GetTimeSeconds();

            if (!bRotationPreviewActive)
            {
                if (TryActivateRotationPreview(CurrentTime, RotationHoldThreshold, RotationCenter, MouseLocation))
                {
                    BeginRotationPreview(CurrentTime, RotationCenter, BaseRotation);
                }
                return;
            }

            UpdateRotationPreview(MouseLocation);
        }
    }

    // -------------------------
    // Patrol radius update
    // -------------------------
    if (bPatrolActive && bAltPressed)
    {
        FHitResult Hit;
        if (GetMouseHitOnTerrain(Hit))
        {
            UpdatePatrolRadius(Hit.Location);
        }
    }
}

// ------------------------------------------------------------
// COMMAND INPUT (right-click)
// ------------------------------------------------------------

void UCameraCommandSystem::HandleCommandActionStarted()
{
    if (!GetSelectionComponent())
        return;

    bIsCommandHeld = true;
    bRotationPreviewActive = false;
    bRotationHoldTriggered = false;

    FHitResult Hit;
    if (GetMouseHitOnTerrain(Hit))
    {
        CommandStartLocation = Hit.Location;
        RotationCenter = Hit.Location;
    }

    if (GetOwner()->GetCameraComponent())
    {
        BaseRotation = FRotator(0.f, GetOwner()->GetCameraComponent()->GetComponentRotation().Yaw, 0.f);
    }
    CurrentRotation = BaseRotation;

    RotationHoldStartTime = GetWorldSafe()->GetTimeSeconds();
}

void UCameraCommandSystem::HandleCommandActionCompleted()
{
    if (!bIsCommandHeld)
        return;

    bIsCommandHeld = false;

    StopRotationPreview();

    // PATROL MODE TAKES PRIORITY
    if (bPatrolActive)
    {
        float Radius = SphereRadius ? SphereRadius->GetRadius() : 500.f;
        IssuePatrolCommand(Radius, RotationCenter);

        StopPatrolRadius();
        return;
    }

    // ATTACK TARGET
    if (AActor* Target = GetHoveredActor())
    {
        if (Target->Implements<USelectable>())
        {
            IssueAttackCommand(Target);
            return;
        }
    }

    // MOVE COMMAND
    FHitResult Hit;
    if (GetMouseHitOnTerrain(Hit))
    {
        IssueFinalCommand(Hit);
    }
}

// ------------------------------------------------------------
// COMMAND ISSUING
// ------------------------------------------------------------

void UCameraCommandSystem::IssueFinalCommand(const FHitResult& HitResult)
{
    const FVector TargetLocation = HitResult.Location;
    const FRotator Facing = bRotationPreviewActive ? CurrentRotation : BaseRotation;
    IssueMoveCommand(TargetLocation, Facing);
}

void UCameraCommandSystem::IssueAttackCommand(AActor* Target)
{
    if (!GetOrderComponent())
        return;

    FCommandData Data(
        GetOwner()->GetPlayerController(),
        CommandStartLocation,
        BaseRotation,
        ECommandType::CommandAttack,
        Target);

    GetOrderComponent()->IssueOrder(Data);
}

void UCameraCommandSystem::IssueMoveCommand(const FVector& TargetLocation, const FRotator& Facing)
{
    if (!GetOrderComponent())
        return;

    FCommandData Data(
        GetOwner()->GetPlayerController(),
        TargetLocation,
        Facing,
        ECommandType::CommandMove);

    GetOrderComponent()->IssueOrder(Data);
}

void UCameraCommandSystem::IssuePatrolCommand(float Radius, const FVector& Center)
{
    if (!GetOrderComponent())
        return;

    FCommandData Data(
        GetOwner()->GetPlayerController(),
        Center,
        BaseRotation,
        ECommandType::CommandPatrol);

    Data.Radius = Radius;

    GetOrderComponent()->IssueOrder(Data);
}

// ------------------------------------------------------------
// PATROL MODE
// ------------------------------------------------------------

void UCameraCommandSystem::HandleAltPressed()
{
    bAltPressed = true;
}

void UCameraCommandSystem::HandleAltReleased()
{
    bAltPressed = false;

    if (bPatrolActive)
    {
        StopPatrolRadius();
    }
}

void UCameraCommandSystem::HandleAltHold(const FInputActionValue& Value)
{
    if (!bAltPressed)
        return;

    const float Held = Value.Get<float>();
    if (Held == 0.f)
        return;

    float TimeHeld = GetWorldSafe()->GetTimeSeconds() - RotationHoldStartTime;
    if (TimeHeld >= PatrolHoldThreshold && !bPatrolActive)
    {
        StartPatrolRadius(RotationCenter);
    }
}

void UCameraCommandSystem::StartPatrolRadius(const FVector& Center)
{
    bPatrolActive = true;

    if (SphereRadius)
    {
        SphereRadius->Start(Center, FRotator::ZeroRotator);
    }
}

void UCameraCommandSystem::StopPatrolRadius()
{
    bPatrolActive = false;

    if (SphereRadius)
    {
        SphereRadius->End();
    }
}

void UCameraCommandSystem::UpdatePatrolRadius(const FVector& MouseLocation)
{
    if (!SphereRadius)
        return;
}

// ------------------------------------------------------------
// ROTATION PREVIEW
// ------------------------------------------------------------

bool UCameraCommandSystem::TryActivateRotationPreview(float CurrentTime, float Threshold, const FVector& InitialLocation, const FVector& MouseLocation)
{
    const float Dist = FVector::Dist2D(InitialLocation, MouseLocation);
    if (Dist < 10.f)
        return false;

    const float HeldTime = CurrentTime - RotationHoldStartTime;
    return HeldTime > Threshold;
}

void UCameraCommandSystem::BeginRotationPreview(float CurrentTime, const FVector& Center, const FRotator& BaseYaw)
{
    bRotationPreviewActive = true;
    bRotationHoldTriggered = true;

    RotationCenter = Center;
    BaseRotation = BaseYaw;
    CurrentRotation = BaseYaw;

    InitialDirection = FVector::VectorPlaneProject(FVector::ForwardVector, FVector::UpVector);
}

void UCameraCommandSystem::UpdateRotationPreview(const FVector& MouseLocation)
{
    FVector Dir = MouseLocation - RotationCenter;
    Dir.Z = 0.f;

    if (Dir.IsNearlyZero())
        return;

    Dir.Normalize();

    const float Angle = FMath::Acos(FVector::DotProduct(InitialDirection, Dir));
    const float CrossZ = FVector::CrossProduct(InitialDirection, Dir).Z;
    const float YawOffset = FMath::RadiansToDegrees(Angle) * FMath::Sign(CrossZ);

    CurrentRotation = BaseRotation + FRotator(0.f, YawOffset, 0.f);
}

void UCameraCommandSystem::StopRotationPreview()
{
    bRotationPreviewActive = false;
    bRotationHoldTriggered = false;
}

// ------------------------------------------------------------
// Destroy Selected
// ------------------------------------------------------------

void UCameraCommandSystem::HandleDestroySelected()
{
    if (!GetSelectionComponent())
        return;

    HandleServerDestroyActor(GetSelectionComponent()->GetSelectedActors());
}

void UCameraCommandSystem::HandleServerDestroyActor(const TArray<AActor*>& ActorsToDestroy)
{
    for (AActor* A : ActorsToDestroy)
    {
        if (A)
        	A->Destroy();
    }
}

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

bool UCameraCommandSystem::GetMouseHitOnTerrain(FHitResult& OutHit) const
{
    if (!GetSelectionComponent())
        return false;

    OutHit = GetSelectionComponent()->GetMousePositionOnTerrain();
    return OutHit.bBlockingHit;
}

AActor* UCameraCommandSystem::GetHoveredActor() const
{
    if (!GetOwner() || !GetWorldSafe())
        return nullptr;

    APlayerController* PC = GetOwner()->GetPlayerController();
    if (!PC)
        return nullptr;

    FVector WL, WD;
    PC->DeprojectMousePositionToWorld(WL, WD);

    const FVector End = WL + WD * 1000000.f;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());

    if (GetWorldSafe()->LineTraceSingleByChannel(Hit, WL, End, ECC_Visibility, Params))
        return Hit.GetActor();

    return nullptr;
}
