#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/PatrolRoute.h"
#include "InputCoreTypes.h"
#include "UnitPatrolComponent.generated.h"

class USplineComponent;
class UUnitSelectionComponent;

/** Component responsible for creating, replicating, and visualising unit patrol routes. */
UCLASS(ClassGroup = (RTS), meta = (BlueprintSpawnableComponent))
class JUPITERPLUGIN_API UUnitPatrolComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UUnitPatrolComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Adds a patrol point to the locally edited route. */
    UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
    void AddPendingPatrolPoint(const FVector& WorldLocation);

    /** Clears the locally edited patrol. */
    UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
    void ClearPendingPatrol();

    /** Confirms the locally edited patrol and sends it to the server for replication. */
    UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
    void ConfirmPendingPatrol(bool bLoop);

    /** Enables or disables patrol spline visualisation on the owning client. */
    UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
    void SetShowPatrols(bool bEnable);

    /** Returns the currently replicated patrol routes. */
    UFUNCTION(BlueprintPure, Category = "RTS|Patrol")
    const TArray<FPatrolRoute>& GetActiveRoutes() const { return ActiveRoutes; }

    /** Handles right mouse button presses. Returns true if the event was consumed. */
    bool HandleRightClickPressed(bool bAltDown);

    /** Handles right mouse button releases. Returns true if the event was consumed. */
    bool HandleRightClickReleased(bool bAltDown);

    /** Handles left mouse button events. Returns true if the event was consumed. */
    bool HandleLeftClick(EInputEvent InputEvent);

    /** Returns true when a patrol route is currently being edited locally. */
    bool IsCreatingPatrol() const { return bIsCreatingPatrol; }

protected:
    /** RPC used to confirm a patrol on the server. */
    UFUNCTION(Server, Reliable)
    void ServerConfirmPatrol(const TArray<FVector>& RoutePoints, bool bLoop);

    /** CLIENT: Updates spline visualisation when routes are replicated. */
    UFUNCTION()
    void OnRep_ActiveRoutes();

    /** CLIENT: Rebuilds spline components that visualise replicated patrol routes. */
    void UpdateSplineVisualisation();

    /** CLIENT: Updates the preview spline used while editing a local patrol. */
    void UpdatePendingSpline();

    /** Removes all dynamically created spline components that represent active routes. */
    void ClearSplineComponents();

    /** CLIENT: Destroys the pending preview spline if present. */
    void DestroyPendingSpline();

    /** Returns true when the owning actor is locally controlled. */
    bool IsLocallyControlled() const;

    // SERVER: Called when a client confirms a patrol route
    void HandleServerPatrolConfirmation(const TArray<FVector>& RoutePoints, bool bLoop);

    /** Begins editing a new patrol route. */
    bool StartPatrolCreation();

    /** Attempts to add another patrol point based on the current cursor location. */
    bool TryAddPatrolPoint();

    /** Finalises the pending patrol route and sends it to the server. */
    void ConfirmPatrolCreation();

    /** Cancels the currently edited patrol route. */
    void CancelPatrolCreation();

    /** Retrieves the cursor location projected onto the world. */
    bool TryGetCursorLocation(FVector& OutLocation);

protected:
    /** SERVER/CLIENT: Replicated list of active patrol routes. */
    UPROPERTY(ReplicatedUsing = OnRep_ActiveRoutes)
    TArray<FPatrolRoute> ActiveRoutes;

    /** CLIENT: Runtime spline components created to render the active routes. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<USplineComponent>> ActiveSplines;

    /** CLIENT: Spline used for previewing the locally edited patrol. */
    UPROPERTY(Transient)
    TObjectPtr<USplineComponent> PendingSpline;

    /** CLIENT: Locally edited patrol points prior to confirmation. */
    UPROPERTY(Transient)
    TArray<FVector> PendingPatrolPoints;

    /** CLIENT: When false, spline visualisations are hidden. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Patrol")
    bool bShowPatrols = true;

    /** CLIENT: Height offset applied to spline points to avoid z-fighting. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Patrol")
    float SplineHeightOffset = 20.f;

    /** Minimum number of points required before creating a spline preview. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Patrol")
    int32 MinimumPointsForPreview = 2;

    /** Distance threshold used to detect loop closures when confirming a patrol. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Patrol", meta = (ClampMin = "0.0"))
    float LoopClosureThreshold = 200.f;

    /** Cached reference to the unit selection component for cursor projection. */
    UPROPERTY()
    TObjectPtr<UUnitSelectionComponent> CachedSelectionComponent;

    /** Tracks whether a patrol route is currently being created. */
    UPROPERTY(Transient)
    bool bIsCreatingPatrol = false;

    /** When true, the next right-click release will confirm the pending patrol. */
    UPROPERTY(Transient)
    bool bPendingConfirmationClick = false;

    /** Used to consume the left mouse button release after cancelling a patrol. */
    UPROPERTY(Transient)
    bool bConsumePendingLeftRelease = false;

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

