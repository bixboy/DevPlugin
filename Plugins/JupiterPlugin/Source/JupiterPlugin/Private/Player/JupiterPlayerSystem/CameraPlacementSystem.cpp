#include "Player/JupiterPlayerSystem/CameraPlacementSystem.h"
#include "Player/JupiterPlayerSystem/CameraPreviewSystem.h"
#include "Player/JupiterPlayerSystem/CameraCommandSystem.h"
#include "Player/PlayerCamera.h"
#include "Components/Placement/PlacementHandlerComponent.h"
#include "Components/Unit/UnitSelectionComponent.h"
#include "Data/Placement/PlacementUnitData.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Units/SoldierRts.h" 
#include "Utilities/PreviewPoseMesh.h"
#include "Utilities/PreviewPoseMesh.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"


void UCameraPlacementSystem::Init(APlayerCamera* InOwner)
{
	Super::Init(InOwner);
    RotationState.Reset(FRotator::ZeroRotator);
}

void UCameraPlacementSystem::Tick(float DeltaTime)
{
    if (!bIsPlacementActive || !CurrentItemData || !PreviewSystem)
        return;

    const float CurrentTime = (GetWorldSafe() ? GetWorldSafe()->GetTimeSeconds() : 0.f);
    UpdateMouseFollow(CurrentTime);
}

	// --- Setters ---
void UCameraPlacementSystem::SetSpawnCount(int32 NewCount)
{
	if (NewCount != CurrentSpawnCount)
	{
		CurrentSpawnCount = FMath::Max(1, NewCount);
		OnSpawnCountChanged.Broadcast(CurrentSpawnCount);
		UpdatePreviewVisuals();
	}
}

void UCameraPlacementSystem::SetFormation(ESpawnFormation NewFormation)
{
	if (NewFormation != CurrentFormation)
	{
		CurrentFormation = NewFormation;
		OnSpawnFormationChanged.Broadcast(CurrentFormation);
		UpdatePreviewVisuals();
	}
}

void UCameraPlacementSystem::SetCustomFormationDimensions(FIntPoint NewDimensions)
{
	if (NewDimensions != CustomFormationDimensions)
	{
		CustomFormationDimensions = NewDimensions;
		OnCustomFormationDimensionsChanged.Broadcast(CustomFormationDimensions);
		UpdatePreviewVisuals();
	}
}

void UCameraPlacementSystem::SetCurrentSpacing(float NewSpacing)
{
    CurrentSpacing = FMath::Max(50.0f, NewSpacing);
}

void UCameraPlacementSystem::StartPlacement(const UPlacementItemData* ItemToPlace)
{
    if (!ItemToPlace)
    {
        CancelPlacement();
        return;
    }

    CurrentItemData = ItemToPlace;
    bIsPlacementActive = true;
	
    OnSpawnCountChanged.Broadcast(CurrentSpawnCount);
    OnSpawnFormationChanged.Broadcast(CurrentFormation);
    OnCustomFormationDimensionsChanged.Broadcast(CustomFormationDimensions);
    
    if (GetOwner() && GetOwner()->GetCameraComponent())
    {
         const float Yaw = GetOwner()->GetCameraComponent()->GetComponentRotation().Yaw;
         RotationState.Reset(FRotator(0, Yaw, 0));
    }

    UpdatePreviewVisuals();
}

void UCameraPlacementSystem::CancelPlacement()
{
    bIsPlacementActive = false;
    CurrentItemData = nullptr;
    RotationState.Deactivate();

    if (PreviewSystem)
    {
        PreviewSystem->HidePreview();
    }
}

void UCameraPlacementSystem::HandlePlacementInput()
{
    if (!bIsPlacementActive || !CurrentItemData)
        return;

    if (PreviewSystem && !PreviewSystem->IsPlacementValid())
    {
        // TODO: Sound Error
        return;
    }

    if (APlayerCamera* PC = GetOwner())
    {
        if (UPlacementHandlerComponent* Handler = PC->FindComponentByClass<UPlacementHandlerComponent>())
        {
             const FVector SpawnLoc = RotationState.Center;
             const FRotator SpawnRot = RotationState.bPreviewActive ? RotationState.CurrentRotation : FRotator::ZeroRotator;
             
             Handler->Server_RequestPlacement(CurrentItemData, SpawnLoc, SpawnRot, CurrentSpawnCount, CurrentFormation, CustomFormationDimensions);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("CameraPlacementSystem: Missing PlacementHandlerComponent on PlayerCamera!"));
        }
    }
	
    CancelPlacement();
}

void UCameraPlacementSystem::UpdatePreviewVisuals()
{
    if (!PreviewSystem || !CurrentItemData) 
    	return;

    UStreamableRenderAsset* Asset = nullptr;

    if (CurrentItemData->PreviewMesh.IsValid())
    {
         Asset = CurrentItemData->PreviewMesh.Get();
    }
    else if (CurrentItemData->PreviewMesh.IsNull() && CurrentItemData->ActorToSpawn)
    {
        if (AActor* CDO = CurrentItemData->ActorToSpawn->GetDefaultObject<AActor>())
        {
             if (USkeletalMeshComponent* SkelComp = CDO->FindComponentByClass<USkeletalMeshComponent>())
             {
                 Asset = SkelComp->GetSkeletalMeshAsset();
             }
             else if (UStaticMeshComponent* StaticComp = CDO->FindComponentByClass<UStaticMeshComponent>())
             {
                 Asset = StaticComp->GetStaticMesh();
             }
        }
    }

    if (Asset)
    {
        int32 Count = CurrentSpawnCount;
        bool bSuccess;
    	
        if (USkeletalMesh* Skel = Cast<USkeletalMesh>(Asset))
        {
            bSuccess = PreviewSystem->ShowSkeletalPreview(Skel, CurrentItemData->InternalScale, Count);
        }
        else if (UStaticMesh* Static = Cast<UStaticMesh>(Asset))
        {
            bSuccess = PreviewSystem->ShowStaticPreview(Static, CurrentItemData->InternalScale, Count);
        }
    	
        return;
    }

    if (CurrentItemData->PreviewMesh.IsPending())
    {
       FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
       Streamable.RequestAsyncLoad(CurrentItemData->PreviewMesh.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &UCameraPlacementSystem::OnPreviewAssetLoaded));
    }
}

void UCameraPlacementSystem::OnPreviewAssetLoaded()
{
    if (bIsPlacementActive && CurrentItemData)
    {
        UpdatePreviewVisuals();
    }
}

void UCameraPlacementSystem::UpdateMouseFollow(float CurrentTime)
{
    if (!PreviewSystem || !PreviewSystem->HasPreviewActor())
        return;
    
    if (UUnitSelectionComponent* SelComp = GetSelectionComponent())
    {
         FHitResult Hit = SelComp->GetMousePositionOnTerrain();
         FVector MousePos = Hit.Location;

         if (MousePos.IsNearlyZero()) 
         	return;
         
         if (APlayerController* PC = GetOwner()->GetPlayerController())
         {
             const bool bLDown = PC->IsInputKeyDown(EKeys::LeftMouseButton);
             const bool bJustPressed = PC->WasInputKeyJustPressed(EKeys::LeftMouseButton);
             
             // --- Rotation Logic ---
             const float Yaw = GetOwner()->GetCameraComponent()->GetComponentRotation().Yaw;
             const FRotator CameraAlignRot(0, Yaw, 0);

             if (bJustPressed)
             {
                RotationState.BeginHold(CurrentTime, MousePos, CameraAlignRot);
             }
             else if (!bLDown)
             {
                RotationState.StopHold();
             }

             if (!RotationState.bPreviewActive)
             {
                 RotationState.Center = MousePos;
                 RotationState.BaseRotation = CameraAlignRot;
                 RotationState.CurrentRotation = CameraAlignRot;
             }

             if (RotationState.bHoldActive && !RotationState.bPreviewActive)
             {
                 if (RotationState.TryActivate(CurrentTime, RotationHoldTime, MousePos, MousePos))
                 {
                      RotationState.InitialDirection = FRotationPreviewState::ResolvePlanarDirection(MousePos - RotationState.Center, RotationState.BaseRotation);
                 }
             }

             if (RotationState.bPreviewActive)
             {
                 RotationState.UpdateRotation(MousePos);
             }
             else
             {
                 RotationState.Center = MousePos;
             }

             UpdateTransforms(RotationState.Center, RotationState.CurrentRotation);
         }
    }
}

void UCameraPlacementSystem::UpdateTransforms(const FVector& Center, const FRotator& Facing)
{
    if (!PreviewSystem || !CurrentItemData) 
    	return;
    
    PreviewSystem->SetPreviewTransform(Center, Facing);
    CachedTransforms.Reset();

    if (const UPlacementUnitData* UnitData = Cast<UPlacementUnitData>(CurrentItemData))
    {
        BuildGroupTransforms(UnitData, Center, Facing, CachedTransforms);
    }
    else
    {
        BuildSingleTransform(Center, Facing, CachedTransforms);
    }

    PreviewSystem->UpdateInstances(CachedTransforms);
}

void UCameraPlacementSystem::BuildSingleTransform(const FVector& Center, const FRotator& Facing, TArray<FTransform>& OutTransforms)
{
    FVector FinalPos = Center + Facing.RotateVector(CurrentItemData->PreviewOffset);
    OutTransforms.Emplace(FTransform(Facing, FinalPos));
}

void UCameraPlacementSystem::BuildGroupTransforms(const UPlacementUnitData* UnitData, const FVector& Center, const FRotator& Facing, TArray<FTransform>& OutTransforms)
{
    CachedOffsets.Reset();
    
    int32 Count = CurrentSpawnCount;
    float Spacing = CurrentSpacing;
    if (Count <= 0) 
        return;

    FVector RightDir = UKismetMathLibrary::GetRightVector(Facing);
    FVector ForwardDir = UKismetMathLibrary::GetForwardVector(Facing);

    // --- FORMATION CALCULATION START ---
    if (CurrentFormation == ESpawnFormation::Line)
    {
        float Width = (Count - 1) * Spacing;
        FVector StartPos = Center - (RightDir * Width * 0.5f);

        for (int32 i = 0; i < Count; i++)
        {
            CachedOffsets.Add(StartPos + (RightDir * i * Spacing));
        }
    }
    else if (CurrentFormation == ESpawnFormation::Column)
    {
        float Depth = (Count - 1) * Spacing;
        FVector StartPos = Center - (ForwardDir * Depth * 0.5f);

        for (int32 i = 0; i < Count; i++)
        {
            CachedOffsets.Add(StartPos + (ForwardDir * i * Spacing));
        }
    }
    else if (CurrentFormation == ESpawnFormation::Wedge)
    {
        int32 CurrentIdx = 0;
        int32 Row = 0;
        while (CurrentIdx < Count)
        {
            int32 UnitsInRow = Row + 1;
            float RowWidth = (UnitsInRow - 1) * Spacing;
            FVector RowStart = Center - (RightDir * RowWidth * 0.5f) - (ForwardDir * Row * Spacing);

            for (int32 k = 0; k < UnitsInRow && CurrentIdx < Count; k++)
            {
                CachedOffsets.Add(RowStart + (RightDir * k * Spacing));
                CurrentIdx++;
            }
            Row++;
        }
    }
    else if (CurrentFormation == ESpawnFormation::Custom)
    {
        int32 Columns = CustomFormationDimensions.X > 0 ? CustomFormationDimensions.X : 1;
        float Width = (Columns - 1) * Spacing;
        FVector LineStart = Center - (RightDir * Width * 0.5f) - (ForwardDir * ((Count/Columns) * Spacing * 0.5f));

        for (int32 i = 0; i < Count; i++)
        {
             int32 Row = i / Columns;
             int32 Col = i % Columns;
             CachedOffsets.Add(LineStart + (RightDir * Col * Spacing) + (ForwardDir * Row * Spacing));
        }
    }
    else // Square / Default
    {
        int32 RowSize = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(Count)));
        float Width = (RowSize - 1) * Spacing;
        FVector StartPos = Center - (RightDir * Width * 0.5f) - (ForwardDir * Width * 0.5f);

        for (int32 i = 0; i < Count; i++)
        {
            int32 Row = i / RowSize;
            int32 Col = i % RowSize;
            CachedOffsets.Add(StartPos + (RightDir * Col * Spacing) + (ForwardDir * Row * Spacing));
        }
    }
    // --- FORMATION CALCULATION END ---

    UWorld* World = GetWorldSafe();
    FCollisionQueryParams Params;
    if (GetOwner()) 
        Params.AddIgnoredActor(GetOwner());

    if (PreviewSystem->GetPreviewActor()) 
        Params.AddIgnoredActor(PreviewSystem->GetPreviewActor());

    for (const FVector& BasePos : CachedOffsets)
    {
        FVector TargetPos = BasePos;
        
        if (World)
        {
             FVector TraceStart = TargetPos + FVector(0,0,500);
             FVector TraceEnd = TargetPos + FVector(0,0,-1000);
             FHitResult Hit;
             if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
             {
                 TargetPos = Hit.Location;
             }
        }
        
        FTransform RootTransform(Facing, Center);
        FVector LocalPos = RootTransform.InverseTransformPosition(TargetPos);
        
        OutTransforms.Emplace(FTransform(LocalPos));
    }
}
