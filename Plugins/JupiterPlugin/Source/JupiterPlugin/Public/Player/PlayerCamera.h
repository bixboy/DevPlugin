#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerCamera.generated.h"

class APreviewPoseMesh;
class ASelectionBox;
class UFloatingPawnMovement;
class USpringArmComponent;
class UCameraComponent;

class UUnitSelectionComponent;
class UUnitOrderComponent;
class UUnitFormationComponent;
class UUnitSpawnComponent;
class UUnitPatrolComponent;

class UInputMappingContext;
class UInputAction;

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

    // -------- Getters for Systems --------
    FORCEINLINE UCameraMovementSystem* GetMovementSystem() const { return MovementSystem; }
    FORCEINLINE UCameraSelectionSystem* GetSelectionSystem() const { return SelectionSystem; }
    FORCEINLINE UCameraCommandSystem* GetCommandSystem() const { return CommandSystem; }
    FORCEINLINE UCameraSpawnSystem* GetSpawnSystem() const { return SpawnSystem; }
    FORCEINLINE UCameraPreviewSystem* GetPreviewSystem() const { return PreviewSystem; }
	
	FORCEINLINE APlayerController* GetPlayerController() const { return Player.IsValid() ? Player.Get() : nullptr; }
	
	FORCEINLINE const FRotator& GetTargetRotation() const { return TargetRotation; }
	FORCEINLINE TSubclassOf<ASelectionBox> GetSelectionBoxClass() { return SelectionBoxClass; }

	FORCEINLINE UUnitSpawnComponent* GetSpawnComponent() { return SpawnComponent; }
	FORCEINLINE UUnitFormationComponent* GetFormationComponent() { return FormationComponent; }
	FORCEINLINE UUnitOrderComponent* GetOrderComponent() { return OrderComponent; }
	FORCEINLINE UUnitSelectionComponent* GetSelectionComponent() { return SelectionComponent; }
	FORCEINLINE UUnitPatrolComponent* GetPatrolComponent() { return PatrolComponent; }
	
	FORCEINLINE UCameraComponent* GetCameraComponent() { return CameraComponent; }
	FORCEINLINE USpringArmComponent* GetSpringArm() { return SpringArm; }

    // -------- Components --------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UFloatingPawnMovement> PawnMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> SceneComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UCameraComponent> CameraComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UUnitSelectionComponent> SelectionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UUnitOrderComponent> OrderComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UUnitFormationComponent> FormationComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UUnitSpawnComponent> SpawnComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UUnitPatrolComponent> PatrolComponent;

protected:
    void InitializeSystems();

protected:
    // -------- Input Mapping --------
    UPROPERTY(EditAnywhere, Category="Settings|Inputs")
    TObjectPtr<UInputMappingContext> InputMapping;

    // Movement
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

    // Selection
    UPROPERTY(EditAnywhere, Category="Settings|Inputs|Actions|Selection")
	TObjectPtr<UInputAction> SelectAction;
	
    UPROPERTY(EditAnywhere, Category="Settings|Inputs|Actions|Selection")
	TObjectPtr<UInputAction> SelectHoldAction;
	
    UPROPERTY(EditAnywhere, Category="Settings|Inputs|Actions|Selection")
	TObjectPtr<UInputAction> DoubleTapAction;

    // Commands
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

    // Spawn
    UPROPERTY(EditAnywhere, Category="Settings|Inputs|Actions|Spawn")
	TObjectPtr<UInputAction> SpawnUnitAction;

protected:
    // -------- Systems instances --------
    UPROPERTY()
	UCameraMovementSystem* MovementSystem;
	
    UPROPERTY()
	UCameraSelectionSystem* SelectionSystem;
	
    UPROPERTY()
	UCameraCommandSystem* CommandSystem;
	
    UPROPERTY()
	UCameraSpawnSystem* SpawnSystem;
	
    UPROPERTY()
	UCameraPreviewSystem* PreviewSystem;

    // Class references (editable)
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

	UPROPERTY(EditAnywhere, Category="Settings|Class|Other")
	TSubclassOf<ASelectionBox> SelectionBoxClass;

private:
    TWeakObjectPtr<APlayerController> Player;

public:

	UPROPERTY(EditAnywhere, Category="Settings|Class|Other")
	TSubclassOf<APreviewPoseMesh> PreviewMeshClass;
	
	// ------------------------------------------------------------
	// Camera Settings
	// ------------------------------------------------------------
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

	// ------------------------------------------------------------
	// Camera State
	// ------------------------------------------------------------
	UPROPERTY()
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY()
	FRotator TargetRotation = FRotator::ZeroRotator;

	UPROPERTY()
	float TargetZoom = 3000.f;

	
	// ---------------------------------------------------------
	// Get actor in camera bound
	// ---------------------------------------------------------
	template <typename T>
	TArray<T*> GetAllActorsOfClassInCameraBound(UWorld* World, TSubclassOf<T> Class)
	{
		TArray<T*> VisibleActors;
		if (!World || !Player.IsValid() || !Class)
			return VisibleActors;

		int32 ViewX = 0, ViewY = 0;
		Player->GetViewportSize(ViewX, ViewY);
		if (ViewX <= 0 || ViewY <= 0)
			return VisibleActors;

		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(World, Class, Found);

		for (AActor* A : Found)
		{
			if (!A)
				continue;

			FVector2D ScreenPos;
			if (Player->ProjectWorldLocationToScreen(A->GetActorLocation(), ScreenPos))
			{
				if ((ScreenPos.X >= 0 && ScreenPos.X <= ViewX) && (ScreenPos.Y >= 0 && ScreenPos.Y <= ViewY))
				{
					if (T* AsType = Cast<T>(A))
						VisibleActors.Add(AsType);
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
		
		T* System = NewObject<T>(this, Class);
		return System;
	}
};
