#include "Player/CameraPatrolSystem.h"

#include "Components/UnitPatrolComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "GameFramework/PlayerController.h"
#include "Player/CameraCommandSystem.h"
#include "Player/CameraSelectionSystem.h"
#include "Player/PlayerCamera.h"
#include "Engine/World.h"
#include "Player/Selections/SphereRadius.h"
#include "Utils/JupiterInputHelpers.h"

void UCameraPatrolSystem::Init(APlayerCamera* InOwner)
{
    Owner = InOwner;
    if (!Owner.IsValid())
    {
        return;
    }

    Player = Owner->GetPlayerController();
    PatrolComponent = Owner->GetPatrolComponent();

    EnsureSphereRadius();
}

void UCameraPatrolSystem::Tick(float DeltaTime)
{
    if (!Player.IsValid())
    {
        return;
    }

    bAltPressed = Player->IsInputKeyDown(EKeys::LeftAlt) || Player->IsInputKeyDown(EKeys::RightAlt);
}

void UCameraPatrolSystem::HandleAltPressed()
{
    bAltPressed = true;
    EnsureSphereRadius();
}

void UCameraPatrolSystem::HandleAltReleased()
{
    if (bSphereRadiusEnabled && CommandSystem.IsValid() && SphereRadius)
    {
        CommandSystem->IssuePatrolCommand(SphereRadius->GetRadius());
    }

    StopSphereRadius();
    bAltPressed = false;
}

void UCameraPatrolSystem::HandleAltHold(const FInputActionValue& Value)
{
    if (!bAltPressed || !Player.IsValid())
    {
        StopSphereRadius();
        return;
    }

    const float InputValue = Value.Get<float>();
    if (FMath::IsNearlyZero(InputValue))
    {
        StopSphereRadius();
        return;
    }

    const float HoldTime = JupiterInputHelpers::GetKeyTimeDown(Player.Get(), EKeys::RightMouseButton);
    if (HoldTime >= LeftMouseHoldThreshold)
    {
        if (SelectionSystem.IsValid())
        {
            StartSphereRadiusAtLocation(SelectionSystem->GetLastGroundLocation());
        }
    }
}

void UCameraPatrolSystem::HandlePatrolRadius(const FInputActionValue& Value)
{
    if (!bAltPressed)
    {
        StopSphereRadius();
        return;
    }

    const float InputValue = Value.Get<float>();
    if (FMath::IsNearlyZero(InputValue))
    {
        StopSphereRadius();
        return;
    }

    if (SelectionSystem.IsValid())
    {
        StartSphereRadiusAtLocation(SelectionSystem->GetLastGroundLocation());
    }
}

bool UCameraPatrolSystem::HandleLeftClick(EInputEvent InputEvent)
{
    if (!PatrolComponent)
    {
        return false;
    }

    return PatrolComponent->HandleLeftClick(InputEvent);
}

bool UCameraPatrolSystem::HandleRightClickPressed(bool bAltDown)
{
    if (!PatrolComponent)
    {
        return false;
    }

    return PatrolComponent->HandleRightClickPressed(bAltDown);
}

bool UCameraPatrolSystem::HandleRightClickReleased(bool bAltDown)
{
    if (!PatrolComponent)
    {
        return false;
    }

    return PatrolComponent->HandleRightClickReleased(bAltDown);
}

void UCameraPatrolSystem::EnsureSphereRadius()
{
    if (SphereRadius || !SphereRadiusClass || !Owner.IsValid())
    {
        return;
    }

    if (UWorld* World = Owner->GetWorld())
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = Owner.Get();
        SpawnParams.Instigator = Owner.Get();
        SphereRadius = World->SpawnActor<ASphereRadius>(SphereRadiusClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
        if (SphereRadius)
        {
            SphereRadius->SetOwner(Owner.Get());
        }
    }
}

void UCameraPatrolSystem::StartSphereRadiusAtLocation(const FVector& Location)
{
    FVector StartLocation = Location;
    if (StartLocation.IsNearlyZero() && Owner.IsValid())
    {
        if (UUnitSelectionComponent* Selection = Owner->GetSelectionComponent())
        {
            StartLocation = Selection->GetMousePositionOnTerrain().Location;
        }
    }

    if (!SphereRadius || StartLocation.IsNearlyZero())
    {
        return;
    }

    const FRotator FacingRotation = Owner.IsValid() && Owner->GetSpringArm() ? Owner->GetSpringArm()->GetComponentRotation() : FRotator::ZeroRotator;
    SphereRadius->Start(StartLocation, FacingRotation);
    bSphereRadiusEnabled = true;
}

void UCameraPatrolSystem::StopSphereRadius(bool bResetState)
{
    if (SphereRadius)
    {
        SphereRadius->End();
    }

    if (bResetState)
    {
        bSphereRadiusEnabled = false;
    }
}

