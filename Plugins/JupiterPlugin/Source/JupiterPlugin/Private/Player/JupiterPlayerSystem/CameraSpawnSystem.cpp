#include "Player/JupiterPlayerSystem/CameraSpawnSystem.h"

#include "Camera/CameraComponent.h"
#include "Player/PlayerCamera.h"

#include "Components/UnitSpawnComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "Units/SoldierRts.h"

#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Player/JupiterPlayerSystem/CameraPreviewSystem.h"


// ------------------------------------------------------------
// INIT
// ------------------------------------------------------------
void UCameraSpawnSystem::Init(APlayerCamera* InOwner)
{
    Super::Init(InOwner);

    if (!GetOwner() || !GetOwner()->GetSpawnComponent())
        return;

    SpawnRotationPreview.Reset(FRotator::ZeroRotator);

    if (UUnitSpawnComponent* Spawn = GetSpawnComponent())
    {
        ShowUnitPreview(Spawn->GetUnitToSpawn());
    }
}

// ------------------------------------------------------------
// TICK
// ------------------------------------------------------------
void UCameraSpawnSystem::Tick(float DeltaTime)
{
    if (!bIsInSpawnMode || !PreviewSystem || !PreviewSystem->IsValidLowLevel())
        return;

    const float CurrentTime =
        (GetWorldSafe() ? GetWorldSafe()->GetTimeSeconds() : 0.f);

    UpdatePreviewFollowMouse(CurrentTime);
}

// ------------------------------------------------------------
// On spawn key pressed
// ------------------------------------------------------------
void UCameraSpawnSystem::HandleSpawnInput()
{
    if (!bIsInSpawnMode || !PreviewSystem || !GetSpawnComponent())
        return;

    if (!PreviewSystem->IsPlacementValid())
        return;

    const FVector SpawnLocation = SpawnRotationPreview.Center;
    const FRotator SpawnRot = 
        SpawnRotationPreview.bPreviewActive ?
            SpawnRotationPreview.CurrentRotation :
            FRotator::ZeroRotator;

    GetSpawnComponent()->SpawnUnitsWithTransform(SpawnLocation, SpawnRot);

    // Reset
    SpawnRotationPreview.Deactivate();
    bIsInSpawnMode = false;
	
    if (PreviewSystem)
        PreviewSystem->HidePreview();
}

// ------------------------------------------------------------
// Reset forced by selection system / UI
// ------------------------------------------------------------
void UCameraSpawnSystem::ResetSpawnState()
{
    bIsInSpawnMode = false;
    SpawnRotationPreview.Deactivate();

    if (PreviewSystem)
        PreviewSystem->HidePreview();
}

// ------------------------------------------------------------
// Called when unit class changes
// ------------------------------------------------------------
void UCameraSpawnSystem::ShowUnitPreview(TSubclassOf<ASoldierRts> NewUnitClass)
{
    if (!PreviewSystem)
        return;

    if (!EnsurePreviewActor())
    {
        ResetSpawnState();
        return;
    }

    if (!NewUnitClass)
    {
        ResetSpawnState();
        return;
    }

    ASoldierRts* DefaultUnit = NewUnitClass->GetDefaultObject<ASoldierRts>();
    if (!DefaultUnit)
    {
        ResetSpawnState();
        return;
    }

    const int32 InstanceCount = GetEffectiveSpawnCount();

    const float Yaw = GetOwner()->GetCameraComponent()->GetComponentRotation().Yaw;
    const FRotator BaseRotation(0, Yaw, 0);
    SpawnRotationPreview.Reset(BaseRotation);

    // ------------------------------------------------------------
    // Skeletal Mesh preview
    // ------------------------------------------------------------
    if (USkeletalMeshComponent* SkelComp = DefaultUnit->GetMesh())
    {
        if (USkeletalMesh* Skel = SkelComp->GetSkeletalMeshAsset())
        {
            if (PreviewSystem->ShowSkeletalPreview(Skel, SkelComp->GetRelativeScale3D(), InstanceCount))
            {
                bIsInSpawnMode = true;
                RefreshPreviewInstances();
                return;
            }
        }
    }

    // ------------------------------------------------------------
    // Static Mesh preview
    // ------------------------------------------------------------
    if (UStaticMeshComponent* StaticComp = DefaultUnit->FindComponentByClass<UStaticMeshComponent>())
    {
        if (UStaticMesh* Mesh = StaticComp->GetStaticMesh())
        {
            if (PreviewSystem->ShowStaticPreview(Mesh, StaticComp->GetRelativeScale3D(), InstanceCount))
            {
                bIsInSpawnMode = true;
                RefreshPreviewInstances();
                return;
            }
        }
    }

    ResetSpawnState();
}

// ------------------------------------------------------------
// Ensure preview actor exists
// ------------------------------------------------------------
bool UCameraSpawnSystem::EnsurePreviewActor()
{
    if (!PreviewSystem)
        return false;

    return PreviewSystem->EnsurePreviewActor();
}

// ------------------------------------------------------------
// Effective spawn count according to formation
// ------------------------------------------------------------
int32 UCameraSpawnSystem::GetEffectiveSpawnCount() const
{
    UUnitSpawnComponent* Spawn = GetSpawnComponent();
    if (!Spawn)
        return 1;

    if (Spawn->GetSpawnFormation() == ESpawnFormation::Custom)
    {
        const FIntPoint Dim = Spawn->GetCustomFormationDimensions();
        return FMath::Max(1, Dim.X * Dim.Y);
    }

    return FMath::Max(1, Spawn->GetUnitsPerSpawn());
}

// ------------------------------------------------------------
// Follow mouse + hold-to-rotate
// ------------------------------------------------------------
void UCameraSpawnSystem::UpdatePreviewFollowMouse(float CurrentTime)
{
    if (!PreviewSystem || !PreviewSystem->HasPreviewActor())
        return;

    UUnitSelectionComponent* Selection = GetSelectionComponent();
    if (!Selection)
        return;

    const FHitResult MouseHit = Selection->GetMousePositionOnTerrain();
    const FVector MousePos = MouseHit.Location;

    if (MousePos.IsNearlyZero())
        return;

    APlayerController* PC = GetOwner()->GetPlayerController();
    if (!PC)
        return;

    // Input states
    const bool bLDown = PC->IsInputKeyDown(EKeys::LeftMouseButton);
    const bool bRDown = PC->IsInputKeyDown(EKeys::RightMouseButton);
    const bool bDown = (bLDown || bRDown);

    const bool bLJust = PC->WasInputKeyJustPressed(EKeys::LeftMouseButton);
    const bool bRJust = PC->WasInputKeyJustPressed(EKeys::RightMouseButton);
    const bool bJust = (bLJust || bRJust);

    const float Yaw = GetOwner()->GetCameraComponent()->GetComponentRotation().Yaw;
    const FRotator MouseRotation(0, Yaw, 0);

    // Start hold
    if (bJust)
    {
        SpawnRotationPreview.BeginHold(CurrentTime, MousePos, MouseRotation);
    }
    else if (!bDown)
    {
        SpawnRotationPreview.StopHold();
    }

    if (!SpawnRotationPreview.bPreviewActive)
    {
        SpawnRotationPreview.Center = MousePos;
        SpawnRotationPreview.BaseRotation = MouseRotation;
        SpawnRotationPreview.CurrentRotation = MouseRotation;
    }

    if (SpawnRotationPreview.bHoldActive &&
        !SpawnRotationPreview.bPreviewActive)
    {
        if (SpawnRotationPreview.TryActivate(CurrentTime, RotationPreviewHoldTime, MousePos, MousePos))
        {
            SpawnRotationPreview.InitialDirection = FRotationPreviewState::ResolvePlanarDirection(
            	MousePos - SpawnRotationPreview.Center, SpawnRotationPreview.BaseRotation);
        }
    }

    if (SpawnRotationPreview.bPreviewActive)
    {
        SpawnRotationPreview.UpdateRotation(MousePos);
    }

    SpawnRotationPreview.Center = MousePos;
    UpdatePreviewTransforms(SpawnRotationPreview.Center, SpawnRotationPreview.CurrentRotation);
}

// ------------------------------------------------------------
// Rebuild instance transforms when formation changes
// ------------------------------------------------------------
void UCameraSpawnSystem::RefreshPreviewInstances()
{
    if (!bIsInSpawnMode)
        return;

    ShowUnitPreview(GetSpawnComponent()->GetUnitToSpawn());
}

// ------------------------------------------------------------
// Compute final transforms for preview instances
// ------------------------------------------------------------
void UCameraSpawnSystem::UpdatePreviewTransforms(const FVector& Center, const FRotator& Facing)
{
    if (!PreviewSystem || !PreviewSystem->HasPreviewActor())
        return;

    const int32 Count = GetEffectiveSpawnCount();
    const float Spacing = GetSpawnComponent() ? FMath::Max(GetSpawnComponent()->GetFormationSpacing(), 0.f) : 0.f;

    TArray<FVector> Offsets;
	GetSpawnComponent()->BuildSpawnFormationOffsets(Count, Spacing, Offsets, Facing);

    if (Offsets.IsEmpty())
        return;

    TArray<FTransform> Transforms;
    Transforms.Reserve(Offsets.Num());

    const FQuat RotQ = FRotator(0.f, Facing.Yaw, 0.f).Quaternion();

    for (const FVector& Offset : Offsets)
    {
        const FVector Rotated = RotQ.RotateVector(Offset);
        Transforms.Add(FTransform(Facing, Center + Rotated));
    }

    PreviewSystem->UpdateInstances(Transforms);
}
