#include "Utilities/PreviewPoseMesh.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/EngineTypes.h"
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

void APreviewPoseMesh::ShowPreview(UStaticMesh* NewStaticMesh, FVector NewScale, int32 InstanceCount)
{
        if (!NewStaticMesh || InstanceCount <= 0)
        {
                HidePreview();
                return;
        }

        SetActorHiddenInGame(false);
        bUsingStaticMesh = true;
        CurrentScale = NewScale;

        InstancedStaticMesh->SetStaticMesh(NewStaticMesh);
        InstancedStaticMesh->SetVisibility(true);
        InstancedStaticMesh->ClearInstances();

        if (HighlightMaterial)
        {
                StaticMaterialInstance = UMaterialInstanceDynamic::Create(HighlightMaterial, this);
                InstancedStaticMesh->SetOverlayMaterial(StaticMaterialInstance);
        }
        else
        {
                StaticMaterialInstance = nullptr;
        }

        for (UPoseableMeshComponent* Poseable : PoseableMeshes)
        {
                if (Poseable)
                {
                        Poseable->SetVisibility(false);
                        Poseable->SetSkeletalMesh(nullptr);
                }
        }
        SkeletalMaterialInstances.Reset();

        bLastPlacementValid = true;
}

void APreviewPoseMesh::ShowPreview(USkeletalMesh* NewSkeletalMesh, FVector NewScale, int32 InstanceCount)
{
        if (!NewSkeletalMesh || InstanceCount <= 0)
        {
                HidePreview();
                return;
        }

        SetActorHiddenInGame(false);
        bUsingStaticMesh = false;
        CurrentScale = NewScale;

        InstancedStaticMesh->ClearInstances();
        InstancedStaticMesh->SetStaticMesh(nullptr);
        InstancedStaticMesh->SetVisibility(false);
        StaticMaterialInstance = nullptr;

        EnsurePoseableComponents(InstanceCount);

        if (SkeletalMaterialInstances.Num() < InstanceCount)
        {
                SkeletalMaterialInstances.SetNum(InstanceCount);
        }

        for (int32 Index = 0; Index < PoseableMeshes.Num(); ++Index)
        {
                UPoseableMeshComponent* Poseable = PoseableMeshes[Index];
                if (!Poseable)
                {
                        continue;
                }

                if (Index < InstanceCount)
                {
                        Poseable->SetSkeletalMesh(NewSkeletalMesh);
                        Poseable->SetWorldScale3D(NewScale);
                        Poseable->SetVisibility(true);

                        if (HighlightMaterial)
                        {
                                UMaterialInstanceDynamic* DynMaterial = SkeletalMaterialInstances[Index];
                                if (!DynMaterial)
                                {
                                        DynMaterial = UMaterialInstanceDynamic::Create(HighlightMaterial, this);
                                        SkeletalMaterialInstances[Index] = DynMaterial;
                                }
                                Poseable->SetOverlayMaterial(DynMaterial);
                        }
                }
                else
                {
                        Poseable->SetVisibility(false);
                        Poseable->SetSkeletalMesh(nullptr);
                        if (SkeletalMaterialInstances.IsValidIndex(Index))
                        {
                                SkeletalMaterialInstances[Index] = nullptr;
                        }
                }
        }

        if (SkeletalMaterialInstances.Num() > InstanceCount)
        {
                SkeletalMaterialInstances.SetNum(InstanceCount);
        }

        bLastPlacementValid = true;
}

void APreviewPoseMesh::UpdateInstances(const TArray<FTransform>& InstanceTransforms)
{

        if (bUsingStaticMesh)
        {
                InstancedStaticMesh->ClearInstances();
                for (const FTransform& Transform : InstanceTransforms)
                {
                        FTransform InstanceTransform = Transform;
                        InstanceTransform.SetScale3D(CurrentScale);
                        InstancedStaticMesh->AddInstance(InstanceTransform);
                }
                InstancedStaticMesh->SetVisibility(InstanceTransforms.Num() > 0);
        }
        else
        {
                EnsurePoseableComponents(InstanceTransforms.Num());
                if (SkeletalMaterialInstances.Num() < InstanceTransforms.Num())
                {
                        SkeletalMaterialInstances.SetNum(InstanceTransforms.Num());
                }

                for (int32 Index = 0; Index < PoseableMeshes.Num(); ++Index)
                {
                        UPoseableMeshComponent* Poseable = PoseableMeshes[Index];
                        if (!Poseable)
                        {
                                continue;
                        }

                        if (InstanceTransforms.IsValidIndex(Index))
                        {
                                const FTransform& Transform = InstanceTransforms[Index];
                                Poseable->SetWorldTransform(FTransform(Transform.GetRotation(), Transform.GetLocation(), CurrentScale));
                                Poseable->SetVisibility(true);

                                if (HighlightMaterial)
                                {
                                        UMaterialInstanceDynamic* DynMaterial = SkeletalMaterialInstances[Index];
                                        if (!DynMaterial)
                                        {
                                                DynMaterial = UMaterialInstanceDynamic::Create(HighlightMaterial, this);
                                                SkeletalMaterialInstances[Index] = DynMaterial;
                                        }
                                        Poseable->SetOverlayMaterial(DynMaterial);
                                }
                        }
                        else
                        {
                                Poseable->SetVisibility(false);
                        }
                }

                if (SkeletalMaterialInstances.Num() > InstanceTransforms.Num())
                {
                        SkeletalMaterialInstances.SetNum(InstanceTransforms.Num());
                }
        }

        CheckIsValidPlacement();
}

void APreviewPoseMesh::HidePreview()
{
        InstancedStaticMesh->ClearInstances();
        InstancedStaticMesh->SetStaticMesh(nullptr);
        InstancedStaticMesh->SetVisibility(false);

        for (UPoseableMeshComponent* Poseable : PoseableMeshes)
        {
                if (Poseable)
                {
                        Poseable->SetVisibility(false);
                        Poseable->SetSkeletalMesh(nullptr);
                }
        }

        SkeletalMaterialInstances.Reset();
        bLastPlacementValid = true;

        SetActorHiddenInGame(true);
}

void APreviewPoseMesh::CheckIsValidPlacement()
{
        bLastPlacementValid = true;

        const float Status = bLastPlacementValid ? 1.f : 0.f;

        if (StaticMaterialInstance)
        {
                StaticMaterialInstance->SetScalarParameterValue(TEXT("Status"), Status);
        }

        for (UMaterialInstanceDynamic* MaterialInstance : SkeletalMaterialInstances)
        {
                if (MaterialInstance)
                {
                        MaterialInstance->SetScalarParameterValue(TEXT("Status"), Status);
                }
        }
}

bool APreviewPoseMesh::GetIsValidPlacement() const
{
        return bLastPlacementValid;
}

void APreviewPoseMesh::EnsurePoseableComponents(int32 DesiredCount)
{
        DesiredCount = FMath::Max(0, DesiredCount);

        while (PoseableMeshes.Num() < DesiredCount)
        {
                const FName ComponentName = *FString::Printf(TEXT("PoseableMesh_%d"), PoseableMeshes.Num());
                UPoseableMeshComponent* Poseable = NewObject<UPoseableMeshComponent>(this, ComponentName);
                if (!Poseable)
                {
                        break;
                }

                Poseable->SetupAttachment(Root);
                Poseable->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                Poseable->RegisterComponent();
                Poseable->SetVisibility(false);
                PoseableMeshes.Add(Poseable);
        }

        for (int32 Index = 0; Index < PoseableMeshes.Num(); ++Index)
        {
                if (Index >= DesiredCount)
                {
                        if (UPoseableMeshComponent* Poseable = PoseableMeshes[Index])
                        {
                                Poseable->SetVisibility(false);
                                Poseable->SetSkeletalMesh(nullptr);
                        }
                }
        }
}
