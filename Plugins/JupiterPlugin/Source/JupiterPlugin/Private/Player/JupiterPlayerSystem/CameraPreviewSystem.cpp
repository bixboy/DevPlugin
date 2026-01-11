#include "Player/JupiterPlayerSystem/CameraPreviewSystem.h"
#include "Player/PlayerCamera.h"
#include "Utilities/PreviewPoseMesh.h"
#include "Engine/World.h"


void UCameraPreviewSystem::Init(APlayerCamera* InOwner)
{
    Super::Init(InOwner);
}

void UCameraPreviewSystem::Cleanup()
{
    if (PreviewActor)
    {
        PreviewActor->Destroy();
        PreviewActor = nullptr;
    }
	
    bPreviewVisible = false;
}

void UCameraPreviewSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bPreviewVisible && PreviewActor)
	{
		bool bIsValid = IsPlacementValid();
		PreviewActor->SetPlacementValid(bIsValid);
	}
}

// ------------------------------------------
// PREVIEW CORE
// ------------------------------------------

bool UCameraPreviewSystem::EnsurePreviewActor()
{
    if (PreviewActor)
    	return true;

    if (!GetOwner())
    {
        UE_LOG(LogTemp, Error, TEXT("EnsurePreviewActor failed: Owner is NULL"));
        return false;
    }

    TSubclassOf<APreviewPoseMesh> PreviewClass = GetOwner()->GetPreviewMeshClass();
    if (!PreviewClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("EnsurePreviewActor failed: PreviewMeshClass not set in PlayerCamera"));
        return false;
    }
    
    return InternalCreatePreviewActor(PreviewClass);
}

bool UCameraPreviewSystem::InternalCreatePreviewActor(TSubclassOf<APreviewPoseMesh> PreviewClass)
{
    if (!GetWorldSafe())
    	return false;

    FActorSpawnParameters Params;
    Params.Owner = GetOwner();
    Params.Instigator = GetOwner();
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    PreviewActor = GetWorldSafe()->SpawnActor<APreviewPoseMesh>(PreviewClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);
    
    if (PreviewActor)
    {
        PreviewActor->SetActorHiddenInGame(true);
        return true;
    }
    
    return false;
}

// ------------------------------------------
// PREVIEW SHOW / HIDE
// ------------------------------------------

void UCameraPreviewSystem::HidePreview()
{
    if (PreviewActor)
    {
        PreviewActor->SetActorHiddenInGame(true);
        // Vider les instances pour libérer la mémoire GPU si masqué longtemps
        // PreviewActor->ClearInstances(); 
    }
	
    bPreviewVisible = false;
}

bool UCameraPreviewSystem::IsPlacementValid() const
{
	if (!bPreviewVisible || !PreviewActor)
		return false;

    UWorld* World = GetWorldSafe();
    if (!World)
    	return false;

    FVector CenterLocation = PreviewActor->GetActorLocation();

    // --- CHECK : PENTE ---
    FHitResult Hit;
    FVector TraceStart = CenterLocation + FVector(0, 0, 500);
    FVector TraceEnd = CenterLocation - FVector(0, 0, 500);
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(PreviewActor);

    if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
    {
        if (Hit.ImpactNormal.Z < 0.7f) 
        {
            // UE_LOG(LogTemp, Warning, TEXT("PLACEMENT INVALID: Pente trop raide"));
            return false;
        }
    }
    else
    {
        // UE_LOG(LogTemp, Warning, TEXT("PLACEMENT INVALID: Pas de sol trouvé sous l'unité"));
        return false;
    }

    // --- CHECK : COLLISION ---
    FVector SphereCheckCenter = CenterLocation + FVector(0, 0, 50.0f); 

    bool bOverlap = World->OverlapBlockingTestByChannel(
        SphereCheckCenter,
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(40.f),
        Params
    );

    if (bOverlap)
    {
        // UE_LOG(LogTemp, Warning, TEXT("PLACEMENT INVALID: Collision détectée (Unité ou Obstacle)"));
    	DrawDebugSphere(World, SphereCheckCenter, 40.f, 12, FColor::Red, false, 0.1f);
    	
        return false;
    }

    return true;
}

// ------------------------------------------
// MESH PREVIEW LOGIC
// ------------------------------------------

bool UCameraPreviewSystem::ShowSkeletalPreview(USkeletalMesh* Mesh, const FVector& Scale, int32 Count)
{
    if (!Mesh || !EnsurePreviewActor())
    	return false;

    PreviewActor->ShowPreview(Mesh, Scale, Count);
    PreviewActor->SetActorHiddenInGame(false);
    
    bPreviewVisible = true;
    return true;
}

bool UCameraPreviewSystem::ShowStaticPreview(UStaticMesh* Mesh, const FVector& Scale, int32 Count)
{
    if (!Mesh || !EnsurePreviewActor())
    	return false;

    PreviewActor->ShowPreview(Mesh, Scale, Count);
    PreviewActor->SetActorHiddenInGame(false);

    bPreviewVisible = true;
    return true;
}

// ------------------------------------------
// INSTANCE TRANSFORMS
// ------------------------------------------

void UCameraPreviewSystem::UpdateInstances(const TArray<FTransform>& Transforms)
{
    if (PreviewActor)
    {
        PreviewActor->UpdateInstances(Transforms);
        
        if (Transforms.Num() > 0 && PreviewActor->IsHidden())
        {
            PreviewActor->SetActorHiddenInGame(false);
            bPreviewVisible = true;
        }
    }
}

void UCameraPreviewSystem::SetPreviewTransform(const FVector& Location, const FRotator& Rotation)
{
	if (PreviewActor)
	{
		PreviewActor->SetActorLocationAndRotation(Location, Rotation);
	}
}
