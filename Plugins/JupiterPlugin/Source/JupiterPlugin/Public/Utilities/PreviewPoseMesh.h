#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PreviewPoseMesh.generated.h"

class UInstancedStaticMeshComponent;
class UPoseableMeshComponent;


UCLASS()
class JUPITERPLUGIN_API APreviewPoseMesh : public AActor
{
	GENERATED_BODY()

public:
	APreviewPoseMesh();

	// --- Show Methods ---
	UFUNCTION()
	void ShowPreview(UStaticMesh* NewStaticMesh, FVector NewScale, int32 InstanceCount);
	void ShowPreview(USkeletalMesh* NewSkeletalMesh, FVector NewScale, int32 InstanceCount);

	UFUNCTION()
	void HidePreview();

	// --- Update Methods ---
	UFUNCTION()
	void UpdateInstances(const TArray<FTransform>& InstanceTransforms);

	// --- Visual Feedback ---
	UFUNCTION()
	void SetPlacementValid(bool bValid);

protected:
	void EnsurePoseableComponents(int32 DesiredCount);
	void UpdateMaterialsParameter();

protected:
	UPROPERTY(EditAnywhere, Category = "Settings|Material")
	UMaterialInterface* GhostMaterialBase;

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Material")
	FName StatusParamName = "Status";

	UPROPERTY()
	UMaterialInstanceDynamic* SharedMaterialInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UInstancedStaticMeshComponent* InstancedStaticMesh;

	UPROPERTY()
	TArray<TObjectPtr<UPoseableMeshComponent>> PoseableMeshes;

	UPROPERTY()
	FVector CurrentScale = FVector::OneVector;

	UPROPERTY()
	bool bUsingStaticMesh = false;

	UPROPERTY()
	bool bIsPlacementValid = true;
};