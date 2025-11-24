#pragma once

#include "CoreMinimal.h"
#include "CameraSystemBase.h"
#include "CameraCommandSystem.generated.h"

class ASphereRadius;
class UPatrolVisualizerComponent;

UENUM(BlueprintType)
enum class ECommandMode : uint8
{
    None,
    MoveRotation,
    PatrolCircle,
    PatrolPathing
};

UCLASS()
class JUPITERPLUGIN_API UCameraCommandSystem : public UCameraSystemBase
{
    GENERATED_BODY()

public:
    virtual void Init(APlayerCamera* InOwner) override;
    virtual void Tick(float DeltaTime) override;

    // --- Inputs Handlers ---
    void HandleCommandActionStarted();
    void HandleCommandActionCompleted();

    void HandleAltPressed();
    void HandleAltReleased();

    // --- Actions ---
    void HandleDestroySelected();
    void HandleServerDestroyActor(const TArray<AActor*>& ActorsToDestroy);

    void SetPreviewSystem(class UCameraPreviewSystem* InPreview) { PreviewSystem = InPreview; }
    void SetSpawnSystem(class UCameraSpawnSystem* InSpawn) { SpawnSystem = InSpawn; }

    bool IsBuildingPatrolPath() const { return bIsBuildingPatrolPath; }
    bool IsAltDown() const { return bIsAltDown; }

protected:
    void ExecuteFinalCommand(const FHitResult& HitResult);
    
    void IssueMoveCommand(const FVector& TargetLocation, const FRotator& Facing);
    void IssueAttackCommand(AActor* TargetActor);
    void IssuePatrolCircleCommand(float Radius, const FVector& Center);
    void IssuePatrolPathCommand();

    void UpdateRotationPreview(const FVector& MouseLocation);
    void UpdateCircleRadius(const FVector& MouseLocation);
    
    void AddPatrolWaypoint(const FVector& Location);
    void ResetPatrolPath();
    
    void UpdatePatrolPreview();
    void ClearPatrolPreview();

    bool GetMouseHitOnTerrain(FHitResult& OutHit) const;
    AActor* GetHoveredActor() const;

protected:
    // --- Systems References ---
    UPROPERTY()
    UCameraPreviewSystem* PreviewSystem = nullptr;

    UPROPERTY()
    UCameraSpawnSystem* SpawnSystem = nullptr;

    UPROPERTY()
    UPatrolVisualizerComponent* PatrolVisualizer = nullptr;
	

    // --- Settings ---
    UPROPERTY()
    float RotationHoldThreshold;

    UPROPERTY()
    float DragThreshold;

	UPROPERTY()
	float LoopThreshold;
	

    // --- State Machine Data ---
    ECommandMode CurrentMode = ECommandMode::None;
    
    bool bIsRightClickDown = false;
    bool bIsAltDown = false;
    
    float ClickStartTime = 0.0f;
    FVector CommandStartLocation;

    // --- Rotation Data ---
    FRotator BaseRotation = FRotator::ZeroRotator;
    FRotator CurrentRotation = FRotator::ZeroRotator;

    // --- Patrol Data ---
    bool bIsBuildingPatrolPath = false;
    
    UPROPERTY(VisibleAnywhere, Category = "Debug|Patrol")
    TArray<FVector> PatrolWaypoints;

    UPROPERTY()
    ASphereRadius* SphereRadius = nullptr;
};