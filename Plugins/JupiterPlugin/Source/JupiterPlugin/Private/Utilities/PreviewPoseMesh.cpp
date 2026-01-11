#include "Utilities/PreviewPoseMesh.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
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