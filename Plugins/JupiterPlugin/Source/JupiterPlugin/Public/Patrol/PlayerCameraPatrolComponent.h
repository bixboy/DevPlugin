#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Patrol/PatrolRouteTypes.h"
#include "PlayerCameraPatrolComponent.generated.h"

class APlayerCamera;
class UInputComponent;
class UUnitSelectionComponent;
class UPatrolRouteComponent;

/** Handles patrol authoring inputs directly from the player camera pawn. */
UCLASS(ClassGroup = (RTS), meta = (BlueprintSpawnableComponent))
class JUPITERPLUGIN_API UPlayerCameraPatrolComponent : public UActorComponent
{
        GENERATED_BODY()

public:
        UPlayerCameraPatrolComponent();

        virtual void BeginPlay() override;
        virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
        virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

        /** Binds the input required to author patrol routes. */
        void BindInput(UInputComponent* InputComponent);

protected:
        /** Handles right mouse clicks. */
        void HandleRightMousePressed();

        /** Handles left mouse clicks. */
        void HandleLeftMousePressed();

        /** Updates cached selection when units change. */
        UFUNCTION()
        void HandleSelectionChanged(const TArray<AActor*>& SelectedActors);

        /** Retrieves the world-space cursor location. */
        bool TryGetCursorWorldLocation(FVector& OutLocation) const;

        /** Begins a new patrol authoring session. */
        void StartPatrolCreation(const FVector& InitialPoint);

        /** Confirms the current patrol and assigns it to the selected units. */
        void ConfirmPatrolCreation();

        /** Cancels the current patrol authoring session. */
        void CancelPatrolCreation();

        /** Adds a waypoint to the pending patrol path. */
        void AddPatrolPoint(const FVector& Point);

        /** Returns true when either Alt key is held. */
        bool IsAltPressed() const;

        /** Draws the in-progress patrol preview. */
        void DrawPatrolPreview() const;

        /** Renders patrol paths assigned to the current selection. */
        void DrawSelectedRoutes() const;

        /** Ensures cached selection removes invalid actors. */
        void PruneInvalidSelection();

protected:
        /** Weak reference to the owning camera pawn. */
        TWeakObjectPtr<APlayerCamera> CachedCamera;

        /** Cached controller used for cursor traces and input state. */
        TWeakObjectPtr<APlayerController> CachedController;

        /** Selection component we listen to for selection updates. */
        UPROPERTY()
        TWeakObjectPtr<UUnitSelectionComponent> SelectionComponent;

        /** Currently selected units. */
        UPROPERTY()
        TArray<TWeakObjectPtr<AActor>> SelectedUnits;

        /** Pending patrol points while editing. */
        UPROPERTY()
        TArray<FVector> PendingPatrolPoints;

        /** True when a patrol is being authored. */
        bool bIsCreatingPatrol = false;

        /** Threshold used to detect closed loops. */
        UPROPERTY(EditAnywhere, Category = "Patrol")
        float LoopClosureThreshold = 150.0f;

        /** Debug color for preview visuals. */
        UPROPERTY(EditAnywhere, Category = "Patrol|Debug")
        FColor PreviewColor = FColor::Cyan;

        /** Debug color for selected patrols. */
        UPROPERTY(EditAnywhere, Category = "Patrol|Debug")
        FColor SelectedRouteColor = FColor::Green;

        /** True once input bindings have been registered. */
        bool bInputBound = false;
};
