#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "Data/AiData.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerCamera.generated.h"

class ASoldierRts;
class APreviewPoseMesh;
class UInputAction;
class UInputMappingContext;
class UFloatingPawnMovement;
class ASphereRadius;
class ASelectionBox;
class UCameraComponent;
class USpringArmComponent;
class UUnitSelectionComponent;
class UUnitOrderComponent;
class UUnitFormationComponent;
class UUnitSpawnComponent;
enum class ESpawnFormation : uint8;
UCLASS()
class JUPITERPLUGIN_API APlayerCamera : public APawn
{
	GENERATED_BODY()

public:
	APlayerCamera();
	
	virtual void BeginPlay() override;

	virtual void NotifyControllerChanged() override;
	
	virtual void CustomInitialized();
	
	virtual void Tick(float DeltaTime) override;

	virtual void UnPossessed() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UFloatingPawnMovement> PawnMovementComponent;

        UPROPERTY(EditAnywhere, BlueprintReadOnly)
        UUnitSelectionComponent* SelectionComponent;

        UPROPERTY(EditAnywhere, BlueprintReadOnly)
        UUnitOrderComponent* OrderComponent;

        UPROPERTY(EditAnywhere, BlueprintReadOnly)
        UUnitFormationComponent* FormationComponent;

        UPROPERTY(EditAnywhere, BlueprintReadOnly)
        UUnitSpawnComponent* SpawnComponent;
protected:
//------------------------------------ Inputs ------------------------------------
#pragma region Inputs
	
protected:
	virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Jupiter Fields", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputMappingContext> InputMappingContext;
	
	/*- Input Action -*/
	
	//MOVEMENTS
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Jupiter Fields", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Jupiter Fields", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> ZoomAction;

	//ROTATION
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Jupiter Fields", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> EnableRotateAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Jupiter Fields", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> RotateHorizontalAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Jupiter Fields", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> RotateVerticalAction;
	

	//SELECTION
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Jupiter Fields", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> SelectAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Jupiter Fields", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> SelectHoldAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Jupiter Fields", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> DoubleTap;

	//COMMAND
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Jupiter Fields", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> CommandAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Jupiter Fields", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> AltCommandAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Jupiter Fields", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> AltCommandActionTrigger;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Jupiter Fields", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> PatrolCommandAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Jupiter Fields", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> DeleteCommandAction;

	//SPAWN
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Jupiter Fields", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> SpawnUnitAction;

#pragma endregion	

	
//------------------------------------ Camera ------------------------------------
#pragma region Camera Components
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = true))
	USceneComponent* SceneComponent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = true))
	USpringArmComponent* SpringArm;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = true))
	UCameraComponent* CameraComponent;
#pragma endregion

#pragma region Camera Movement
	
protected:
	/** Déplacements de la caméra */
	UFUNCTION()
	void Input_OnMove(const FInputActionValue& ActionValue);
	
	UFUNCTION(BlueprintCallable)
	void Input_Zoom(const FInputActionValue& ActionValue);

	UFUNCTION()
	void CameraBounds();

	/** Rotation de la caméra */
	UFUNCTION()
	void Input_RotateHorizontal(const FInputActionValue& ActionValue);
	
	UFUNCTION()
	void Input_RotateVertical(const FInputActionValue& ActionValue);

	UFUNCTION()
	void Input_EnableRotate(const FInputActionValue& ActionValue);
	

	/** Gestion du Edge Scroll */
	UFUNCTION()
	void EdgeScroll();

	/** Position ciblée de la caméra */
	UPROPERTY()
	FVector TargetLocation;
	
	UPROPERTY()
	FRotator TargetRotation;
	
	UPROPERTY()
	float TargetZoom;
	
	UPROPERTY()
	bool CanRotate;

	UFUNCTION()
	void GetTerrainPosition(FVector& TerrainPosition) const;
	
#pragma endregion

#pragma region Camera Settings
protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Camera")
	float CameraSpeed = 20.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Camera")
	float EdgeScrollSpeed = 2.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Camera")
	float RotateSpeed = 2.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Camera")
	float RotatePitchMin = 10.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Camera")
	float RotatePitchMax = 80.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Camera")
	float ZoomSpeed = 2.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Camera")
	float MinZoom = 500.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Camera")
	float MaxZoom = 4000.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Camera")
	bool CanEdgeScroll = false;
#pragma endregion

	
//------------------------------------ Selection ------------------------------------
#pragma region Selection
	
protected:
	// Left Click
	UFUNCTION(BlueprintCallable)
	AActor* GetSelectedObject();

	UFUNCTION()
	void Input_SquareSelection();

	UFUNCTION()
	void Input_LeftMouseReleased();

	UFUNCTION()
	void Input_LeftMouseInputHold(const FInputActionValue& ActionValue);

	UFUNCTION()
	void HandleLeftMouse(EInputEvent InputEvent, float Value);
	
	UFUNCTION()
	void Input_SelectAllUnitType();

	
	// Alt Click
	UFUNCTION(BlueprintCallable)
	void HandleAltRightMouse(EInputEvent InputEvent, float Value);

	UFUNCTION()
	void Input_AltFunction();

	UFUNCTION()
	void Input_AltFunctionRelease();

	UFUNCTION()
	void Input_AltFunctionHold(const FInputActionValue& ActionValue);

	UFUNCTION()
	void CreateSelectionBox();
	
	UFUNCTION()
	void CreateSphereRadius();

	//--------
	template <typename T>
	TArray<T*> GetAllActorsOfClassInCameraBound(UWorld* World, TSubclassOf<T> ActorsClass);

	
	// Variables
	UPROPERTY()
	APlayerController* Player;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Mouse")
	float LeftMouseHoldThreshold = 0.15f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Mouse")
	TSubclassOf<ASelectionBox> SelectionBoxClass;
	
	UPROPERTY()
	ASelectionBox* SelectionBox;

	UPROPERTY()
	bool BoxSelect;
	
	UPROPERTY()
	FVector LeftMouseHitLocation;

	UPROPERTY()
	bool MouseProjectionIsGrounded;

	UPROPERTY(BlueprintReadWrite)
	bool bAltIsPressed = false;
	
#pragma endregion

#pragma region Command

	UFUNCTION()
	void Input_PatrolZone(const FInputActionValue& ActionValue);

	UFUNCTION()
	void Input_OnDestroySelected();

	UFUNCTION(Server, Reliable)
	void Server_DestroyActor(const TArray<AActor*>& ActorToDestroy);

	UFUNCTION(BlueprintCallable)
	void CommandStart();

	UFUNCTION(BlueprintCallable)
	void Command();

	UFUNCTION()
	FCommandData CreateCommandData(const ECommandType Type, AActor* Enemy = nullptr, float Radius = 0.f) const;

	
	UPROPERTY(EditAnywhere, Category = "Settings|Command")
	TSubclassOf<ASphereRadius> SphereRadiusClass;
	
	UPROPERTY()
	ASphereRadius* SphereRadius;
	
	UPROPERTY()
	bool SphereRadiusEnable;
	
	UPROPERTY()
	FVector CommandLocation;
#pragma endregion

#pragma region Spawn Units

protected:

	UFUNCTION()
	void CreatePreviewMesh();
	
        UFUNCTION()
        void Input_OnSpawnUnits();

        UFUNCTION()
        void ShowUnitPreview(TSubclassOf<ASoldierRts> NewUnitClass);

        UFUNCTION()
        void HidePreview();

        UFUNCTION()
        void PreviewFollowMouse();

        UFUNCTION()
        void HandleSpawnCountChanged(int32 NewSpawnCount);

        UFUNCTION()
        void HandleSpawnFormationChanged(ESpawnFormation NewFormation);

        UFUNCTION()
        void HandleCustomFormationDimensionsChanged(FIntPoint NewDimensions);

        UPROPERTY()
        bool bIsInSpawnUnits = false;

        UPROPERTY(EditAnywhere, Category = "Settings|Spawn Units")
        TSubclassOf<APreviewPoseMesh> PreviewUnitsClass;

        UPROPERTY()
        TObjectPtr<APreviewPoseMesh> PreviewUnit;

        void EnsurePreviewUnit();
        void UpdatePreviewTransforms(const FVector& CenterLocation, const FRotator& FacingRotation);
        void RefreshPreviewInstances();
        void BuildPreviewFormationOffsets(int32 SpawnCount, float Spacing, TArray<FVector>& OutOffsets) const;
        int32 GetEffectiveSpawnCount() const;
        bool HasPreviewUnits() const { return PreviewUnit != nullptr; }

#pragma endregion

private:
        /** Lazily resolves the selection component, logging a warning if missing. */
        UUnitSelectionComponent* GetSelectionComponentChecked() const;
};

template <typename T>
TArray<T*> APlayerCamera::GetAllActorsOfClassInCameraBound(UWorld* World, TSubclassOf<T> ActorsClass)
{
	TArray<T*> ActorsInView;
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, ActorsClass, FoundActors);

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (PC)
	{
		int32 ViewportX, ViewportY;
		PC->GetViewportSize(ViewportX, ViewportY);

		for (AActor* Actor : FoundActors)
		{
			FVector2D ScreenLocation;
			if (PC->ProjectWorldLocationToScreen(Actor->GetActorLocation(), ScreenLocation))
			{
				if (ScreenLocation.X >= 0 && ScreenLocation.X <= ViewportX &&
					ScreenLocation.Y >= 0 && ScreenLocation.Y <= ViewportY)
				{
					T* CastedActor = Cast<T>(Actor);
					if (CastedActor)
					{
						ActorsInView.Add(CastedActor);
					}
				}
			}
		}
	}
	return ActorsInView;
}
