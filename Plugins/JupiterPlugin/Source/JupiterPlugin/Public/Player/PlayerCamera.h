#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/Pawn.h"
#include "PlayerCamera.generated.h"

class UInputAction;
class UInputMappingContext;
class UFloatingPawnMovement;
class UCameraComponent;
class USpringArmComponent;
class UUnitSelectionComponent;
class UUnitOrderComponent;
class UUnitFormationComponent;
class UUnitSpawnComponent;
class UUnitPatrolComponent;
class UEnhancedInputComponent;
class UCameraMovementSystem;
class UCameraSelectionSystem;
class UCameraCommandSystem;
class UCameraPatrolSystem;
class UCameraSpawnSystem;
class UCameraPreviewSystem;

/** Lightweight pawn that wires modular camera subsystems and routes inputs. */
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

    APlayerController* GetPlayerController() const { return Player.Get(); }
    UFloatingPawnMovement* GetMovementComponent() const { return PawnMovementComponent; }
    USpringArmComponent* GetSpringArm() const { return SpringArm; }
    UCameraComponent* GetCameraComponent() const { return CameraComponent; }
    UUnitSelectionComponent* GetSelectionComponent() const { return SelectionComponent; }
    UUnitOrderComponent* GetOrderComponent() const { return OrderComponent; }
    UUnitFormationComponent* GetFormationComponent() const { return FormationComponent; }
    UUnitSpawnComponent* GetSpawnComponent() const { return SpawnComponent; }
    UUnitPatrolComponent* GetPatrolComponent() const { return PatrolComponent; }
    UCameraPreviewSystem* GetPreviewSystem() const { return PreviewSystem; }

protected:
    /** Instantiates all modular systems and wires their cross references. */
    void InitializeSystems();

    /** Utility method that creates a subsystem of the provided class, or the default one. */
    template <typename TSystem>
    TSystem* CreateSystem(TSubclassOf<TSystem> SystemClass);

    /** Registers the default input bindings using the enhanced input component. */
    void BindDefaultInputs(UEnhancedInputComponent* EnhancedInput);

    void OnMove(const FInputActionValue& Value);              // Forwards movement input to the movement system.
    void OnZoom(const FInputActionValue& Value);              // Forwards zoom input to the movement system.
    void OnRotateHorizontal(const FInputActionValue& Value);  // Forwards yaw rotation input to the movement system.
    void OnRotateVertical(const FInputActionValue& Value);    // Forwards pitch rotation input to the movement system.
    void OnEnableRotate(const FInputActionValue& Value);      // Toggles camera rotation on or off.

    void OnSelectionPressed();                                // Notifies the selection system that selection has started.
    void OnSelectionReleased();                               // Notifies the selection system that selection has ended.
    void OnSelectionHold(const FInputActionValue& Value);     // Provides hold updates to the selection system (box select).
    void OnSelectAll();                                       // Broadcasts the select-all command to the selection system.

    void OnCommandActionStarted();                            // Signals the command system that a command started.
    void OnCommandActionCompleted();                          // Signals the command system that a command completed.

    void OnAltPressed();                                      // Forwards ALT press to the patrol system.
    void OnAltReleased();                                     // Forwards ALT release to the patrol system.
    void OnAltHold(const FInputActionValue& Value);           // Delivers ALT hold values to the patrol system.
    void OnPatrolTriggered(const FInputActionValue& Value);   // Sends patrol radius input to the patrol system.

    void OnDestroySelected();                                 // Requests the command system to destroy selected actors.
    void OnSpawnTriggered();                                  // Forwards spawn requests to the spawn system.

    UFUNCTION(Server, Reliable)
    void Server_DestroyActor(const TArray<AActor*>& ActorToDestroy); // Invoked by the pawn to destroy selected actors on the server.

private:
    /** Locates and caches the currently associated player controller. */
    void CachePlayerController();

    TWeakObjectPtr<APlayerController> Player;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Systems", meta=(AllowPrivateAccess="true"))
    TSubclassOf<UCameraMovementSystem> MovementSystemClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Systems", meta=(AllowPrivateAccess="true"))
    TSubclassOf<UCameraSelectionSystem> SelectionSystemClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Systems", meta=(AllowPrivateAccess="true"))
    TSubclassOf<UCameraCommandSystem> CommandSystemClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Systems", meta=(AllowPrivateAccess="true"))
    TSubclassOf<UCameraPatrolSystem> PatrolSystemClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Systems", meta=(AllowPrivateAccess="true"))
    TSubclassOf<UCameraSpawnSystem> SpawnSystemClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Systems", meta=(AllowPrivateAccess="true"))
    TSubclassOf<UCameraPreviewSystem> PreviewSystemClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UFloatingPawnMovement> PawnMovementComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
    TObjectPtr<USceneComponent> SceneComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
    TObjectPtr<USpringArmComponent> SpringArm;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UCameraComponent> CameraComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UUnitSelectionComponent> SelectionComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UUnitOrderComponent> OrderComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UUnitFormationComponent> FormationComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UUnitSpawnComponent> SpawnComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UUnitPatrolComponent> PatrolComponent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UInputMappingContext> InputMappingContext;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UInputAction> MoveAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UInputAction> ZoomAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UInputAction> EnableRotateAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UInputAction> RotateHorizontalAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UInputAction> RotateVerticalAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UInputAction> SelectAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UInputAction> SelectHoldAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UInputAction> DoubleTapAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UInputAction> CommandAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UInputAction> AltCommandAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UInputAction> AltCommandHoldAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UInputAction> PatrolCommandAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UInputAction> DeleteCommandAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UInputAction> SpawnUnitAction;

    UPROPERTY()
    TObjectPtr<UCameraMovementSystem> MovementSystem;
    UPROPERTY()
    TObjectPtr<UCameraSelectionSystem> SelectionSystem;
    UPROPERTY()
    TObjectPtr<UCameraCommandSystem> CommandSystem;
    UPROPERTY()
    TObjectPtr<UCameraPatrolSystem> PatrolSystem;
    UPROPERTY()
    TObjectPtr<UCameraSpawnSystem> SpawnSystem;
    UPROPERTY()
    TObjectPtr<UCameraPreviewSystem> PreviewSystem;
};

template <typename TSystem>
TSystem* APlayerCamera::CreateSystem(TSubclassOf<TSystem> SystemClass)
{
    const TSubclassOf<TSystem> ClassToUse = SystemClass ? SystemClass : TSystem::StaticClass();
    return NewObject<TSystem>(this, ClassToUse);
}

