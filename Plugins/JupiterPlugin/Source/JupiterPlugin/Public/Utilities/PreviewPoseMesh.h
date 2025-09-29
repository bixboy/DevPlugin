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

        UFUNCTION()
        void ShowPreview(UStaticMesh* NewStaticMesh, FVector NewScale, int32 InstanceCount);
        void ShowPreview(USkeletalMesh* NewSkeletalMesh, FVector NewScale, int32 InstanceCount);

        UFUNCTION()
        void UpdateInstances(const TArray<FTransform>& InstanceTransforms);

        UFUNCTION()
        void HidePreview();

        UFUNCTION()
        void CheckIsValidPlacement();

        UFUNCTION()
        bool GetIsValidPlacement() const;

protected:

        void EnsurePoseableComponents(int32 DesiredCount);

        UPROPERTY(EditAnywhere, Category = "Settings|Material")
        UMaterialInstance* HighlightMaterial;

        UPROPERTY()
        UMaterialInstanceDynamic* StaticMaterialInstance;

        UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
        USceneComponent* Root;

        UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
        UInstancedStaticMeshComponent* InstancedStaticMesh;

        UPROPERTY()
        TArray<UPoseableMeshComponent*> PoseableMeshes;

        UPROPERTY()
        TArray<UMaterialInstanceDynamic*> SkeletalMaterialInstances;

        UPROPERTY()
        FVector CurrentScale = FVector::OneVector;

        UPROPERTY()
        bool bUsingStaticMesh = false;

        UPROPERTY()
        bool bLastPlacementValid = true;

};
