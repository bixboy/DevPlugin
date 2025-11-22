#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "Data/AiData.h"
#include "Data/PatrolData.h"
#include "UnitPatrolComponent.generated.h"

class UPatrolVisualizerComponent;
class UUnitOrderComponent;
class UUnitSelectionComponent;

/** Structure représentant une route de patrouille (répliquée). */
USTRUCT(BlueprintType)
struct FPatrolRoute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FVector> PatrolPoints;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsLoop = false;
};

/**
 * Component managing patrol logic and integration.
 * Delegates visualization to UPatrolVisualizerComponent for better separation of concerns.
 */
UCLASS(ClassGroup=(RTS), meta=(BlueprintSpawnableComponent))
class JUPITERPLUGIN_API UUnitPatrolComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUnitPatrolComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ============================================================
	// INPUT HANDLING
	// ============================================================

	/** Handles left mouse button events forwarded by the player camera. */
	bool HandleLeftClick(EInputEvent InputEvent);

	/** Handles the right mouse button being pressed. */
	bool HandleRightClickPressed(bool bAltDown);

	/** Handles the right mouse button being released. */
	bool HandleRightClickReleased(bool bAltDown);

	// ============================================================
	// PATROL PREVIEW SYSTEM
	// ============================================================

	/** Start creating a patrol route preview */
	UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
	void StartPatrolPreview(const FVector& StartPoint);

	/** Add a waypoint to the preview */
	UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
	void AddPreviewWaypoint(const FVector& Point);

	/** Update the last waypoint position (for mouse following) */
	UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
	void UpdatePreviewCursor(const FVector& CursorPosition);

	/** Commit the preview as an active route */
	UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
	void CommitPatrolPreview(bool bLoop);

	/** Cancel the current preview */
	UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
	void CancelPatrolPreview();

	/** Check if currently in preview mode */
	UFUNCTION(BlueprintPure, Category = "RTS|Patrol")
	bool IsInPreviewMode() const { return bIsCreatingPreview; }

	// ============================================================
	// PATROL MANAGEMENT
	// ============================================================

	/** Get current active patrol routes */
	UFUNCTION(BlueprintPure, Category = "RTS|Patrol")
	const TArray<FPatrolRoute>& GetActiveRoutes() const { return ActivePatrolRoutes; }

	/** Manually set patrol routes (for testing/debugging) */
	UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
	void SetPatrolRoutes(const TArray<FPatrolRoute>& NewRoutes);

	/** Clear all patrol routes */
	UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
	void ClearAllRoutes();

protected:
	// ============================================================
	// INTERNAL HELPERS
	// ============================================================

	/** Ensure visualizer component exists */
	void EnsureVisualizerComponent();

	/** Update visualization to match current routes */
	void UpdateVisualization();

	/** Convert FPatrolRoute to FPatrolRouteExtended for visualization */
	static FPatrolRouteExtended ConvertToExtended(const FPatrolRoute& Route, const FLinearColor& Color = FLinearColor::Blue);

	/** Check if this component is locally controlled */
	bool IsLocallyControlled() const;

	/** Check if this component has authority */
	bool HasAuthority() const;

	// ============================================================
	// EVENT HANDLERS
	// ============================================================

	/** Handle orders dispatched to units */
	UFUNCTION()
	void HandleOrdersDispatched(const TArray<AActor*>& AffectedUnits, const FCommandData& IssuedCommand);

	/** Refresh routes based on current selection */
	void RefreshRoutesFromSelection();

	/** Apply new routes (with equivalence check) */
	void ApplyRoutes(const TArray<FPatrolRoute>& NewRoutes);

	// ============================================================
	// DATA - REPLICATED
	// ============================================================

	/** Active patrol routes (replicated to clients) */
	UPROPERTY(ReplicatedUsing=OnRep_ActivePatrolRoutes)
	TArray<FPatrolRoute> ActivePatrolRoutes;

	UFUNCTION()
	void OnRep_ActivePatrolRoutes();

	// ============================================================
	// DATA - LOCAL
	// ============================================================

	/** Visualizer component for rendering routes */
	UPROPERTY(Transient)
	TObjectPtr<UPatrolVisualizerComponent> PatrolVisualizer = nullptr;

	/** Preview route being created */
	UPROPERTY()
	FPatrolRouteExtended PreviewRoute;

	/** Is currently creating a preview route */
	UPROPERTY()
	bool bIsCreatingPreview = false;

	/** Dependencies */
	UPROPERTY()
	TObjectPtr<UUnitOrderComponent> OrderComponent = nullptr;

	UPROPERTY()
	TObjectPtr<UUnitSelectionComponent> SelectionComponent = nullptr;

	// ============================================================
	// SETTINGS (exposed for per-instance customization)
	// ============================================================

	/** Color for this unit's active routes */
	UPROPERTY(EditAnywhere, Category = "RTS|Patrol|Visuals")
	FLinearColor RouteColor = FLinearColor(0.0f, 1.0f, 1.0f);

	/** Whether to auto-create visualizer component */
	UPROPERTY(EditAnywhere, Category = "RTS|Patrol")
	bool bAutoCreateVisualizer = true;
};
