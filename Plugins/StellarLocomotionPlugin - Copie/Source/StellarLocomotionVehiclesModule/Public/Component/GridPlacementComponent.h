#pragma once
#include "CoreMinimal.h"
#include "GridPlacementComponent.generated.h"

class UProceduralMeshComponent;

USTRUCT(BlueprintType)
struct FGridCell
{
	GENERATED_BODY()
	int32 X, Y, Z;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class STELLARLOCOMOTIONVEHICLESMODULE_API UGridPlacementComponent : public USceneComponent
{
	GENERATED_BODY()
	
public:
	UGridPlacementComponent();

	UFUNCTION()
	FVector SnapWorldPosition(const FVector& InWorldPos) const;

	UFUNCTION()
	FVector GetGridOrigin() const;

	UFUNCTION()
	FGridCell WorldPosToCell(const FVector& InWorldPos) const;

	UFUNCTION()
	FVector CellToWorldPos(int32 X, int32 Y, int32 Z) const;

	UFUNCTION()
	FVector2D GetGridCount() const;

	UFUNCTION()
	float GetGridHeight() const;

	UFUNCTION()
	void OnTriggerBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnTriggerEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

protected:

#if WITH_EDITOR
	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	
	virtual void OnRegister() override;

	virtual void BeginPlay() override;

	UFUNCTION()
	void BuildGridMesh();
	
#endif

private:

	FVector GetBottomLeft() const
	{
		const float HalfX = GridCountX * CellSize * 0.5f;
		const float HalfY = GridCountY * CellSize * 0.5f;
		FVector Center = GetComponentLocation();
		return Center - FVector(HalfX, HalfY, 0);
	}
	
	UPROPERTY(EditAnywhere, Category="Grid")
	float CellSize = 100.f;

	UPROPERTY(EditAnywhere, Category="Grid")
	int32 GridCountX = 10;

	UPROPERTY(EditAnywhere, Category="Grid")
	int32 GridCountY = 10;

	UPROPERTY(EditAnywhere, Category="Grid")
	float GridHeight = 0.f;

	UPROPERTY(EditAnywhere, Category="Grid")
	float LineThickness = 5.f;

	UPROPERTY(EditAnywhere, Category="Grid")
	UMaterialInterface* LineMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category="Grid")
	bool bShowGrid = true;

	UPROPERTY(EditAnywhere, Category="Grid")
	bool bShowGridInGame = true;

	UPROPERTY(EditAnywhere, Category="Grid")
	bool bSnapOnMove = true;

	UPROPERTY(EditAnywhere, Category="Grid")
	bool bEnableRuntimeTrigger = true;

	UPROPERTY()
	UProceduralMeshComponent* ProcMesh;

	UPROPERTY()
	class UBoxComponent* TriggerVolume;

};
