#pragma once
#include "CoreMinimal.h"
#include "CameraSystemBase.h"
#include "InputActionValue.h"
#include "CameraCommandSystem.generated.h"


class ASphereRadius;


UCLASS()
class JUPITERPLUGIN_API UCameraCommandSystem : public UCameraSystemBase
{
    GENERATED_BODY()

public:
    virtual void Init(APlayerCamera* InOwner) override;
    virtual void Tick(float DeltaTime) override;

    // Command (Right-click)
    void HandleCommandActionStarted();
    void HandleCommandActionCompleted();

    // Patrol (Alt + Right-click)
    void HandleAltPressed();
    void HandleAltReleased();
    void HandleAltHold(const FInputActionValue& Value);

    // Destroy
    void HandleDestroySelected();
    void HandleServerDestroyActor(const TArray<AActor*>& ActorsToDestroy);

    // Link systems
    void SetPreviewSystem(class UCameraPreviewSystem* InPreview) { PreviewSystem = InPreview; }
    void SetSpawnSystem(class UCameraSpawnSystem* InSpawn) { SpawnSystem = InSpawn; }

protected:
    // --- Command issuing ---
    void IssueMoveCommand(const FVector& TargetLocation, const FRotator& Facing);
    void IssueAttackCommand(AActor* TargetActor);
    void IssuePatrolCommand(float Radius, const FVector& Center);

    void IssueFinalCommand(const FHitResult& HitResult);

    // --- Patrol helpers ---
    void StartPatrolRadius(const FVector& Center);
    void StopPatrolRadius();
    void UpdatePatrolRadius(const FVector& MouseLocation);

    // --- Rotation preview ---
    bool TryActivateRotationPreview(float CurrentTime, float Threshold, const FVector& InitialLocation, const FVector& MouseLocation);
    void BeginRotationPreview(float CurrentTime, const FVector& Center, const FRotator& BaseYaw);
    void UpdateRotationPreview(const FVector& MouseLocation);
    void StopRotationPreview();

    // --- Helpers ---
    bool GetMouseHitOnTerrain(FHitResult& OutHit) const;
    AActor* GetHoveredActor() const;

protected:

    // Base preview systems
    UCameraPreviewSystem* PreviewSystem = nullptr;
    UCameraSpawnSystem* SpawnSystem = nullptr;

    // --- Rotation preview ---
    bool bIsCommandHeld = false;
    bool bRotationPreviewActive = false;
    bool bRotationHoldTriggered = false;

    float RotationHoldStartTime = 0.f;
    float RotationHoldThreshold = 0.25f;

    FVector RotationCenter = FVector::ZeroVector;
    FVector InitialDirection = FVector::ForwardVector;

    FRotator BaseRotation = FRotator::ZeroRotator;
    FRotator CurrentRotation = FRotator::ZeroRotator;

    FVector CommandStartLocation = FVector::ZeroVector;

    // --- Patrol ---
    bool bAltPressed = false;
    bool bPatrolActive = false;
    bool bIsCreatingPatrol = false;

public:
    void CancelPatrolCreation();
    bool IsCreatingPatrol() const { return bIsCreatingPatrol; }

    UPROPERTY(EditAnywhere)
    TSubclassOf<ASphereRadius> SphereRadiusClass;

    UPROPERTY()
    ASphereRadius* SphereRadius = nullptr;

    float PatrolHoldThreshold = 0.25f;
};
