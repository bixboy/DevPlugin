#include "Player/JupiterPlayerSystem/CameraPreviewSystem.h"
#include "Player/PlayerCamera.h"
#include "Utilities/PreviewPoseMesh.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Player/JupiterPlayerSystem/CameraSpawnSystem.h"

// ------------------------------------------
// INIT
// ------------------------------------------
void UCameraPreviewSystem::Init(APlayerCamera* InOwner)
{
	Super::Init(InOwner);
}

// ------------------------------------------
// TICK
// ------------------------------------------
void UCameraPreviewSystem::Tick(float DeltaTime)
{
	// (Optional interpolation)
}

// ------------------------------------------
// PREVIEW CORE
// ------------------------------------------
bool UCameraPreviewSystem::EnsurePreviewActor()
{
	if (PreviewActor)
		return true;

	if (!SpawnSystem || !GetOwner())
		return false;

	TSubclassOf<APreviewPoseMesh> PreviewClass = GetOwner()->PreviewMeshClass;
	if (!PreviewClass)
		return false;

	return InternalCreatePreviewActor(PreviewClass);
}

bool UCameraPreviewSystem::InternalCreatePreviewActor(TSubclassOf<APreviewPoseMesh> PreviewClass)
{
	if (!GetWorldSafe())
		return false;

	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.Instigator = GetOwner();

	PreviewActor = GetWorldSafe()->SpawnActor<APreviewPoseMesh>(PreviewClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);
	return PreviewActor != nullptr;
}

// ------------------------------------------
// PREVIEW SHOW / HIDE
// ------------------------------------------
void UCameraPreviewSystem::HidePreview()
{
	if (PreviewActor)
		PreviewActor->SetActorHiddenInGame(true);

	bPreviewVisible = false;
}

// ------------------------------------------
// MESH PREVIEW
// ------------------------------------------
bool UCameraPreviewSystem::ShowSkeletalPreview(USkeletalMesh* Mesh, const FVector& Scale, int32 Count)
{
	if (!EnsurePreviewActor() || !Mesh)
		return false;

	PreviewActor->ShowPreview(Mesh, Scale, Count);
	PreviewActor->SetActorHiddenInGame(false);
	bPreviewVisible = true;
	return true;
}

bool UCameraPreviewSystem::ShowStaticPreview(UStaticMesh* Mesh, const FVector& Scale, int32 Count)
{
	if (!EnsurePreviewActor() || !Mesh)
		return false;

	PreviewActor->ShowPreview(Mesh, Scale, Count);
	bPreviewVisible = true;
	return true;
}


// ------------------------------------------
// INSTANCE TRANSFORMS
// ------------------------------------------
void UCameraPreviewSystem::SetupInstances(const TArray<FTransform>& BaseTransforms)
{
	if (!PreviewActor)
		return;

	CachedTransforms = BaseTransforms;
	PreviewActor->UpdateInstances(CachedTransforms);
}

void UCameraPreviewSystem::UpdateInstances(const TArray<FTransform>& Transforms)
{
	if (PreviewActor)
		PreviewActor->UpdateInstances(Transforms);
}
