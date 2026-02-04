#pragma once
#include "CoreMinimal.h"
#include "Player/JupiterPlayerSystem/CameraSystemBase.h"
#include "Player/PlayerCameraRotationPreview.h"
#include "Data/Placement/PlacementItemData.h"
#include "Data/Placement/PlacementTypes.h"
#include "CameraPlacementSystem.generated.h"

class UCameraPreviewSystem;
class UCameraCommandSystem;
class UPlacementHandlerComponent;


UCLASS()
class JUPITERPLUGIN_API UCameraPlacementSystem : public UCameraSystemBase
{
	GENERATED_BODY()

public:
	virtual void Init(APlayerCamera* InOwner) override;
	virtual void Tick(float DeltaTime) override;

	// --- Public API ---
    
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpawnCountChanged, int32, NewCount);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpawnFormationChanged, ESpawnFormation, NewFormation);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCustomFormationDimensionsChanged, FIntPoint, NewDimensions);

	// --- properties ---
	UPROPERTY(BlueprintAssignable, Category = "Placement Context")
	FOnSpawnCountChanged OnSpawnCountChanged;

	UPROPERTY(BlueprintAssignable, Category = "Placement Context")
	FOnSpawnFormationChanged OnSpawnFormationChanged;

	UPROPERTY(BlueprintAssignable, Category = "Placement Context")
	FOnCustomFormationDimensionsChanged OnCustomFormationDimensionsChanged;

	UPROPERTY(BlueprintReadOnly, Category = "Placement Context")
	int32 CurrentSpawnCount = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Placement Context")
	ESpawnFormation CurrentFormation = ESpawnFormation::Square;

	UPROPERTY(BlueprintReadOnly, Category = "Placement Context")
	FIntPoint CustomFormationDimensions = FIntPoint(1, 1);

	UPROPERTY(BlueprintReadOnly, Category = "Placement Context")
	float CurrentSpacing = 150.f;

    // --- setters ---
	UFUNCTION(BlueprintCallable, Category = "Placement")
	void SetSpawnCount(int32 NewCount);

	UFUNCTION(BlueprintCallable, Category = "Placement")
	void SetFormation(ESpawnFormation NewFormation);

	UFUNCTION(BlueprintCallable, Category = "Placement")
	void SetCustomFormationDimensions(FIntPoint NewDimensions);

	UFUNCTION(BlueprintCallable, Category = "Placement")
	void StartPlacement(const UPlacementItemData* ItemToPlace);

	void HandlePlacementInput();
    
	UFUNCTION(BlueprintCallable, Category = "Placement")
	void CancelPlacement();

	bool IsPlacementActive() const { return bIsPlacementActive; }

	// --- Dependencies ---
	void SetPreviewSystem(UCameraPreviewSystem* InPreview) { PreviewSystem = InPreview; }
	void SetCommandSystem(UCameraCommandSystem* InCmd) { CommandSystem = InCmd; }

private:
	// --- Internals ---
	void UpdatePreviewVisuals();
    
    UFUNCTION()
    void OnPreviewAssetLoaded();	

	void UpdateTransforms(const FVector& Center, const FRotator& Facing);
    
	void BuildGroupTransforms(const class UPlacementUnitData* UnitData, const FVector& Center, const FRotator& Facing, TArray<FTransform>& OutTransforms);

	void BuildSingleTransform(const FVector& Center, const FRotator& Facing, TArray<FTransform>& OutTransforms);

	void UpdateMouseFollow(float CurrentTime);

private:
	// --- Dependencies ---
	UPROPERTY()
	TObjectPtr<UCameraPreviewSystem> PreviewSystem;

	UPROPERTY()
	TObjectPtr<UCameraCommandSystem> CommandSystem;
    
    // --- State ---
	UPROPERTY()
	const UPlacementItemData* CurrentItemData;

	FRotationPreviewState RotationState;
	bool bIsPlacementActive = false;

	UPROPERTY(EditDefaultsOnly, Category="Settings")
	float RotationHoldTime = 0.25f;

	// --- Cache ---
	UPROPERTY(Transient)
	TArray<FVector> CachedOffsets;

	UPROPERTY(Transient)
	TArray<FTransform> CachedTransforms;
};
