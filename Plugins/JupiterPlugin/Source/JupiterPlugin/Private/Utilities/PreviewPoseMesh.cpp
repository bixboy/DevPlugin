#include "Utilities/PreviewPoseMesh.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"


APreviewPoseMesh::APreviewPoseMesh()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    InstancedStaticMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("InstancedStaticMesh"));
    InstancedStaticMesh->SetupAttachment(Root);
    InstancedStaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    InstancedStaticMesh->SetVisibility(false);
    InstancedStaticMesh->SetCanEverAffectNavigation(false);

    bReplicates = false;
}

/* ---------------------------------------------------------
                SHOW PREVIEWS
--------------------------------------------------------- */

void APreviewPoseMesh::ShowPreview(UStaticMesh* Mesh, FVector Scale, int32 Count)
{
    if (!Mesh || Count <= 0)
    {
        HidePreview();
        return;
    }

    bUsingStaticMesh = true;
    CurrentScale     = Scale;

    SetActorHiddenInGame(false);

    for (UPoseableMeshComponent* P : PoseableMeshes)
    {
        if (P)
        {
            P->SetVisibility(false);
            P->SetSkinnedAssetAndUpdate(nullptr);
        }
    }
    SkeletalMaterialInstances.Reset();

    // Setup static mesh
    InstancedStaticMesh->SetStaticMesh(Mesh);
    InstancedStaticMesh->ClearInstances();
    InstancedStaticMesh->SetVisibility(true);

    // Material
    StaticMaterialInstance = HighlightMaterial ? UMaterialInstanceDynamic::Create(HighlightMaterial, this) : nullptr;

    InstancedStaticMesh->SetOverlayMaterial(StaticMaterialInstance);

    bLastPlacementValid = true;
}

void APreviewPoseMesh::ShowPreview(USkeletalMesh* Mesh, FVector Scale, int32 Count)
{
    if (!Mesh || Count <= 0)
    {
        HidePreview();
        return;
    }

    bUsingStaticMesh = false;
    CurrentScale     = Scale;

    SetActorHiddenInGame(false);

    // Clear static mesh
    InstancedStaticMesh->ClearInstances();
    InstancedStaticMesh->SetStaticMesh(nullptr);
    InstancedStaticMesh->SetVisibility(false);
    StaticMaterialInstance = nullptr;

    EnsurePoseableComponents(Count);

    SkeletalMaterialInstances.SetNum(Count);

    for (int32 i = 0; i < PoseableMeshes.Num(); ++i)
    {
        UPoseableMeshComponent* P = PoseableMeshes[i];
        if (!P)
            continue;

        if (i < Count)
        {
            P->SetVisibility(true);
            P->SetWorldScale3D(Scale);
            P->SetSkinnedAssetAndUpdate(Mesh);

            if (HighlightMaterial)
            {
                if (!SkeletalMaterialInstances[i])
                {
                    SkeletalMaterialInstances[i] = UMaterialInstanceDynamic::Create(HighlightMaterial, this);
                }
                P->SetOverlayMaterial(SkeletalMaterialInstances[i]);
            }
        }
        else
        {
            P->SetVisibility(false);
            P->SetSkinnedAssetAndUpdate(nullptr);
        }
    }

    bLastPlacementValid = true;
}

/* ---------------------------------------------------------
                        UPDATE INSTANCES
--------------------------------------------------------- */

void APreviewPoseMesh::UpdateInstances(const TArray<FTransform>& Transforms)
{
    if (bUsingStaticMesh)
    {
        InstancedStaticMesh->ClearInstances();

        for (const FTransform& T : Transforms)
        {
            FTransform I = T;
            I.SetScale3D(CurrentScale);
            InstancedStaticMesh->AddInstance(I);
        }

        InstancedStaticMesh->SetVisibility(Transforms.Num() > 0);
    }
    else
    {
        EnsurePoseableComponents(Transforms.Num());
        SkeletalMaterialInstances.SetNum(Transforms.Num());

        for (int32 i = 0; i < PoseableMeshes.Num(); ++i)
        {
            UPoseableMeshComponent* P = PoseableMeshes[i];
            if (!P)
                continue;

            if (Transforms.IsValidIndex(i))
            {
                const FTransform& T = Transforms[i];
                P->SetWorldTransform(FTransform(T.GetRotation(), T.GetLocation(), CurrentScale));
                P->SetVisibility(true);

                if (HighlightMaterial)
                {
                    if (!SkeletalMaterialInstances[i])
                        SkeletalMaterialInstances[i] = UMaterialInstanceDynamic::Create(HighlightMaterial, this);

                    P->SetOverlayMaterial(SkeletalMaterialInstances[i]);
                }
            }
            else
            {
                P->SetVisibility(false);
            }
        }
    }

    CheckIsValidPlacement();
}

/* ---------------------------------------------------------
                    HIDE PREVIEW
--------------------------------------------------------- */

void APreviewPoseMesh::HidePreview()
{
    // Static
    InstancedStaticMesh->ClearInstances();
    InstancedStaticMesh->SetStaticMesh(nullptr);
    InstancedStaticMesh->SetVisibility(false);

    // Skeletal
    for (UPoseableMeshComponent* P : PoseableMeshes)
    {
        if (P)
        {
            P->SetVisibility(false);
            P->SetSkinnedAssetAndUpdate(nullptr);
        }
    }

    SkeletalMaterialInstances.Reset();
    StaticMaterialInstance = nullptr;

    bLastPlacementValid = true;
    SetActorHiddenInGame(true);
}

void APreviewPoseMesh::CheckIsValidPlacement()
{
    const float Status = bLastPlacementValid ? 1.f : 0.f;

    if (StaticMaterialInstance)
        StaticMaterialInstance->SetScalarParameterValue(TEXT("Status"), Status);

    for (UMaterialInstanceDynamic* M : SkeletalMaterialInstances)
    {
        if (M)
            M->SetScalarParameterValue(TEXT("Status"), Status);
    }
}

bool APreviewPoseMesh::GetIsValidPlacement() const
{
    return bLastPlacementValid;
}

void APreviewPoseMesh::EnsurePoseableComponents(int32 Count)
{
    Count = FMath::Max(0, Count);

    while (PoseableMeshes.Num() < Count)
    {
        const FName Name = *FString::Printf(TEXT("PoseableMesh_%d"), PoseableMeshes.Num());
        UPoseableMeshComponent* P = NewObject<UPoseableMeshComponent>(this, Name);

        if (!P)
            break;

        P->SetupAttachment(Root);
        P->RegisterComponent();
        P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        P->SetVisibility(false);

        PoseableMeshes.Add(P);
    }

    // Hide extras
    for (int32 i = Count; i < PoseableMeshes.Num(); ++i)
    {
	    if (UPoseableMeshComponent* P = PoseableMeshes[i])
        {
            P->SetVisibility(false);
            P->SetSkinnedAssetAndUpdate(nullptr);
        }
    }
}
