#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/PatrolData.h"
#include "PatrolVisualizerComponent.generated.h"

class ULineBatchComponent;
class UCameraComponent;

/** Cache entry for optimized spline rendering */
USTRUCT()
struct FPatrolVisualizationCache
{
	GENERATED_BODY()

	/** Elevated waypoint positions (with Z offset applied) */
	TArray<FVector> ElevatedPoints;

	/** Smooth spline samples for rendering */
	TArray<FVector> SplineSamples;

	/** Hash of the geometry for cache invalidation */
	uint32 GeometryHash = 0;

	/** Cached loop state */
	bool bLoopCached = false;

	/** Current visualization state */
	EPatrolVisualizationState State = EPatrolVisualizationState::Active;

	/** Cached color */
	FLinearColor CachedColor = FLinearColor::White;

	/** Bounds of this route for culling */
	FBox RouteBounds = FBox(ForceInit);

	/** Last time this cache was accessed */
	float LastAccessTime = 0.f;

	void Reset()
	{
		ElevatedPoints.Reset();
		SplineSamples.Reset();
		GeometryHash = 0;
		bLoopCached = false;
		State = EPatrolVisualizationState::Active;
		CachedColor = FLinearColor::White;
		RouteBounds.Init();
		LastAccessTime = 0.f;
	}
};

/**
 * Component responsible for visualizing patrol routes.
 * Separated from UnitPatrolComponent to follow single-responsibility principle.
 * Handles all rendering, LOD, culling, and visual effects.
 */
UCLASS(ClassGroup=(RTS), meta=(BlueprintSpawnableComponent))
class JUPITERPLUGIN_API UPatrolVisualizerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPatrolVisualizerComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================
	// PUBLIC API
	// ============================================================

	/** Update visualization with new routes */
	UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
	void UpdateVisualization(const TArray<FPatrolRouteExtended>& Routes);

	/** Clear all visualizations */
	UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
	void ClearVisualization();

	/** Set the quality level for visualization */
	UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
	void SetVisualizationQuality(EPatrolVisualQuality Quality);

	/** Get current quality level */
	UFUNCTION(BlueprintPure, Category = "RTS|Patrol")
	EPatrolVisualQuality GetVisualizationQuality() const { return CurrentQuality; }

	/** Enable/disable visualization */
	UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
	void SetVisualizationEnabled(bool bEnabled);

	/** Check if visualization is enabled */
	UFUNCTION(BlueprintPure, Category = "RTS|Patrol")
	bool IsVisualizationEnabled() const { return bVisualizationEnabled; }

	/** Force refresh of all visuals */
	UFUNCTION(BlueprintCallable, Category = "RTS|Patrol")
	void ForceRefresh();

protected:
	// ============================================================
	// CORE RENDERING
	// ============================================================

	/** Ensure line batch component is created and ready */
	void EnsureLineBatchComponent();

	/** Main rendering update */
	void UpdateRouteVisuals(float DeltaSeconds);

	/** Render all active routes */
	void RenderActiveRoutes();

	/** Render a single route with caching */
	void RenderRouteCached(const FPatrolRouteExtended& Route, int32 CacheIndex);

	/** Draw the spline polyline with glow effects */
	void DrawSplinePolyline(const TArray<FVector>& Samples, const FLinearColor& Color, float Thickness, bool bAnimated);

	/** Draw waypoint markers */
	void DrawWaypoints(const TArray<FVector>& Points, const FLinearColor& Color, bool bShowNumbers);

	/** Draw direction arrows along the route */
	void DrawDirectionArrows(const TArray<FVector>& Samples, const FLinearColor& Color);

	// ============================================================
	// GEOMETRY GENERATION
	// ============================================================

	/** Build smooth spline samples from waypoints */
	void BuildSplineSamples(const TArray<FVector>& Points, bool bLoop, TArray<FVector>& OutSamples);

	/** Evaluate Catmull-Rom spline */
	static FVector EvaluateCatmullRom(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T);

	/** Calculate hash for geometry cache validation */
	static uint32 HashGeometry(const TArray<FVector>& Points, bool bLoop);

	/** Calculate bounds for a route */
	static FBox CalculateRouteBounds(const TArray<FVector>& Points);

	// ============================================================
	// LOD & CULLING
	// ============================================================

	/** Update LOD based on camera distance */
	void UpdateLOD();

	/** Get camera for LOD calculations */
	UCameraComponent* GetViewCamera() const;

	/** Check if route should be culled */
	bool ShouldCullRoute(const FBox& RouteBounds) const;

	/** Get distance to camera for LOD purposes */
	float GetDistanceToCamera() const;

	// ============================================================
	// HELPERS
	// ============================================================

	/** Get color based on route state and settings */
	FLinearColor GetRouteColor(const FPatrolRouteExtended& Route, EPatrolVisualizationState State) const;

	/** Apply animation to color */
	FLinearColor ApplyColorAnimation(const FLinearColor& BaseColor, float DeltaTime) const;

	/** Check if animations are enabled */
	bool ShouldAnimate() const;

	// ============================================================
	// DATA
	// ============================================================

	/** Current routes to visualize */
	UPROPERTY()
	TArray<FPatrolRouteExtended> CurrentRoutes;

	/** Visualization cache for each route */
	UPROPERTY()
	TArray<FPatrolVisualizationCache> VisualizationCache;

	/** Line batch component for rendering */
	UPROPERTY(Transient)
	TObjectPtr<ULineBatchComponent> LineBatchComponent = nullptr;

	/** Current visualization quality level */
	UPROPERTY()
	EPatrolVisualQuality CurrentQuality = EPatrolVisualQuality::High;

	/** Is visualization enabled */
	UPROPERTY()
	bool bVisualizationEnabled = true;

	/** Global animation time */
	float GlobalAnimationTime = 0.f;

	/** Timer for refresh throttling */
	float RefreshTimer = 0.f;

	/** Dirty flag for forced refresh */
	bool bVisualsDirty = true;

	/** Current LOD level (0 = highest quality) */
	int32 CurrentLODLevel = 0;

	/** Last known camera distance for LOD */
	float LastCameraDistance = 0.f;
};
