#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/AiData.h"
#include "UnitPatrolComponent.generated.h"

class APlayerCamera;
class APlayerController;
class UUnitSelectionComponent;
class UUnitOrderComponent;

USTRUCT(BlueprintType)
struct FPatrolRoute
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<AActor*> AssignedUnits;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FVector> PatrolPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsLoop = false;
};

/** Component responsible for handling unit patrol creation and debug rendering. */
UCLASS(ClassGroup = (RTS), meta = (BlueprintSpawnableComponent))
class JUPITERPLUGIN_API UUnitPatrolComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UUnitPatrolComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** Called when the player presses the right mouse button. Returns true when the event was consumed. */
    bool HandleRightClickPressed(bool bAltDown);

    /** Called when the player releases the right mouse button. Returns true when the event was consumed. */
    bool HandleRightClickReleased(bool bAltDown);

    /** Called when the player presses the left mouse button (used to cancel pending patrol creation). */
    void HandleLeftClick(EInputEvent InputEvent);

    /** Returns true when a patrol is currently being edited. */
    bool IsCreatingPatrol() const { return bIsCreatingPatrol; }

protected:
    //------------------------------------ Initialization ------------------------------------
    void CacheDependencies();
    bool IsLocallyControlled() const;

    //------------------------------------ Input Handling ------------------------------------
    bool StartPatrolCreation();
    bool ConfirmPatrolCreation();
    bool TryAddPatrolPoint();
    void CancelPatrolCreation();

    //------------------------------------ Patrol Management ------------------------------------
    void RegisterPatrolRoute(const FPatrolRoute& NewRoute);
    void IssuePatrolCommands(FPatrolRoute& Route);
    void RemoveUnitsFromRoutes(const TArray<AActor*>& UnitsToRemove);
    void CleanupActiveRoutes();
    void RefreshSelectionCache(const TArray<AActor*>& SelectedActors);
    bool SelectionContainsRouteUnits(const FPatrolRoute& Route) const;
    bool TryGetCursorLocation(FVector& OutLocation) const;
    APlayerController* ResolveOwningController() const;

    //------------------------------------ Debug Drawing ------------------------------------
    void DrawPendingRoute() const;
    void DrawActiveRoutes() const;
    void DrawRoute(const TArray<FVector>& Points, bool bLoop, const FColor& Color) const;

    UFUNCTION()
    void HandleSelectionChanged(const TArray<AActor*>& SelectedActors);

    UFUNCTION()
    void HandleOrdersDispatched(const TArray<AActor*>& AffectedUnits, const FCommandData& CommandData);

protected:
    /** Cached reference to the owning player camera pawn. */
    UPROPERTY()
    TObjectPtr<APlayerCamera> CachedPlayerCamera;

    /** Cached reference to the selection component that provides cursor projection. */
    UPROPERTY()
    TObjectPtr<UUnitSelectionComponent> CachedSelectionComponent;

    /** Cached reference to the order component to react to manual orders. */
    UPROPERTY()
    TObjectPtr<UUnitOrderComponent> CachedOrderComponent;

    /** All confirmed patrol routes currently active. */
    UPROPERTY()
    TArray<FPatrolRoute> ActivePatrolRoutes;

    /** Currently selected units used for visualising patrols. */
    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> CachedSelection;

    /** Temporary patrol points while the player is editing a patrol. */
    UPROPERTY()
    TArray<FVector> PendingPatrolPoints;

    /** Units assigned to the patrol currently being edited. */
    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> PendingAssignedUnits;

    /** When true a patrol is currently being created. */
    UPROPERTY()
    bool bIsCreatingPatrol = false;

    /** Tracks whether the last alt+click should confirm on release even if alt is no longer pressed. */
    UPROPERTY()
    bool bPendingConfirmationClick = false;

    /** Cached cursor position used for drawing preview lines. */
    UPROPERTY()
    FVector CachedCursorLocation = FVector::ZeroVector;

    /** Tracks whether the cached cursor location is valid for preview rendering. */
    UPROPERTY()
    bool bHasCursorLocation = false;

    //------------------------------------ Settings ------------------------------------
    /** Distance threshold (in uu) used to detect loop closures. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Patrol", meta = (ClampMin = "0.0"))
    float LoopClosureThreshold = 200.f;

    /** Radius of the debug spheres drawn for patrol points. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Patrol", meta = (ClampMin = "0.0"))
    float DebugPointRadius = 40.f;

    /** Thickness of the debug lines used to visualise patrol paths. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Patrol", meta = (ClampMin = "0.0"))
    float DebugLineThickness = 3.f;

    /** Colour applied to pending patrol debug visuals. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Patrol")
    FColor PendingRouteColor = FColor::Yellow;

    /** Colour applied to confirmed patrol routes when one of their units is selected. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Patrol")
    FColor ActiveRouteColor = FColor::Cyan;

    /** When true, a line preview towards the current cursor location is drawn while editing a patrol. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Patrol")
    bool bDrawCursorPreview = true;
};

