#pragma once
#include "CoreMinimal.h"
#include "CameraSystemBase.h"
#include "CameraCommandSystem.generated.h"


class ASphereRadius;
class UPatrolVisualizerComponent;
class UCameraPreviewSystem;
class UCameraSpawnSystem;

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
    
    void CancelPatrolCreation();

    // --- Actions ---
    void HandleDestroySelected();
	
    void HandleServerDestroyActor(const TArray<AActor*>& ActorsToDestroy);

    // --- Dependency Injection ---
    void SetPreviewSystem(UCameraPreviewSystem* InPreview) { PreviewSystem = InPreview; }
    void SetSpawnSystem(UCameraSpawnSystem* InSpawn) { SpawnSystem = InSpawn; }

    // --- Getters ---
    bool IsBuildingPatrolPath() const { return bIsBuildingPatrolPath; }
    bool IsAltDown() const { return bIsAltDown; }

protected:
    void ExecuteFinalCommand(const FHitResult& HitResult);
    
    // Issuers
    void IssueMoveCommand(const FVector& TargetLocation, const FRotator& Facing);
    void IssueAttackCommand(AActor* TargetActor);
    void IssuePatrolCircleCommand(float Radius, const FVector& Center);
    void IssuePatrolPathCommand();

    // Updates
    void UpdateRotationPreview(const FVector& MouseLocation);
    void UpdateCircleRadius(const FVector& MouseLocation);
    
    // Patrol Logic
    void AddPatrolWaypoint(const FVector& Location);
    void ResetPatrolPath();
    
    void UpdatePatrolPreview(const FVector& CurrentMouseLocation); 
    void ClearPatrolPreview();

    bool GetMouseHitOnTerrain(FHitResult& OutHit) const;
    AActor* GetHoveredActor() const;

protected:
    // --- References ---
    UPROPERTY()
    TObjectPtr<UCameraPreviewSystem> PreviewSystem;

    UPROPERTY()
    TObjectPtr<UCameraSpawnSystem> SpawnSystem;

    UPROPERTY()
    TObjectPtr<UPatrolVisualizerComponent> PatrolVisualizer;

    UPROPERTY()
    TObjectPtr<ASphereRadius> SphereRadius;

    // --- Settings ---
    UPROPERTY(EditDefaultsOnly, Category = "Settings|Input")
    float RotationHoldThreshold = 0.2f;

    UPROPERTY(EditDefaultsOnly, Category = "Settings|Input")
    float DragThreshold = 10.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Settings|Patrol")
    float LoopThreshold = 150.f;

    // --- State Machine ---
    ECommandMode CurrentMode = ECommandMode::None;
    
    bool bIsRightClickDown = false;
    bool bIsAltDown = false;
    
    float ClickStartTime = 0.0f;
    FVector CommandStartLocation = FVector::ZeroVector;

    // --- Rotation Data ---
    FRotator BaseRotation = FRotator::ZeroRotator;
    FRotator CurrentRotation = FRotator::ZeroRotator;

    // --- Patrol Data ---
    bool bIsBuildingPatrolPath = false;
    
    UPROPERTY(VisibleAnywhere, Category = "Debug|Patrol")
    TArray<FVector> PatrolWaypoints;

    // --- Optimization Flags ---
    FVector LastPreviewMouseLocation = FVector::ZeroVector;
    bool bPatrolPreviewDirty = false;
};