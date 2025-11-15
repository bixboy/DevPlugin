#include "Player/CameraPreviewSystem.h"

#include "Player/PlayerCamera.h"
#include "Utilities/PreviewPoseMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void UCameraPreviewSystem::Init(APlayerCamera* InOwner)
{
    Owner = InOwner;
    if (!Owner.IsValid())
    {
        return;
    }

    Player = Owner->GetPlayerController();
    if (!PreviewActorClass)
    {
        PreviewActorClass = APreviewPoseMesh::StaticClass();
    }

    EnsurePreviewActor();
}

void UCameraPreviewSystem::Tick(float DeltaTime)
{
    if (PreviewActor)
    {
        PreviewActor->CheckIsValidPlacement();
    }
}

bool UCameraPreviewSystem::EnsurePreviewActor()
{
    if (PreviewActor)
    {
        return true;
    }

    SpawnPreviewActor();
    return PreviewActor != nullptr;
}

void UCameraPreviewSystem::SpawnPreviewActor()
{
    if (!Owner.IsValid())
    {
        return;
    }

    if (!PreviewActorClass)
    {
        return;
    }

    if (UWorld* World = Owner->GetWorld())
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = Owner.Get();
        SpawnParams.Instigator = Owner.Get();

        PreviewActor = World->SpawnActor<APreviewPoseMesh>(PreviewActorClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
        if (PreviewActor)
        {
            PreviewActor->SetOwner(Owner.Get());
            PreviewActor->SetReplicates(false);
            PreviewActor->SetActorHiddenInGame(true);
            bPreviewVisible = false;
        }
    }
}

bool UCameraPreviewSystem::ShowSkeletalPreview(USkeletalMesh* Mesh, const FVector& Scale, int32 InstanceCount)
{
    if (!EnsurePreviewActor() || !Mesh)
    {
        return false;
    }

    PreviewActor->ShowPreview(Mesh, Scale, InstanceCount);
    bPreviewVisible = true;
    return true;
}

bool UCameraPreviewSystem::ShowStaticPreview(UStaticMesh* Mesh, const FVector& Scale, int32 InstanceCount)
{
    if (!EnsurePreviewActor() || !Mesh)
    {
        return false;
    }

    PreviewActor->ShowPreview(Mesh, Scale, InstanceCount);
    bPreviewVisible = true;
    return true;
}

void UCameraPreviewSystem::UpdateInstances(const TArray<FTransform>& InstanceTransforms)
{
    if (PreviewActor)
    {
        PreviewActor->UpdateInstances(InstanceTransforms);
    }
}

void UCameraPreviewSystem::HidePreview()
{
    if (PreviewActor)
    {
        PreviewActor->HidePreview();
    }

    bPreviewVisible = false;
}

bool UCameraPreviewSystem::IsPlacementValid() const
{
    return PreviewActor ? PreviewActor->GetIsValidPlacement() : false;
}

