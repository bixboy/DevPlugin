#include "Utilities/PreviewPoseMesh.h"

#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "CollisionShape.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

APreviewPoseMesh::APreviewPoseMesh()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(Root);
	
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(Root);
	
	bReplicates = false;
}

void APreviewPoseMesh::ShowPreview(UStaticMesh* NewStaticMesh, FVector NewScale)
{
	SetActorHiddenInGame(false);
	
	if (SkeletalMesh->GetSkeletalMeshAsset())
		SkeletalMesh->SetSkeletalMesh(nullptr);

	StaticMesh->SetStaticMesh(NewStaticMesh);
	StaticMesh->SetRelativeScale3D(NewScale);
	
	if (HighlightMaterial)
	{
		MaterialInstance = UMaterialInstanceDynamic::Create(HighlightMaterial, this);
		StaticMesh->SetOverlayMaterial(MaterialInstance);
	}
}

void APreviewPoseMesh::ShowPreview(USkeletalMesh* NewSkeletalMesh, FVector NewScale)
{
	SetActorHiddenInGame(false);
	
	if (StaticMesh->GetStaticMesh())
		StaticMesh->SetStaticMesh(nullptr);

	SkeletalMesh->SetSkeletalMesh(NewSkeletalMesh);
	SkeletalMesh->SetRelativeScale3D(NewScale);

	if (HighlightMaterial)
	{
		MaterialInstance = UMaterialInstanceDynamic::Create(HighlightMaterial, this);
		SkeletalMesh->SetOverlayMaterial(MaterialInstance);
	}
}

void APreviewPoseMesh::HidePreview()
{
	if (StaticMesh->GetStaticMesh())
		StaticMesh->SetStaticMesh(nullptr);

	if (SkeletalMesh->GetSkeletalMeshAsset())
		SkeletalMesh->SetSkeletalMesh(nullptr);

	SetActorHiddenInGame(true);
}

void APreviewPoseMesh::CheckIsValidPlacement()
{
	if (GetIsValidPlacement())
	{
		if (MaterialInstance)
			MaterialInstance->SetScalarParameterValue("Status", 1.f);
	}
	else
	{
		if (MaterialInstance)
			MaterialInstance->SetScalarParameterValue("Status", 0.f);
	}
}

bool APreviewPoseMesh::GetIsValidPlacement()
{

	return true;
	
	// FBoxSphereBounds Bounds;
	// FVector Center;
	// FVector Extent;
	//
	// // Si c'est un StaticMesh
	// if (StaticMesh && StaticMesh->GetStaticMesh())
	// {
	// 	Bounds = StaticMesh->CalcBounds(StaticMesh->GetComponentTransform());
	// 	Center = Bounds.Origin;
	// 	Extent = Bounds.BoxExtent;
	//
	// 	FCollisionQueryParams QueryParams;
	// 	QueryParams.AddIgnoredActor(this);
	//
	// 	bool bOverlapping = GetWorld()->OverlapAnyTestByChannel(
	// 		Center,
	// 		FQuat::Identity,
	// 		ECC_WorldStatic,
	// 		FCollisionShape::MakeBox(Extent),
	// 		QueryParams
	// 	);
 //        
	// 	return !bOverlapping;
	// }
	// // Sinon, si c'est un SkeletalMesh
	// else if (SkeletalMesh && SkeletalMesh->GetSkeletalMeshAsset())
	// {
	// 	Bounds = SkeletalMesh->CalcBounds(SkeletalMesh->GetComponentTransform());
	// 	Center = Bounds.Origin;
	// 	Extent = Bounds.BoxExtent;
	//
	// 	FCollisionQueryParams QueryParams;
	// 	QueryParams.AddIgnoredActor(this);
	//
	// 	bool bOverlapping = GetWorld()->OverlapAnyTestByChannel(
	// 		Center,
	// 		FQuat::Identity,
	// 		ECC_WorldStatic,
	// 		FCollisionShape::MakeBox(Extent),
	// 		QueryParams
	// 	);
	// 	return !bOverlapping;
	// }
	//
	// return false;
}

