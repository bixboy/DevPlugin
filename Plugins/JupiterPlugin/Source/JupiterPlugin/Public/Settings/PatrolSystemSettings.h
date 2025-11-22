#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Data/PatrolData.h"
#include "PatrolSystemSettings.generated.h"

/**
 * Developer settings for the patrol system.
 * Accessible via Project Settings > Plugins > Patrol System
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Patrol System"))
class JUPITERPLUGIN_API UPatrolSystemSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPatrolSystemSettings();

	//~ Begin UDeveloperSettings Interface
	virtual FName GetCategoryName() const override;
	virtual FText GetSectionText() const override;
	//~ End UDeveloperSettings Interface

	/** Get the global settings instance */
	static const UPatrolSystemSettings* Get();

	// ============================================================
	// VISUAL SETTINGS
	// ============================================================

	/** Default quality level for patrol visualization */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals", meta = (DisplayName = "Default Quality Level"))
	EPatrolVisualQuality DefaultQualityLevel = EPatrolVisualQuality::High;

	/** Maximum distance at which patrol routes are visible */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals", meta = (ClampMin = "1000.0", DisplayName = "Max Visualization Distance"))
	float MaxVisualizationDistance = 15000.f;

	/** Distance at which to switch to LOD1 (medium detail) */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|LOD", meta = (ClampMin = "500.0", DisplayName = "LOD Distance 1 (Full -> Medium)"))
	float LODDistance1 = 3000.f;

	/** Distance at which to switch to LOD2 (low detail) */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|LOD", meta = (ClampMin = "1000.0", DisplayName = "LOD Distance 2 (Medium -> Low)"))
	float LODDistance2 = 6000.f;

	// ============================================================
	// COLOR SETTINGS
	// ============================================================

	/** Default color for active patrol routes */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|Colors", meta = (DisplayName = "Active Route Color"))
	FLinearColor ActiveRouteColor = FLinearColor(0.0f, 1.0f, 1.0f); // Cyan

	/** Color for routes in preview mode */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|Colors", meta = (DisplayName = "Preview Route Color"))
	FLinearColor PreviewRouteColor = FLinearColor(1.0f, 1.0f, 0.0f, 0.6f); // Yellow, semi-transparent

	/** Color for selected/highlighted routes */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|Colors", meta = (DisplayName = "Selected Route Color"))
	FLinearColor SelectedRouteColor = FLinearColor(1.0f, 0.5f, 0.0f); // Orange

	/** Color for waypoint markers */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|Colors", meta = (DisplayName = "Waypoint Color"))
	FLinearColor WaypointColor = FLinearColor(0.0f, 1.0f, 1.0f); // Cyan

	// ============================================================
	// LINE RENDERING SETTINGS
	// ============================================================

	/** Base thickness for patrol route lines */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|Lines", meta = (ClampMin = "1.0", ClampMax = "20.0", DisplayName = "Line Thickness"))
	float LineThickness = 4.f;

	/** Number of glow passes for line rendering */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|Lines", meta = (ClampMin = "0", ClampMax = "5", DisplayName = "Glow Passes"))
	int32 GlowPasses = 2;

	/** Size multiplier for glow effect */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|Lines", meta = (ClampMin = "0.0", ClampMax = "20.0", DisplayName = "Glow Size"))
	float GlowSize = 6.f;

	/** Alpha/opacity for glow effect */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|Lines", meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "Glow Alpha"))
	float GlowAlpha = 0.25f;

	/** Vertical offset for route visualization above ground */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|Lines", meta = (ClampMin = "0.0", ClampMax = "500.0", DisplayName = "Z Offset"))
	float VisualZOffset = 12.f;

	// ============================================================
	// WAYPOINT SETTINGS
	// ============================================================

	/** Radius of waypoint sphere markers */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|Waypoints", meta = (ClampMin = "10.0", ClampMax = "200.0", DisplayName = "Waypoint Radius"))
	float WaypointRadius = 40.f;

	/** Whether to show waypoint numbers by default */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|Waypoints", meta = (DisplayName = "Show Waypoint Numbers"))
	bool bShowWaypointNumbers = true;

	/** Scale of waypoint number text */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|Waypoints", meta = (ClampMin = "0.5", ClampMax = "5.0", DisplayName = "Number Text Scale"))
	float WaypointNumberScale = 1.5f;

	// ============================================================
	// ANIMATION SETTINGS
	// ============================================================

	/** Enable animated effects (pulse, flow, etc.) */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|Animation", meta = (DisplayName = "Enable Animations"))
	bool bEnableAnimations = true;

	/** Speed of color pulsing animation */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|Animation", meta = (ClampMin = "0.0", ClampMax = "10.0", DisplayName = "Color Pulse Speed"))
	float ColorPulseSpeed = 2.4f;

	/** Speed of line wave animation */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|Animation", meta = (ClampMin = "0.0", ClampMax = "10.0", DisplayName = "Line Wave Speed"))
	float LineWaveSpeed = 2.6f;

	/** Amplitude of line wave animation */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|Animation", meta = (ClampMin = "0.0", ClampMax = "10.0", DisplayName = "Line Wave Amplitude"))
	float LineWaveAmplitude = 1.6f;

	/** Speed of waypoint pulsing */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|Animation", meta = (ClampMin = "0.0", ClampMax = "10.0", DisplayName = "Waypoint Pulse Speed"))
	float PointPulseSpeed = 2.1f;

	// ============================================================
	// ARROW SETTINGS
	// ============================================================

	/** Distance between direction arrows (0 = disable arrows) */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|Arrows", meta = (ClampMin = "0.0", DisplayName = "Arrow Spacing (Unreal Units)"))
	float ArrowSpacing = 600.f;

	/** Size of direction arrow heads */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|Arrows", meta = (ClampMin = "10.0", ClampMax = "200.0", DisplayName = "Arrow Head Size"))
	float ArrowHeadSize = 36.f;

	/** Angle of arrow head (degrees) */
	UPROPERTY(Config, EditAnywhere, Category = "Visuals|Arrows", meta = (ClampMin = "5.0", ClampMax = "60.0", DisplayName = "Arrow Head Angle"))
	float ArrowHeadAngle = 22.5f;

	// ============================================================
	// PERFORMANCE SETTINGS
	// ============================================================

	/** Maximum number of routes to visualize simultaneously */
	UPROPERTY(Config, EditAnywhere, Category = "Performance", meta = (ClampMin = "1", ClampMax = "500", DisplayName = "Max Visible Routes"))
	int32 MaxVisibleRoutes = 50;

	/** Interval between visual updates (seconds) */
	UPROPERTY(Config, EditAnywhere, Category = "Performance", meta = (ClampMin = "0.01", ClampMax = "1.0", DisplayName = "Visual Refresh Interval"))
	float VisualRefreshInterval = 0.05f;

	/** Number of samples per spline segment for smooth curves */
	UPROPERTY(Config, EditAnywhere, Category = "Performance", meta = (ClampMin = "2", ClampMax = "20", DisplayName = "Spline Samples Per Segment"))
	int32 SplineSamplesPerSegment = 8;

	/** Enable frustum culling for routes outside camera view */
	UPROPERTY(Config, EditAnywhere, Category = "Performance", meta = (DisplayName = "Enable Frustum Culling"))
	bool bEnableFrustumCulling = true;

	/** Enable aggressive caching of spline geometry */
	UPROPERTY(Config, EditAnywhere, Category = "Performance", meta = (DisplayName = "Enable Geometry Caching"))
	bool bEnableGeometryCache = true;

	// ============================================================
	// DEBUG SETTINGS
	// ============================================================

	/** Show debug information for patrol system */
	UPROPERTY(Config, EditAnywhere, Category = "Debug", meta = (DisplayName = "Show Debug Info"))
	bool bShowDebugInfo = false;

	/** Show route bounds for debugging */
	UPROPERTY(Config, EditAnywhere, Category = "Debug", meta = (DisplayName = "Show Route Bounds"))
	bool bShowRouteBounds = false;
};
