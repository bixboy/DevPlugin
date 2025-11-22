#pragma once

#include "CoreMinimal.h"
#include "CameraSystemBase.h"
#include "CameraCommandSystem.generated.h"

class ASphereRadius;

UENUM(BlueprintType)
enum class ECommandMode : uint8
{
    None,
    MoveRotation,   // Clic droit maintenu (Flèche direction)
    PatrolCircle,   // Alt + Clic droit maintenu + Slide (Cercle zone)
    PatrolPathing   // Mode actif (on est en train de placer des points)
};

UCLASS()
class JUPITERPLUGIN_API UCameraCommandSystem : public UCameraSystemBase
{
    GENERATED_BODY()

public:
    virtual void Init(APlayerCamera* InOwner) override;
    virtual void Tick(float DeltaTime) override;

    // --- Inputs Handlers ---
    // Clic Droit
    void HandleCommandActionStarted();
    void HandleCommandActionCompleted();

    // Alt
    void HandleAltPressed();
    void HandleAltReleased();
    
    // NOTE : J'ai supprimé HandleAltHold car on gère maintenant le hold dans le Tick 
    // pour différencier proprement le "Slide" du "Click"

    // --- Actions ---
    void HandleDestroySelected();
    void HandleServerDestroyActor(const TArray<AActor*>& ActorsToDestroy);

    // --- Link systems ---
    void SetPreviewSystem(class UCameraPreviewSystem* InPreview) { PreviewSystem = InPreview; }
    void SetSpawnSystem(class UCameraSpawnSystem* InSpawn) { SpawnSystem = InSpawn; }

    // --- Getters / State Checks ---
    bool IsBuildingPatrolPath() const { return bIsBuildingPatrolPath; }
    bool IsAltDown() const;

protected:
    // --- Core Logic Helpers ---
    void ExecuteFinalCommand(const FHitResult& HitResult);
    
    // Helpers pour l'exécution des ordres (Envoi au Component)
    void IssueMoveCommand(const FVector& TargetLocation, const FRotator& Facing);
    void IssueAttackCommand(AActor* TargetActor);
    void IssuePatrolCircleCommand(float Radius, const FVector& Center);
    void IssuePatrolPathCommand();

    // Helpers visuels & calculs
    void UpdateRotationPreview(const FVector& MouseLocation);
    void UpdateCircleRadius(const FVector& MouseLocation);
    
    // Helpers Patrouille par points
    void AddPatrolWaypoint(const FVector& Location);
    void ResetPatrolPath();

    // Helpers Raycast
    bool GetMouseHitOnTerrain(FHitResult& OutHit) const;
    AActor* GetHoveredActor() const;

protected:
    // --- Systems References ---
    UPROPERTY()
    UCameraPreviewSystem* PreviewSystem = nullptr;

    UPROPERTY()
    UCameraSpawnSystem* SpawnSystem = nullptr;

    // --- Settings ---
    UPROPERTY(EditDefaultsOnly, Category = "Settings|Command")
    TSubclassOf<ASphereRadius> SphereRadiusClass;

    UPROPERTY(EditDefaultsOnly, Category = "Settings|Command")
    float RotationHoldThreshold = 0.20f; // Temps avant d'afficher la flèche

    UPROPERTY(EditDefaultsOnly, Category = "Settings|Command")
    float DragThreshold = 10.0f; // Distance souris avant de considérer un "Slide"

    // --- State Machine Data ---
    ECommandMode CurrentMode = ECommandMode::None;
    
    bool bIsRightClickDown = false;
    bool bIsAltDown = false;
    
    float ClickStartTime = 0.0f;        // Pour calculer la durée du maintien
    FVector CommandStartLocation;       // Où la souris était au début du clic

    // --- Rotation Data ---
    FRotator BaseRotation = FRotator::ZeroRotator;
    FRotator CurrentRotation = FRotator::ZeroRotator; // Rotation calculée

    // --- Patrol Data ---
    bool bIsBuildingPatrolPath = false; // Est-on en train de placer des points ?
    
    UPROPERTY(VisibleAnywhere, Category = "Debug|Patrol")
    TArray<FVector> PatrolWaypoints;    // Liste des points de passage

    UPROPERTY()
    ASphereRadius* SphereRadius = nullptr;
};