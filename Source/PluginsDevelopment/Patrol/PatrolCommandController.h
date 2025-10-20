#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PatrolRouteTypes.h"
#include "PatrolCommandController.generated.h"

class UPatrolRouteComponent;

/** Player controller implementing patrol creation and visualization commands. */
UCLASS()
class PLUGINSDEVELOPMENT_API APatrolCommandController : public APlayerController
{
        GENERATED_BODY()

public:
        APatrolCommandController();

        virtual void SetupInputComponent() override;
        virtual void PlayerTick(float DeltaTime) override;

        /** Updates the list of currently selected units. */
        UFUNCTION(BlueprintCallable, Category = "Patrol")
        void SetSelectedUnits(const TArray<AActor*>& Units);

        /** Clears the current unit selection. */
        UFUNCTION(BlueprintCallable, Category = "Patrol")
        void ClearSelectedUnits();

protected:
        /** Handles right mouse button presses. */
        void HandleRightClick();

        /** Handles left mouse button presses. */
        void HandleLeftClick();

private:
        /** Attempts to retrieve the world hit location under the cursor. */
        bool TryGetCursorWorldLocation(FVector& OutLocation) const;

        /** Begins a new patrol creation session. */
        void StartPatrolCreation(const FVector& InitialPoint);

        /** Confirms the current patrol, assigning it to selected units. */
        void ConfirmPatrolCreation();

        /** Cancels the current patrol creation session. */
        void CancelPatrolCreation();

        /** Adds an intermediate patrol point. */
        void AddPatrolPoint(const FVector& NewPoint);

        /** Returns whether the Alt key is currently pressed. */
        bool IsAltKeyDown() const;

        /** Draws the pending patrol preview. */
        void DrawPatrolPreview() const;

        /** Draws patrol routes for the selected units. */
        void DrawSelectedRoutes() const;

private:
        /** Units currently selected by the player. */
        UPROPERTY()
        TArray<TWeakObjectPtr<AActor>> SelectedUnits;

        /** Pending patrol points while editing. */
        UPROPERTY()
        TArray<FVector> PendingPatrolPoints;

        /** Whether a patrol creation is currently in progress. */
        bool bIsCreatingPatrol = false;

        /** Distance threshold used to detect a closed loop (in world units). */
        UPROPERTY(EditDefaultsOnly, Category = "Patrol")
        float LoopClosureThreshold = 150.0f;

        /** Color used for preview lines. */
        UPROPERTY(EditDefaultsOnly, Category = "Patrol|Debug")
        FColor PreviewColor = FColor::Cyan;

        /** Color used for selected patrols. */
        UPROPERTY(EditDefaultsOnly, Category = "Patrol|Debug")
        FColor SelectedRouteColor = FColor::Green;
};
