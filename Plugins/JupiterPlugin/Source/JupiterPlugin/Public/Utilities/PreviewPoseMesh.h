#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PreviewPoseMesh.generated.h"


UCLASS()
class JUPITERPLUGIN_API APreviewPoseMesh : public AActor
{
	GENERATED_BODY()

public:
	APreviewPoseMesh();
	
	UFUNCTION()
	void ShowPreview(UStaticMesh* NewStaticMesh, FVector NewScale);
	void ShowPreview(USkeletalMesh* NewSkeletalMesh, FVector NewScale);

	UFUNCTION()
	void HidePreview();

	UFUNCTION()
	void CheckIsValidPlacement();

	UFUNCTION()
	bool GetIsValidPlacement();

protected:

	UPROPERTY(EditAnywhere, Category = "Settings|Material")
	UMaterialInstance* HighlightMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* MaterialInstance;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
	UStaticMeshComponent* StaticMesh;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
	USkeletalMeshComponent* SkeletalMesh;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
	USceneComponent* Root;
};
