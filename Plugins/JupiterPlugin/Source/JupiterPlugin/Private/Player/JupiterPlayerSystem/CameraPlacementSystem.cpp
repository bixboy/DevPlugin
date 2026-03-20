#include "Player/JupiterPlayerSystem/CameraPlacementSystem.h"
#include "Player/JupiterPlayerSystem/CameraPreviewSystem.h"
#include "Player/PlayerCamera.h"

#include "Components/Placement/PlacementHandlerComponent.h"
#include "Components/Placement/PresetManagerComponent.h"
#include "Components/Unit/UnitSelectionComponent.h"
#include "Interfaces/PlacementItemInterface.h"

#include "Data/Placement/PlacementPropData.h"
#include "Data/Placement/PresetData.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

#include "GameFramework/PlayerController.h"
#include "Utilities/PreviewPoseMesh.h"
#include "Kismet/KismetMathLibrary.h"
#include "Camera/CameraComponent.h"

#include "Engine/World.h"
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

void UCameraPlacementSystem::StartPlacement(UPlacementItemData* ItemToPlace)
{
    if (!ItemToPlace)
    {
        UE_LOG(LogTemp, Warning, TEXT("StartPlacement: ItemToPlace is NULL — cancelling."));
        CancelPlacement();
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("StartPlacement: '%s' | ActorToSpawn=%s | PreviewMesh=%s"),
        *ItemToPlace->DisplayName.ToString(),
        ItemToPlace->ActorToSpawn ? *ItemToPlace->ActorToSpawn->GetName() : TEXT("NULL"),
        ItemToPlace->PreviewMesh.IsNull() ? TEXT("NULL") : *ItemToPlace->PreviewMesh.GetAssetName());

    CurrentItemData = ItemToPlace;
    bIsPlacementActive = true;
	
    if (ItemToPlace->Implements<UPlacementItemInterface>())
    {
        CurrentSpawnCount = IPlacementItemInterface::Execute_GetDefaultUnitCount(ItemToPlace);
        CurrentSpacing = IPlacementItemInterface::Execute_GetFormationSpacing(ItemToPlace);
        CurrentFormation = static_cast<ESpawnFormation>(IPlacementItemInterface::Execute_GetDefaultFormation(ItemToPlace));
    }
    else
    {
        CurrentSpawnCount = 1;
        CurrentSpacing = 100.f;
        CurrentFormation = ESpawnFormation::Square;
    }

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

void UCameraPlacementSystem::HandlePlacementStarted()
{
	if (!bIsPlacementActive || !CurrentItemData)
		return;

	if (UUnitSelectionComponent* SelComp = GetSelectionComponent())
	{
		FHitResult Hit = SelComp->GetMousePositionOnTerrain();
		const FVector MousePos = Hit.Location;

		if (!MousePos.IsNearlyZero())
		{
			const float CurrentTime = (GetWorldSafe() ? GetWorldSafe()->GetTimeSeconds() : 0.f);
			
            float Yaw = 0.f;
            if (GetOwner() && GetOwner()->GetCameraComponent())
            {
                 Yaw = GetOwner()->GetCameraComponent()->GetComponentRotation().Yaw;
            }
			
            const FRotator CameraAlignRot(0, Yaw, 0);
			RotationState.BeginHold(CurrentTime, MousePos, CameraAlignRot);
		}
	}
}

void UCameraPlacementSystem::HandlePlacementReleased()
{
    if (!bIsPlacementActive || !CurrentItemData)
        return;

	UE_LOG(LogTemp, Log, TEXT("HandlePlacementReleased: INPUT RECEIVED."));

    if (PreviewSystem && !PreviewSystem->IsPlacementValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("HandlePlacementReleased: Placement INVALID (collision or slope)."));
    	
        // TODO: Sound Error
    	
        RotationState.StopHold();
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("HandlePlacementReleased: Placement VALID. Requesting Server Spawn..."));
    
    // Calculate Final Transform
    FRotator FinalRot = RotationState.bPreviewActive ? RotationState.CurrentRotation : RotationState.BaseRotation;
    FVector FinalLoc = RotationState.Center;
    
    APlayerCamera* PC = GetOwner();
    if (!PC)
    {
        RotationState.StopHold();
        CancelPlacement();
        return;
    }

    // --- CASE A: PRESET ---
    if (const UPlacementPresetData* PresetData = Cast<UPlacementPresetData>(CurrentItemData))
    {
         if (UPresetManagerComponent* PMC = PC->FindComponentByClass<UPresetManagerComponent>())
         {
             PMC->Server_SpawnPreset(PresetData->Preset.PresetID, FinalLoc, FinalRot);
         }
         else
         {
             UE_LOG(LogTemp, Error, TEXT("CameraPlacementSystem: Missing PresetManagerComponent!"));
         }
    }
    // --- CASE B: STANDARD ITEM (Unit / Prop) ---
    else
    {
        if (UPlacementHandlerComponent* Handler = PC->FindComponentByClass<UPlacementHandlerComponent>())
        {
             FVector SpawnLoc = FinalLoc + FinalRot.RotateVector(CurrentItemData->PreviewOffset + AutoGroundOffset);
        	
            Handler->Server_RequestPlacement(CurrentItemData, SpawnLoc, FinalRot, 
            	CurrentSpawnCount, CurrentFormation, CustomFormationDimensions);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("CameraPlacementSystem: Missing PlacementHandlerComponent!"));
        }
    }
	
    RotationState.StopHold();
    CancelPlacement();
}

void UCameraPlacementSystem::UpdatePreviewVisuals()
{
    if (!PreviewSystem || !CurrentItemData) 
    	return;

    UStreamableRenderAsset* Asset = nullptr;
    AutoGroundOffset = FVector::ZeroVector;



    // --- PRESET HANDLING ---
    if (const UPlacementPresetData* PresetData = Cast<UPlacementPresetData>(CurrentItemData))
    {
        if (PresetData->Preset.IsValid())
        {
             PreviewSystem->ShowPresetPreview(PresetData->Preset);
             return;
        }
    }

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

             if (!Asset)
             {
                 TArray<UActorComponent*> AllComps;
                 CDO->GetComponents(AllComps);
                 for (UActorComponent* Comp : AllComps)
                 {
                     if (auto* SK = Cast<USkeletalMeshComponent>(Comp))
                     {
                         Asset = SK->GetSkeletalMeshAsset();
                         if (Asset)
							break;
                     }
                     else if (auto* SM = Cast<UStaticMeshComponent>(Comp))
                     {
                         Asset = SM->GetStaticMesh();
                         if (Asset)
                         	break;
                     }
                 }
             }
        }
    }

    if (!Asset)
    {
    	if (CurrentItemData->PreviewMesh.IsPending())
    	{
    		FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
    		Streamable.RequestAsyncLoad(CurrentItemData->PreviewMesh.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &UCameraPlacementSystem::OnPreviewAssetLoaded));
    		return;
    	}
    
        UE_LOG(LogTemp, Warning, TEXT("UpdatePreviewVisuals: No preview mesh found for '%s'. Set PreviewMesh on the DataAsset or ensure the actor has a visible mesh component."),
            *CurrentItemData->DisplayName.ToString());
    	
        return;
    }

	if (const UPlacementPropData* PropData = Cast<UPlacementPropData>(CurrentItemData))
	{
		if (PropData->bAutoGround)
		{
			FBoxSphereBounds Bounds(FVector::ZeroVector, FVector::ZeroVector, 0.f);
			if (USkeletalMesh* Skel = Cast<USkeletalMesh>(Asset))
			{
				Bounds = Skel->GetBounds();
			}
			else if (UStaticMesh* Static = Cast<UStaticMesh>(Asset))
			{
				Bounds = Static->GetBounds();
			}
			
			float MinZ = Bounds.Origin.Z - Bounds.BoxExtent.Z;
			float ZLift = -MinZ * CurrentItemData->InternalScale.Z;

			AutoGroundOffset = FVector(0, 0, ZLift);

			FString AssetName = Asset->GetName();
			UE_LOG(LogTemp, Log, TEXT("AutoGround [%s]: BoundsOriginZ=%f, ExtentZ=%f -> MinZ=%f -> ZLift=%f"), 
				*AssetName, Bounds.Origin.Z, Bounds.BoxExtent.Z, MinZ, ZLift);
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
    	
         // --- Rotation Logic ---
        
         if (RotationState.bHoldActive)
         {
             if (!RotationState.bPreviewActive)
             {
                 if (RotationState.TryActivate(CurrentTime, RotationHoldTime, MousePos, MousePos)) 
                 {
                     // Activated!
                 }
             }

             if (RotationState.bPreviewActive)
				RotationState.UpdateRotation(MousePos);
         }
         else
         {
             const float Yaw = GetOwner()->GetCameraComponent()->GetComponentRotation().Yaw;
             RotationState.BaseRotation = FRotator(0, Yaw, 0);
             RotationState.Center = MousePos;
             RotationState.CurrentRotation = RotationState.BaseRotation;
         }

         UpdateTransforms(RotationState.Center, RotationState.CurrentRotation);
    }
}

void UCameraPlacementSystem::UpdateTransforms(const FVector& Center, const FRotator& Facing)
{
    if (!PreviewSystem || !CurrentItemData) 
    	return;
    
    PreviewSystem->SetPreviewTransform(Center, Facing);
    CachedTransforms.Reset();

    CachedTransforms.Reset();

    if (Cast<UPlacementPresetData>(CurrentItemData))
		return;

    bool bSupportsFormations = false;
    if (CurrentItemData->Implements<UPlacementItemInterface>())
    {
        bSupportsFormations = IPlacementItemInterface::Execute_SupportsFormations(CurrentItemData);
    }

    if (bSupportsFormations)
    {
        BuildGroupTransforms(CurrentItemData, Center, Facing, CachedTransforms);
    }
    else
    {
        BuildSingleTransform(Center, Facing, CachedTransforms);
    }

    PreviewSystem->UpdateInstances(CachedTransforms);
}

void UCameraPlacementSystem::BuildSingleTransform(const FVector& Center, const FRotator& Facing, TArray<FTransform>& OutTransforms)
{
    const FVector LocalOffset = Facing.RotateVector(CurrentItemData->PreviewOffset + AutoGroundOffset);

    OutTransforms.Emplace(FTransform(FRotator::ZeroRotator, LocalOffset));
}

void UCameraPlacementSystem::BuildGroupTransforms(UPlacementItemData* ItemData, const FVector& Center, const FRotator& Facing, TArray<FTransform>& OutTransforms)
{
    CachedOffsets.Reset();
    
    int32 Count = CurrentSpawnCount;
    float Spacing = CurrentSpacing;
	
    if (Count <= 0) 
        return;

    FVector RightDir = UKismetMathLibrary::GetRightVector(Facing);
    FVector ForwardDir = UKismetMathLibrary::GetForwardVector(Facing);

    // --- FORMATION CALCULATION START ---
	
    if (CurrentFormation == ESpawnFormation::Line) // Line
    {
        float Width = (Count - 1) * Spacing;
        FVector StartPos = Center - (RightDir * Width * 0.5f);

        for (int32 i = 0; i < Count; i++)
        {
            CachedOffsets.Add(StartPos + (RightDir * i * Spacing));
        }
    }
    else if (CurrentFormation == ESpawnFormation::Column) // Column
    {
        float Depth = (Count - 1) * Spacing;
        FVector StartPos = Center - (ForwardDir * Depth * 0.5f);

        for (int32 i = 0; i < Count; i++)
        {
            CachedOffsets.Add(StartPos + (ForwardDir * i * Spacing));
        }
    }
    else if (CurrentFormation == ESpawnFormation::Wedge) // Wedge
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
    else if (CurrentFormation == ESpawnFormation::Custom) // Custom
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
