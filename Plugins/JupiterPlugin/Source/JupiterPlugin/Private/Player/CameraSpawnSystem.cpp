#include "Player/CameraSpawnSystem.h"

#include "Components/UnitSelectionComponent.h"
#include "Components/UnitSpawnComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Player/CameraPreviewSystem.h"
#include "Player/PlayerCamera.h"
#include "Camera/CameraComponent.h"
#include "Units/SoldierRts.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Utils/JupiterInputHelpers.h"
#include "Utils/PreviewFormationUtils.h"

void UCameraSpawnSystem::Init(APlayerCamera* InOwner)
{
    Owner = InOwner;
    if (!Owner.IsValid())
    {
        return;
    }

    Player = Owner->GetPlayerController();
    SelectionComponent = Owner->GetSelectionComponent();
    SpawnComponent = Owner->GetSpawnComponent();

    SpawnRotationPreview.Reset(FRotator::ZeroRotator);
    BindSpawnComponentDelegates();

    if (SpawnComponent)
    {
        ShowUnitPreview(SpawnComponent->GetUnitToSpawn());
    }
}

void UCameraSpawnSystem::Tick(float DeltaTime)
{
    if (!bIsInSpawnMode || !Owner.IsValid() || !PreviewSystem.IsValid())
    {
        return;
    }

    const float CurrentTime = Owner->GetWorld() ? Owner->GetWorld()->GetTimeSeconds() : 0.f;
    UpdatePreviewFollowMouse(CurrentTime);
}

void UCameraSpawnSystem::HandleSpawnInput()
{
    if (!bIsInSpawnMode || !PreviewSystem.IsValid() || !SpawnComponent)
    {
        return;
    }

    if (!PreviewSystem->IsPlacementValid())
    {
        return;
    }

    const FVector SpawnLocation = SpawnRotationPreview.Center;
    const FRotator SpawnRotation = SpawnRotationPreview.bPreviewActive ? SpawnRotationPreview.CurrentRotation : FRotator::ZeroRotator;
    SpawnComponent->SpawnUnitsWithTransform(SpawnLocation, SpawnRotation);

    SpawnRotationPreview.Deactivate();
    bIsInSpawnMode = false;
    PreviewSystem->HidePreview();
}

void UCameraSpawnSystem::OnSelectionInteractionStarted()
{
    ResetSpawnState();
}

void UCameraSpawnSystem::BindSpawnComponentDelegates()
{
    if (!SpawnComponent)
    {
        return;
    }

    SpawnComponent->OnUnitClassChanged.RemoveAll(this);
    SpawnComponent->OnSpawnCountChanged.RemoveAll(this);
    SpawnComponent->OnSpawnFormationChanged.RemoveAll(this);
    SpawnComponent->OnCustomFormationDimensionsChanged.RemoveAll(this);

    SpawnComponent->OnUnitClassChanged.AddDynamic(this, &UCameraSpawnSystem::HandleUnitClassChanged);
    SpawnComponent->OnSpawnCountChanged.AddDynamic(this, &UCameraSpawnSystem::HandleSpawnCountChanged);
    SpawnComponent->OnSpawnFormationChanged.AddDynamic(this, &UCameraSpawnSystem::HandleSpawnFormationChanged);
    SpawnComponent->OnCustomFormationDimensionsChanged.AddDynamic(this, &UCameraSpawnSystem::HandleCustomFormationDimensionsChanged);
}

void UCameraSpawnSystem::HandleUnitClassChanged(TSubclassOf<ASoldierRts> NewUnitClass)
{
    ShowUnitPreview(NewUnitClass);
}

void UCameraSpawnSystem::HandleSpawnCountChanged(int32 /*NewCount*/)
{
    RefreshPreviewInstances();
}

void UCameraSpawnSystem::HandleSpawnFormationChanged(ESpawnFormation /*NewFormation*/)
{
    RefreshPreviewInstances();
}

void UCameraSpawnSystem::HandleCustomFormationDimensionsChanged(FIntPoint /*NewDimensions*/)
{
    RefreshPreviewInstances();
}

void UCameraSpawnSystem::ShowUnitPreview(TSubclassOf<ASoldierRts> NewUnitClass)
{
    if (!PreviewSystem.IsValid())
    {
        return;
    }

    if (!NewUnitClass)
    {
        PreviewSystem->HidePreview();
        ResetSpawnState();
        return;
    }

    ASoldierRts* DefaultSoldier = NewUnitClass->GetDefaultObject<ASoldierRts>();
    if (!DefaultSoldier)
    {
        PreviewSystem->HidePreview();
        ResetSpawnState();
        return;
    }

    const int32 InstanceCount = PreviewFormationUtils::GetEffectiveSpawnCount(SpawnComponent);
    const FRotator BaseRotation = Owner.IsValid() && Owner->GetCameraComponent() ? FRotator(0.f, Owner->GetCameraComponent()->GetComponentRotation().Yaw, 0.f) : FRotator::ZeroRotator;
    SpawnRotationPreview.Reset(BaseRotation);

    if (USkeletalMeshComponent* MeshComponent = DefaultSoldier->GetMesh())
    {
        if (USkeletalMesh* SkeletalMesh = MeshComponent->GetSkeletalMeshAsset())
        {
            if (PreviewSystem->ShowSkeletalPreview(SkeletalMesh, MeshComponent->GetRelativeScale3D(), InstanceCount))
            {
                bIsInSpawnMode = true;
                RefreshPreviewInstances();
                return;
            }
        }
    }

    if (UStaticMeshComponent* StaticMeshComponent = DefaultSoldier->FindComponentByClass<UStaticMeshComponent>())
    {
        if (UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh())
        {
            if (PreviewSystem->ShowStaticPreview(StaticMesh, StaticMeshComponent->GetRelativeScale3D(), InstanceCount))
            {
                bIsInSpawnMode = true;
                RefreshPreviewInstances();
                return;
            }
        }
    }

    PreviewSystem->HidePreview();
    ResetSpawnState();
}

void UCameraSpawnSystem::RefreshPreviewInstances()
{
    if (!PreviewSystem.IsValid() || !PreviewSystem->HasPreviewActor() || !bIsInSpawnMode)
    {
        return;
    }

    UpdatePreviewFollowMouse(Owner->GetWorld() ? Owner->GetWorld()->GetTimeSeconds() : 0.f);
}

void UCameraSpawnSystem::UpdatePreviewFollowMouse(float CurrentTime)
{
    if (!SelectionComponent || !Player.IsValid())
    {
        return;
    }

    const FHitResult MouseHit = SelectionComponent->GetMousePositionOnTerrain();
    const FVector MouseLocation = MouseHit.Location;
    if (MouseLocation.IsNearlyZero())
    {
        return;
    }

    const bool bLeftMouseDown = JupiterInputHelpers::IsKeyDown(Player.Get(), EKeys::LeftMouseButton);
    const bool bRightMouseDown = JupiterInputHelpers::IsKeyDown(Player.Get(), EKeys::RightMouseButton);
    const bool bSpawnInputDown = bLeftMouseDown || bRightMouseDown;

    const bool bLeftMouseJustPressed = JupiterInputHelpers::WasKeyJustPressed(Player.Get(), EKeys::LeftMouseButton);
    const bool bRightMouseJustPressed = JupiterInputHelpers::WasKeyJustPressed(Player.Get(), EKeys::RightMouseButton);
    const bool bSpawnInputJustPressed = bLeftMouseJustPressed || bRightMouseJustPressed;

    const FRotator MouseRotation = Owner.IsValid() && Owner->GetCameraComponent() ? FRotator(0.f, Owner->GetCameraComponent()->GetComponentRotation().Yaw, 0.f) : FRotator::ZeroRotator;

    if (bSpawnInputJustPressed)
    {
        SpawnRotationPreview.BeginHold(CurrentTime, MouseLocation, MouseRotation);
    }
    else if (!bSpawnInputDown)
    {
        SpawnRotationPreview.StopHold();
    }

    if (!SpawnRotationPreview.bPreviewActive)
    {
        SpawnRotationPreview.Center = MouseLocation;
        SpawnRotationPreview.BaseRotation = MouseRotation;
        SpawnRotationPreview.CurrentRotation = MouseRotation;
    }

    if (SpawnRotationPreview.bHoldActive && !SpawnRotationPreview.bPreviewActive)
    {
        if (SpawnRotationPreview.TryActivate(CurrentTime, RotationPreviewHoldTime, MouseLocation, MouseLocation))
        {
            SpawnRotationPreview.InitialDirection = FRotationPreviewState::ResolvePlanarDirection(MouseLocation - SpawnRotationPreview.Center, SpawnRotationPreview.BaseRotation);
        }
    }

    if (SpawnRotationPreview.bPreviewActive)
    {
        SpawnRotationPreview.UpdateRotation(MouseLocation);
    }

    SpawnRotationPreview.Center = MouseLocation;
    UpdatePreviewTransforms(SpawnRotationPreview.Center, SpawnRotationPreview.CurrentRotation);
}

void UCameraSpawnSystem::UpdatePreviewTransforms(const FVector& CenterLocation, const FRotator& FacingRotation)
{
    if (!PreviewSystem.IsValid())
    {
        return;
    }

    const int32 SpawnCount = PreviewFormationUtils::GetEffectiveSpawnCount(SpawnComponent);
    const float Spacing = SpawnComponent ? FMath::Max(SpawnComponent->GetFormationSpacing(), 0.f) : 0.f;

    TArray<FVector> FormationOffsets;
    PreviewFormationUtils::BuildSpawnFormationOffsets(SpawnComponent, SpawnCount, Spacing, FormationOffsets);

    if (FormationOffsets.IsEmpty())
    {
        return;
    }

    TArray<FTransform> InstanceTransforms;
    InstanceTransforms.Reserve(FormationOffsets.Num());

    const FQuat RotationQuat = FRotator(0.f, FacingRotation.Yaw, 0.f).Quaternion();
    for (const FVector& Offset : FormationOffsets)
    {
        const FVector RotatedOffset = RotationQuat.RotateVector(Offset);
        InstanceTransforms.Add(FTransform(FacingRotation, CenterLocation + RotatedOffset));
    }

    PreviewSystem->UpdateInstances(InstanceTransforms);
}

void UCameraSpawnSystem::ResetSpawnState()
{
    bIsInSpawnMode = false;
    SpawnRotationPreview.Deactivate();
    if (PreviewSystem.IsValid())
    {
        PreviewSystem->HidePreview();
    }
}

