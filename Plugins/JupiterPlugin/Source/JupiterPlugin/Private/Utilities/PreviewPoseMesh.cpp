#include "Utilities/PreviewPoseMesh.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

APreviewPoseMesh::APreviewPoseMesh()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    InstancedStaticMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("InstancedStaticMesh"));
    InstancedStaticMesh->SetupAttachment(Root);
    InstancedStaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    InstancedStaticMesh->SetCanEverAffectNavigation(false);
    InstancedStaticMesh->SetCastShadow(false);

    bReplicates = false;
}

// ---------------------------------------------------------
// SHOW LOGIC
// ---------------------------------------------------------

void APreviewPoseMesh::ShowPreview(UStaticMesh* Mesh, FVector Scale, int32 Count)
{
    if (!Mesh || Count <= 0)
    {
        HidePreview();
        return;
    }

    bUsingStaticMesh = true;
    CurrentScale = Scale;
    SetActorHiddenInGame(false);

	for (auto& P : PoseableMeshes) 
	{
		if (P) 
		{
			P->SetVisibility(false);
		}
	}

    if (InstancedStaticMesh->GetStaticMesh() != Mesh)
    {
        InstancedStaticMesh->SetStaticMesh(Mesh);
    }
    InstancedStaticMesh->SetVisibility(true);

    if (GhostMaterialBase && !SharedMaterialInstance)
    {
        SharedMaterialInstance = UMaterialInstanceDynamic::Create(GhostMaterialBase, this);
    }

    if (SharedMaterialInstance)
    {
        int32 MatCount = Mesh->GetStaticMaterials().Num();
        for (int32 i = 0; i < MatCount; i++)
        {
            InstancedStaticMesh->SetOverlayMaterial(SharedMaterialInstance);
        }
    }
}

void APreviewPoseMesh::ShowPreview(USkeletalMesh* Mesh, FVector Scale, int32 Count)
{
	if (!Mesh || Count <= 0)
	{
		HidePreview();
		return;
	}

	bUsingStaticMesh = false;
	CurrentScale = Scale;
	SetActorHiddenInGame(false);

	// 1. Désactiver Static
	InstancedStaticMesh->SetVisibility(false);
	InstancedStaticMesh->ClearInstances();

	// 2. Setup Material
	if (GhostMaterialBase && !SharedMaterialInstance)
	{
		SharedMaterialInstance = UMaterialInstanceDynamic::Create(GhostMaterialBase, this);
	}

	// 3. Setup Skeletal Instances
	EnsurePoseableComponents(Count);

	for (int32 i = 0; i < PoseableMeshes.Num(); ++i)
	{
		UPoseableMeshComponent* P = PoseableMeshes[i];
		if (!P)
			continue;

		if (i < Count)
		{
			P->SetVisibility(true);
			P->SetWorldScale3D(Scale);
            
			if (P->GetSkinnedAsset() != Mesh)
			{
				P->SetSkinnedAssetAndUpdate(Mesh);
			}

			if (SharedMaterialInstance)
			{
				P->SetOverlayMaterial(SharedMaterialInstance);
			}
		}
		else
		{
			P->SetVisibility(false);
		}
	}
}
// ---------------------------------------------------------
// SHOW PRESET
// ---------------------------------------------------------

void APreviewPoseMesh::ShowPreset(const FPlacementPreset& Preset)
{
	if (!Preset.IsValid())
	{
		HidePreview();
		return;
	}

	bUsingStaticMesh = true; // Use ISM logic
	CurrentScale = FVector::OneVector; // Preset handles scale internally via RelativeTransform? Or we can apply global scale.
	SetActorHiddenInGame(false);

	// 1. Clear previous state
	// Clear default ISM
	InstancedStaticMesh->SetVisibility(false);
	InstancedStaticMesh->ClearInstances();

	// Clear Poseables
	for (auto& P : PoseableMeshes)
	{
		if (P) P->SetVisibility(false);
	}

	// Reset Map Visibility (we will enable used ones)
	for (auto& Elem : StaticMeshComponents)
	{
		if (Elem.Value)
		{
			Elem.Value->ClearInstances();
			Elem.Value->SetVisibility(false);
		}
	}

	// 2. Setup Material
	if (GhostMaterialBase && !SharedMaterialInstance)
	{
		SharedMaterialInstance = UMaterialInstanceDynamic::Create(GhostMaterialBase, this);
	}

	// 3. Process Entries
    int32 NextPoseableIndex = 0;
    const int32 MAX_SKELETAL_PREVIEW = 50;
    
    UE_LOG(LogTemp, Log, TEXT("ShowPreset: %d entries"), Preset.Entries.Num());

	for (const FPresetActorEntry& Entry : Preset.Entries)
	{
		if (!Entry.ActorClass) 
        {
             UE_LOG(LogTemp, Warning, TEXT("ShowPreset: Entry has NULL ActorClass"));
             continue;
        }

		// We need the MESH from the ActorClass.
		// We have to inspect the CDO.
		AActor* CDO = Entry.ActorClass->GetDefaultObject<AActor>();
		if (!CDO) 
        {
             UE_LOG(LogTemp, Warning, TEXT("ShowPreset: Failed to get CDO for %s"), *Entry.ActorClass->GetName());
             continue;
        }


        // --- 1. Try Static Mesh ---
		UStaticMesh* MeshToUse = nullptr;

		TArray<UStaticMeshComponent*> StaticComps;
		CDO->GetComponents(StaticComps);
		
		for (UStaticMeshComponent* SMC : StaticComps)
		{
			if (SMC && SMC->GetStaticMesh())
			{
				MeshToUse = SMC->GetStaticMesh();
				break; // Found one
			}
		}

		if (MeshToUse) 
        {
             // UE_LOG(LogTemp, Log, TEXT("ShowPreset: Found StaticMesh %s for %s"), *MeshToUse->GetName(), *Entry.ActorClass->GetName());
    
             // Get or Create ISM for this Mesh
             UInstancedStaticMeshComponent* ISM = nullptr;
             if (StaticMeshComponents.Contains(MeshToUse))
             {
                 ISM = StaticMeshComponents[MeshToUse];
             }
             else
             {
                 // Create new ISM
                 ISM = NewObject<UInstancedStaticMeshComponent>(this);
                 ISM->SetupAttachment(Root);
                 ISM->RegisterComponent();
                 ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                 ISM->SetCastShadow(false);
                 ISM->SetStaticMesh(MeshToUse);
                 
                 // Apply Material based on status
                 if (SharedMaterialInstance)
                 {
                     int32 MatCount = MeshToUse->GetStaticMaterials().Num();
                     for (int32 i = 0; i < MatCount; i++)
                     {
                         ISM->SetOverlayMaterial(SharedMaterialInstance);
                     }
                 }
    
                 StaticMeshComponents.Add(MeshToUse, ISM);
             }
    
             // Add Instance
             if (ISM)
             {
                 ISM->SetVisibility(true);
                 ISM->AddInstance(Entry.RelativeTransform, false);
             }
             
             continue; // Done with this entry
        }

        // --- 2. Try Skeletal Mesh ---
        USkeletalMesh* SkelMeshToUse = nullptr;
        TArray<USkeletalMeshComponent*> SkelComps;
        CDO->GetComponents(SkelComps);

        for (USkeletalMeshComponent* SKC : SkelComps)
        {
            if (SKC && SKC->GetSkeletalMeshAsset())
            {
                SkelMeshToUse = SKC->GetSkeletalMeshAsset();
                break;
            }
        }

        if (SkelMeshToUse)
        {
            if (NextPoseableIndex >= MAX_SKELETAL_PREVIEW)
            {
                // Cap reached, skip rendering this skeletal mesh
                continue;
            }

            // Need a PoseableMeshComponent for this specific instance
            EnsurePoseableComponents(NextPoseableIndex + 1);
            
            if (PoseableMeshes.IsValidIndex(NextPoseableIndex))
            {
                UPoseableMeshComponent* P = PoseableMeshes[NextPoseableIndex];
                if (P)
                {
                    P->SetVisibility(true);
                    // Reset transform before setting new one? 
                    // SetRelativeTransform does it.
                    P->SetRelativeTransform(Entry.RelativeTransform);
                    // P->SetRelativeScale3D(Entry.RelativeTransform.GetScale3D()); // Already in Transform? Yes.
                    
                    if (P->GetSkinnedAsset() != SkelMeshToUse)
                    {
                        P->SetSkinnedAssetAndUpdate(SkelMeshToUse);
                    }
                    
                    if (SharedMaterialInstance)
                    {
                         P->SetOverlayMaterial(SharedMaterialInstance);
                    }
                }
                NextPoseableIndex++;
            }
            continue;
        }

        UE_LOG(LogTemp, Warning, TEXT("ShowPreset: No Static or Skeletal Mesh found in CDO for %s"), *Entry.ActorClass->GetName());

	}
}

// ---------------------------------------------------------
// UPDATE LOGIC
// ---------------------------------------------------------

void APreviewPoseMesh::UpdateInstances(const TArray<FTransform>& Transforms)
{
    if (bUsingStaticMesh)
    {
        int32 CurrentCount = InstancedStaticMesh->GetInstanceCount();
        int32 NewCount = Transforms.Num();

        if (NewCount == 0)
        {
            InstancedStaticMesh->ClearInstances();
            return;
        }

        // Cas A : Le nombre a changé -> On est obligé de rebuild
        if (CurrentCount != NewCount)
        {
            InstancedStaticMesh->ClearInstances();
            for (const FTransform& T : Transforms)
            {
                FTransform FinalT = T;
                FinalT.SetScale3D(CurrentScale);
            	InstancedStaticMesh->AddInstance(FinalT, false);
            }
        }
        // Cas B : Le nombre est identique -> Update rapide (Batch)
        else
        {
            TArray<FTransform> ScaledTransforms;
            ScaledTransforms.Reserve(NewCount);
            for(const FTransform& T : Transforms)
            {
                FTransform FinalT = T;
                FinalT.SetScale3D(CurrentScale);
                ScaledTransforms.Add(FinalT);
            }
            
			InstancedStaticMesh->BatchUpdateInstancesTransforms(0, ScaledTransforms, false, true);
        }
    }
    else
    {
        int32 Count = Transforms.Num();
        EnsurePoseableComponents(Count);

        for (int32 i = 0; i < Count; ++i)
        {
            if (PoseableMeshes.IsValidIndex(i))
            {
                const FTransform& T = Transforms[i];
            	PoseableMeshes[i]->SetRelativeTransform(T); 
            	PoseableMeshes[i]->SetRelativeScale3D(CurrentScale);
            	PoseableMeshes[i]->SetVisibility(true);
            }
        }
        
        // Hide unused
        for (int32 i = Count; i < PoseableMeshes.Num(); ++i)
        {
            PoseableMeshes[i]->SetVisibility(false);
        }
    }
}

// ---------------------------------------------------------
// FEEDBACK VISUEL
// ---------------------------------------------------------

void APreviewPoseMesh::SetPlacementValid(bool bValid)
{
    if (bIsPlacementValid == bValid)
    	return;

    bIsPlacementValid = bValid;
    UpdateMaterialsParameter();
}

void APreviewPoseMesh::UpdateMaterialsParameter()
{
    if (!SharedMaterialInstance)
    	return;

	// 1.0 = Valide (Vert),
    // 0.0 = Invalide (Rouge)
    float Value = bIsPlacementValid ? 1.0f : 0.0f;
    SharedMaterialInstance->SetScalarParameterValue(StatusParamName, Value);
}

// ---------------------------------------------------------
// HELPERS
// ---------------------------------------------------------

void APreviewPoseMesh::HidePreview()
{
    SetActorHiddenInGame(true);
	
	// Clear Map
	for (auto& Elem : StaticMeshComponents)
	{
		if (Elem.Value)
		{
			Elem.Value->ClearInstances();
			Elem.Value->SetVisibility(false);
		}
	}
}

void APreviewPoseMesh::EnsurePoseableComponents(int32 Count)
{
    if (PoseableMeshes.Num() >= Count)
    	return;

    int32 ToCreate = Count - PoseableMeshes.Num();
    for (int32 i = 0; i < ToCreate; ++i)
    {
        UPoseableMeshComponent* P = NewObject<UPoseableMeshComponent>(this);
        if (P)
        {
            P->SetupAttachment(Root);
            P->RegisterComponent();
            P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            P->SetCastShadow(false);
            PoseableMeshes.Add(P);
        }
    }
}