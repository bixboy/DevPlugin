#pragma once
#include "CoreMinimal.h"
#include "Component/GridPlacementComponent.h"
#include "GameFramework/Actor.h"
#include "Container.generated.h"

class UObjectPreviewComponent;
class UBoxComponent;
class UGridPlacementComponent;


UCLASS()
class STELLARLOCOMOTIONVEHICLESMODULE_API AContainer : public AActor
{
	GENERATED_BODY()

public:
	AContainer();

	UFUNCTION()
	void NotifyEnterGrid(UGridPlacementComponent* Grid);

	UFUNCTION()
	void NotifyExitGrid(UGridPlacementComponent* Grid);

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY()
	APlayerController* PC = nullptr;

	UPROPERTY()
	UObjectPreviewComponent* PreviewComp;
	

	UPROPERTY(VisibleAnywhere)
	UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ContainerMesh;

	UPROPERTY()
	TSet<UGridPlacementComponent*> ActiveGrids;

	UPROPERTY()
	FGridCell PreviousCell = {INT32_MIN, INT32_MIN, INT32_MIN};
};
