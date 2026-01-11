#include "Player/JupiterPlayerSystem/CameraSpawnSystem.h"
#include "Camera/CameraComponent.h"
#include "Player/PlayerCamera.h"
#include "Player/JupiterPlayerSystem/CameraPreviewSystem.h"
#include "Components/Unit/UnitSpawnComponent.h"
#include "Units/SoldierRts.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/Unit/UnitSelectionComponent.h"
#include "Utilities/PreviewPoseMesh.h"


void UCameraSpawnSystem::Init(APlayerCamera* InOwner)
{
    Super::Init(InOwner);

    if (!GetOwner() || !GetOwner()->GetSpawnComponent())
    {
        UE_LOG(LogTemp, Error, TEXT("Init Failed: SpawnComponent missing on Owner"));
        return;
    }

    SpawnRotationPreview.Reset(FRotator::ZeroRotator);
    
    if (UUnitSpawnComponent* Spawn = GetSpawnComponent())
    {
        Spawn->OnUnitClassChanged.AddDynamic(this, &UCameraSpawnSystem::OnUnitClassChanged);
        Spawn->OnSpawnCountChanged.AddDynamic(this, &UCameraSpawnSystem::OnSpawnCountChanged);
        Spawn->OnSpawnFormationChanged.AddDynamic(this, &UCameraSpawnSystem::OnFormationChanged);
        
        if (Spawn->GetUnitToSpawn())
        {
            ShowUnitPreview(Spawn->GetUnitToSpawn());
        }
    }
}

void UCameraSpawnSystem::Tick(float DeltaTime)
{
    if (!bIsInSpawnMode || !PreviewSystem)
        return;

    const float CurrentTime = (GetWorldSafe() ? GetWorldSafe()->GetTimeSeconds() : 0.f);
    UpdatePreviewFollowMouse(CurrentTime);
}

#pragma region Actions

// ------------------------------------------------------------
// Validation du Spawn
// ------------------------------------------------------------
void UCameraSpawnSystem::HandleSpawnInput()
{
    if (!bIsInSpawnMode || !PreviewSystem)
    	return;

    UUnitSpawnComponent* SpawnComp = GetSpawnComponent();
    if (!SpawnComp)
    	return;

    if (!PreviewSystem->IsPlacementValid())
    {
        // TODO: Jouer un son d'erreur ?
    	UE_LOG(LogTemp, Warning, TEXT("Impossible de spawn %s emplacement non valide"), *SpawnComp->GetUnitToSpawn()->GetName());
        return;
    }

    const FVector SpawnLocation = SpawnRotationPreview.Center;
    const FRotator SpawnRot = SpawnRotationPreview.bPreviewActive ? SpawnRotationPreview.CurrentRotation : FRotator::ZeroRotator;

    SpawnComp->SpawnUnitsWithTransform(SpawnLocation, SpawnRot);

    SpawnRotationPreview.Deactivate();
    bIsInSpawnMode = false;
    
    if (PreviewSystem)
        PreviewSystem->HidePreview();
}

void UCameraSpawnSystem::ResetSpawnState()
{
    bIsInSpawnMode = false;
    SpawnRotationPreview.Deactivate();
    LastPreviewedClass = nullptr;

    if (PreviewSystem)
        PreviewSystem->HidePreview();
}

#pragma endregion

#pragma region Preview Logic

// ------------------------------------------------------------
// Mise à jour du visuel (Mesh) de l'unité
// ------------------------------------------------------------
void UCameraSpawnSystem::ShowUnitPreview(TSubclassOf<ASoldierRts> NewUnitClass)
{
    if (!PreviewSystem) return;

    if (NewUnitClass == LastPreviewedClass && bIsInSpawnMode)
    {
        RefreshPreviewInstances();
        return;
    }

    if (!EnsurePreviewActor() || !NewUnitClass)
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

    LastPreviewedClass = NewUnitClass;
    const int32 InstanceCount = GetEffectiveSpawnCount();

    if (GetOwner()->GetCameraComponent())
    {
        const float Yaw = GetOwner()->GetCameraComponent()->GetComponentRotation().Yaw;
        SpawnRotationPreview.Reset(FRotator(0, Yaw, 0));
    }

    // Détection du type de Mesh (Skeletal vs Static)
    bool bSuccess = false;

    // A. Skeletal Mesh
    if (USkeletalMeshComponent* SkelComp = DefaultUnit->GetMesh())
    {
        if (USkeletalMesh* Skel = SkelComp->GetSkeletalMeshAsset())
        {
            bSuccess = PreviewSystem->ShowSkeletalPreview(Skel, SkelComp->GetRelativeScale3D(), InstanceCount);
        }
    }

    // B. Static Mesh 
    if (!bSuccess)
    {
        if (UStaticMeshComponent* StaticComp = DefaultUnit->FindComponentByClass<UStaticMeshComponent>())
        {
            if (UStaticMesh* Mesh = StaticComp->GetStaticMesh())
            {
                bSuccess = PreviewSystem->ShowStaticPreview(Mesh, StaticComp->GetRelativeScale3D(), InstanceCount);
            }
        }
    }

    if (bSuccess)
    {
        bIsInSpawnMode = true;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Impossible de trouver un Mesh valide pour la preview de %s"), *NewUnitClass->GetName());
        ResetSpawnState();
    }
}

// ------------------------------------------------------------
// Mise à jour des positions
// ------------------------------------------------------------
void UCameraSpawnSystem::RefreshPreviewInstances()
{
    if (!bIsInSpawnMode || !PreviewSystem || !PreviewSystem->HasPreviewActor())
        return;
	
    TSubclassOf<ASoldierRts> CurrentClass = LastPreviewedClass;
    LastPreviewedClass = nullptr; 
    ShowUnitPreview(CurrentClass);
}

// ------------------------------------------------------------
// Calculs des transforms
// ------------------------------------------------------------
void UCameraSpawnSystem::UpdatePreviewTransforms(const FVector& Center, const FRotator& Facing)
{
	if (!PreviewSystem || !PreviewSystem->HasPreviewActor())
		return;

	PreviewSystem->SetPreviewTransform(Center, Facing);

	UUnitSpawnComponent* SpawnComp = GetSpawnComponent();
	if (!SpawnComp)
		return;

	const int32 Count = GetEffectiveSpawnCount();
	const float Spacing = FMath::Max(SpawnComp->GetFormationSpacing(), 0.f);

	CachedOffsets.Reset();
    
	SpawnComp->BuildSpawnFormationOffsets(Count, Spacing, CachedOffsets, FRotator::ZeroRotator);

	if (CachedOffsets.IsEmpty())
		return;

    CachedTransforms.Reset();
    CachedTransforms.Reserve(CachedOffsets.Num());

    UWorld* World = GetWorldSafe();
    FCollisionQueryParams Params;
    if (GetOwner())
    	Params.AddIgnoredActor(GetOwner());
	
    if (PreviewSystem && PreviewSystem->GetPreviewActor())
    	Params.AddIgnoredActor(PreviewSystem->GetPreviewActor());
    
    FTransform ActorTransform(Facing, Center);

    for (const FVector& Offset : CachedOffsets)
    {
        FVector WorldPos = ActorTransform.TransformPosition(Offset);

        FVector TraceStart = WorldPos + FVector(0, 0, 500.f);
        FVector TraceEnd = WorldPos + FVector(0, 0, -1000.f);
        FHitResult Hit;
        
        if (World && World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
        {
             WorldPos = Hit.Location;
        }
    	
        FVector LocalPos = ActorTransform.InverseTransformPosition(WorldPos);
        CachedTransforms.Emplace(FTransform(LocalPos));
    }

    PreviewSystem->UpdateInstances(CachedTransforms);
}

// ------------------------------------------------------------
// Gestion Souris & Rotation (Drag & Drop Logic)
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

    // Input States
    const bool bLDown = PC->IsInputKeyDown(EKeys::LeftMouseButton);
    const bool bRDown = PC->IsInputKeyDown(EKeys::RightMouseButton);
    const bool bDown = (bLDown || bRDown);

    const bool bJustPressed = PC->WasInputKeyJustPressed(EKeys::LeftMouseButton);

    // Rotation par défaut alignée caméra
    const float Yaw = GetOwner()->GetCameraComponent()->GetComponentRotation().Yaw;
    const FRotator CameraAlignRot(0, Yaw, 0);

    // --- State Machine Rotation ---
    
    // 1. Début du Clic
    if (bJustPressed)
    {
        SpawnRotationPreview.BeginHold(CurrentTime, MousePos, CameraAlignRot);
    }
    // 2. Relâchement
    else if (!bDown)
    {
        SpawnRotationPreview.StopHold();
    }

    // 3. Mode "Suivi Souris Simple"
    if (!SpawnRotationPreview.bPreviewActive)
    {
        SpawnRotationPreview.Center = MousePos;
        SpawnRotationPreview.BaseRotation = CameraAlignRot;
        SpawnRotationPreview.CurrentRotation = CameraAlignRot;
    }

    // 4. Détection du passage en mode "Drag-Rotate"
    if (SpawnRotationPreview.bHoldActive && !SpawnRotationPreview.bPreviewActive)
    {
        if (SpawnRotationPreview.TryActivate(CurrentTime, RotationPreviewHoldTime, MousePos, MousePos))
        {
            SpawnRotationPreview.InitialDirection = FRotationPreviewState::ResolvePlanarDirection(MousePos - SpawnRotationPreview.Center,
            	SpawnRotationPreview.BaseRotation);
        }
    }

    // 5. Update pendant le Drag-Rotate
    if (SpawnRotationPreview.bPreviewActive)
    {
        SpawnRotationPreview.UpdateRotation(MousePos);
    }

    SpawnRotationPreview.Center = MousePos;
    
    UpdatePreviewTransforms(SpawnRotationPreview.Center, SpawnRotationPreview.CurrentRotation);
}

#pragma endregion


#pragma region Helpers & Events

bool UCameraSpawnSystem::EnsurePreviewActor()
{
    return (PreviewSystem && PreviewSystem->EnsurePreviewActor());
}

int32 UCameraSpawnSystem::GetEffectiveSpawnCount() const
{
    if (UUnitSpawnComponent* Spawn = GetSpawnComponent())
    {
        if (Spawn->GetSpawnFormation() == ESpawnFormation::Custom)
        {
            const FIntPoint Dim = Spawn->GetCustomFormationDimensions();
            return FMath::Max(1, Dim.X * Dim.Y);
        }
    	
        return FMath::Max(1, Spawn->GetUnitsPerSpawn());
    }
	
    return 1;
}

void UCameraSpawnSystem::OnUnitClassChanged(TSubclassOf<ASoldierRts> NewUnitClass)
{
    ShowUnitPreview(NewUnitClass);
}

void UCameraSpawnSystem::OnSpawnCountChanged(int32 /*NewCount*/)
{
    RefreshPreviewInstances();
}

void UCameraSpawnSystem::OnFormationChanged(ESpawnFormation /*NewFormation*/)
{
    RefreshPreviewInstances();
}

#pragma endregion