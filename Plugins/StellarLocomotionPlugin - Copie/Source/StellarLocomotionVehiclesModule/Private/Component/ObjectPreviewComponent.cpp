#include "Component/ObjectPreviewComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/GameplayStatics.h"


UObjectPreviewComponent::UObjectPreviewComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UObjectPreviewComponent::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = Cast<APlayerController>( UGameplayStatics::GetPlayerController(GetWorld(), 0));

	if (PC->IsNetMode(NM_DedicatedServer))
		return;

	FActorSpawnParameters Params;
	Params.Name = "PreviewActor";
	
	PreviewActor = GetWorld()->SpawnActor<AStaticMeshActor>(Params);

	if (PreviewActor)
	{
		PreviewActor->SetMobility(EComponentMobility::Movable);
		
		auto* MeshComp = PreviewActor->GetStaticMeshComponent();
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComp->SetCastShadow(false);
		MeshComp->SetVisibility(false);
	}
}

void UObjectPreviewComponent::ChangePreviewMesh(UStaticMesh* NewMesh)
{
	if (PreviewMesh == NewMesh || !PreviewActor)
		return;
	
	PreviewMesh = NewMesh;
	auto* MeshComp = PreviewActor->GetStaticMeshComponent();
	
	MeshComp->SetStaticMesh(PreviewMesh);

	bool bIsValid = PreviewMesh != nullptr;
	MeshComp->SetVisibility(bIsValid);
}

void UObjectPreviewComponent::SetPreviewTransform(FTransform NewTransform)
{
	if (PreviewActor)
		PreviewActor->SetActorTransform(NewTransform);
}
