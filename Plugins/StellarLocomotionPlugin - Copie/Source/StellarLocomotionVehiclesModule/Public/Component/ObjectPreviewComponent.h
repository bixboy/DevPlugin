#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ObjectPreviewComponent.generated.h"

class AStaticMeshActor;


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class STELLARLOCOMOTIONVEHICLESMODULE_API UObjectPreviewComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UObjectPreviewComponent();

	virtual void BeginPlay() override;

	UFUNCTION()
	void ChangePreviewMesh(UStaticMesh* NewMesh);

	UFUNCTION()
	void SetPreviewTransform(FTransform NewTransform);
	
private:

	UPROPERTY()
	AStaticMeshActor* PreviewActor = nullptr;
	
	UPROPERTY()
	UStaticMesh* PreviewMesh;
};
