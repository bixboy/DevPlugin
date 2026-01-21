#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "PlayerCamera.generated.h"

class ASphereRadius;
class APreviewPoseMesh;
class ASelectionBox;
class UFloatingPawnMovement;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;

class UUnitSelectionComponent;
class UUnitOrderComponent;
class UUnitFormationComponent;
class UUnitSpawnComponent;
class UUnitPatrolComponent;
class UCameraMovementSystem;
class UCameraSelectionSystem;
class UCameraCommandSystem;
class UCameraSpawnSystem;
class UCameraPreviewSystem;


UCLASS()
class JUPITERPLUGIN_API APlayerCamera : public APawn
{
    GENERATED_BODY()

public:
    APlayerCamera();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    virtual void NotifyControllerChanged() override;
    virtual void UnPossessed() override;

    // -------- Getters --------
    FORCEINLINE UCameraMovementSystem* GetMovementSystem() const { return MovementSystem; }
    FORCEINLINE UCameraSelectionSystem* GetSelectionSystem() const { return SelectionSystem; }
    FORCEINLINE UCameraCommandSystem* GetCommandSystem() const { return CommandSystem; }
    FORCEINLINE UCameraSpawnSystem* GetSpawnSystem() const { return SpawnSystem; }
    FORCEINLINE UCameraPreviewSystem* GetPreviewSystem() const { return PreviewSystem; }
    
    FORCEINLINE APlayerController* GetPlayerController() const { return Player.IsValid() ? Player.Get() : nullptr; }
    FORCEINLINE const FRotator& GetTargetRotation() const { return TargetRotation; }
    
    FORCEINLINE TSubclassOf<ASelectionBox> GetSelectionBoxClass() const { return SelectionBoxClass; }
    FORCEINLINE TSubclassOf<APreviewPoseMesh> GetPreviewMeshClass() const { return PreviewMeshClass; }

    // -------- Components Public Access --------
    FORCEINLINE UUnitSpawnComponent* GetSpawnComponent() const { return SpawnComponent; }
    FORCEINLINE UUnitFormationComponent* GetFormationComponent() const { return FormationComponent; }
    FORCEINLINE UUnitOrderComponent* GetOrderComponent() const { return OrderComponent; }
    FORCEINLINE UUnitSelectionComponent* GetSelectionComponent() const { return SelectionComponent; }
    FORCEINLINE UUnitPatrolComponent* GetPatrolComponent() const { return PatrolComponent; }
    
    FORCEINLINE UCameraComponent* GetCameraComponent() const { return CameraComponent; }
    FORCEINLINE USpringArmComponent* GetSpringArm() const { return SpringArm; }

    // -------- Components --------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jupiter|Components")
    TObjectPtr<UFloatingPawnMovement> PawnMovementComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jupiter|Components")
    TObjectPtr<USceneComponent> SceneComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jupiter|Components")
    TObjectPtr<USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jupiter|Components")
    TObjectPtr<UCameraComponent> CameraComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jupiter|Components")
    TObjectPtr<UUnitSelectionComponent> SelectionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jupiter|Components")
    TObjectPtr<UUnitOrderComponent> OrderComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jupiter|Components")
    TObjectPtr<UUnitFormationComponent> FormationComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jupiter|Components")
    TObjectPtr<UUnitSpawnComponent> SpawnComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Jupiter|Components")
    TObjectPtr<UUnitPatrolComponent> PatrolComponent;

protected:
    void InitializeSystems();

protected:
    // -------- Inputs --------
    UPROPERTY(EditAnywhere, Category="Settings|Inputs")
    TObjectPtr<UInputMappingContext> InputMapping;

    // Movement Actions
    UPROPERTY(EditAnywhere, Category="Settings|Inputs|Actions|Movement")
    TObjectPtr<UInputAction> MoveAction;
    UPROPERTY(EditAnywhere, Category="Settings|Inputs|Actions|Movement")
    TObjectPtr<UInputAction> ZoomAction;
    UPROPERTY(EditAnywhere, Category="Settings|Inputs|Actions|Movement")
    TObjectPtr<UInputAction> RotateHorizontalAction;
    UPROPERTY(EditAnywhere, Category="Settings|Inputs|Actions|Movement")
    TObjectPtr<UInputAction> RotateVerticalAction;
    UPROPERTY(EditAnywhere, Category="Settings|Inputs|Actions|Movement")
    TObjectPtr<UInputAction> EnableRotateAction;

    // Selection Actions
    UPROPERTY(EditAnywhere, Category="Settings|Inputs|Actions|Selection")
    TObjectPtr<UInputAction> SelectAction;
    UPROPERTY(EditAnywhere, Category="Settings|Inputs|Actions|Selection")
    TObjectPtr<UInputAction> SelectHoldAction;
    UPROPERTY(EditAnywhere, Category="Settings|Inputs|Actions|Selection")
    TObjectPtr<UInputAction> DoubleTapAction;
    
	UPROPERTY(EditAnywhere, Category="Settings|Inputs")
	TObjectPtr<UInputAction> ControlGroupAction;

    // Command Actions
    UPROPERTY(EditAnywhere, Category="Settings|Inputs|Actions|Commands")
    TObjectPtr<UInputAction> CommandAction;
    UPROPERTY(EditAnywhere, Category="Settings|Inputs|Actions|Commands")
    TObjectPtr<UInputAction> AltCommandAction;
    UPROPERTY(EditAnywhere, Category="Settings|Inputs|Actions|Commands")
    TObjectPtr<UInputAction> AltCommandHoldAction;
    UPROPERTY(EditAnywhere, Category="Settings|Inputs|Actions|Commands")
    TObjectPtr<UInputAction> PatrolAction;
    UPROPERTY(EditAnywhere, Category="Settings|Inputs|Actions|Commands")
    TObjectPtr<UInputAction> DeleteAction;
    UPROPERTY(EditAnywhere, Category="Settings|Inputs|Actions|Commands")
    TObjectPtr<UInputAction> CancelAction;

    // Spawn Actions
    UPROPERTY(EditAnywhere, Category="Settings|Inputs|Actions|Spawn")
    TObjectPtr<UInputAction> SpawnUnitAction;

protected:
    // -------- Systems Instances --------
    UPROPERTY(Transient)
    TObjectPtr<UCameraMovementSystem> MovementSystem;
    UPROPERTY(Transient)
    TObjectPtr<UCameraSelectionSystem> SelectionSystem;
    UPROPERTY(Transient)
    TObjectPtr<UCameraCommandSystem> CommandSystem;
    UPROPERTY(Transient)
    TObjectPtr<UCameraSpawnSystem> SpawnSystem;
    UPROPERTY(Transient)
    TObjectPtr<UCameraPreviewSystem> PreviewSystem;

    // System Classes
    UPROPERTY(EditAnywhere, Category="Settings|Class|Systems")
    TSubclassOf<UCameraMovementSystem> MovementSystemClass;
    UPROPERTY(EditAnywhere, Category="Settings|Class|Systems")
    TSubclassOf<UCameraSelectionSystem> SelectionSystemClass;
    UPROPERTY(EditAnywhere, Category="Settings|Class|Systems")
    TSubclassOf<UCameraCommandSystem> CommandSystemClass;
    UPROPERTY(EditAnywhere, Category="Settings|Class|Systems")
    TSubclassOf<UCameraSpawnSystem> SpawnSystemClass;
    UPROPERTY(EditAnywhere, Category="Settings|Class|Systems")
    TSubclassOf<UCameraPreviewSystem> PreviewSystemClass;

    // Helper Classes
    UPROPERTY(EditAnywhere, Category="Settings|Class|Other")
    TSubclassOf<ASelectionBox> SelectionBoxClass;
    UPROPERTY(EditAnywhere, Category="Settings|Class|Other")
    TSubclassOf<APreviewPoseMesh> PreviewMeshClass;

private:
    TWeakObjectPtr<APlayerController> Player;

public:
    // -------- Camera Settings --------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings|Camera")
    float CameraSpeed = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings|Camera")
    float EdgeScrollSpeed = 2.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings|Camera")
    float RotateSpeed = 2.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings|Camera")
    float RotatePitchMin = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings|Camera")
    float RotatePitchMax = 80.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings|Camera")
    float MinZoom = 500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings|Camera")
    float MaxZoom = 4000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Settings|Camera")
    bool CanEdgeScroll = true;

    // Distance max pour considérer une unité affichable (Optimisation)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Performance")
    float MaxSelectionDistance = 15000.f; 
    
    // -------- Camera State --------
    UPROPERTY(Transient)
    FVector TargetLocation = FVector::ZeroVector;

    UPROPERTY(Transient)
    FRotator TargetRotation = FRotator::ZeroRotator;

    UPROPERTY(Transient)
    float TargetZoom = 3000.f;
    
    // -------- Command Settings --------
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Settings|Commands")
    TSubclassOf<ASphereRadius> SphereRadiusClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Commands")
    float RotationHoldThreshold = 0.20f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Commands")
    float DragThreshold = 10.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Commands")
    float LoopThreshold = 150.f;


    // ---------------------------------------------------------
    // SELECTOR
    // ---------------------------------------------------------
    template <typename T>
    TArray<T*> GetAllActorsOfClassInCameraBound(UWorld* World, TSubclassOf<T> Class)
    {
        TArray<T*> VisibleActors;
        
        // Safety Checks
        if (!World || !Player.IsValid() || !Class)
        	return VisibleActors;

        int32 ViewX = 0, ViewY = 0;
        Player->GetViewportSize(ViewX, ViewY);
        if (ViewX <= 0 || ViewY <= 0)
        	return VisibleActors;

        // Pre-calculation for distance check (optimization)
        const FVector CameraLoc = GetActorLocation();
        const float MaxDistSq = MaxSelectionDistance * MaxSelectionDistance;

        // TODO: A remplacer plus tard par : GetJupiterGameState()->GetRegisteredUnits()
    	
        for (TActorIterator<T> It(World, Class); It; ++It)
        {
            T* Actor = *It;
            if (!Actor)
            	continue;

            // 1. Distance Check
            if (FVector::DistSquared(CameraLoc, Actor->GetActorLocation()) > MaxDistSq)
            {
                continue;
            }

            // 2. Screen Projection
            FVector2D ScreenPos;
            if (Player->ProjectWorldLocationToScreen(Actor->GetActorLocation(), ScreenPos))
            {
                if ((ScreenPos.X >= 0 && ScreenPos.X <= ViewX) && 
                    (ScreenPos.Y >= 0 && ScreenPos.Y <= ViewY))
                {
                    VisibleActors.Add(Actor);
                }
            }
        }

        return VisibleActors;
    }

    template<typename T>
    T* CreateSystem(TSubclassOf<T> Class)
    {
        if (!Class)
        	return nullptr;
    	
        return NewObject<T>(this, Class);
    }
};