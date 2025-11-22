#pragma once

#include "CoreMinimal.h"
#include "PatrolData.generated.h"

/**
 * Quality levels for patrol route visualization.
 * Allows performance tuning on different platforms.
 */
UENUM(BlueprintType)
enum class EPatrolVisualQuality : uint8
{
	Low      UMETA(DisplayName = "Low - Simple Lines"),
	Medium   UMETA(DisplayName = "Medium - Lines + Waypoints"),
	High     UMETA(DisplayName = "High - Full Effects"),
	Ultra    UMETA(DisplayName = "Ultra - Maximum Quality")
};

/**
 * Extended patrol route structure with additional visual and behavioral properties.
 */
USTRUCT(BlueprintType)
struct FPatrolRouteExtended
{
	GENERATED_BODY()

	/** Array of patrol waypoints */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	TArray<FVector> PatrolPoints;

	/** Whether the patrol route loops back to the start */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	bool bIsLoop = false;

	/** Optional name for this patrol route */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	FName RouteName = NAME_None;

	/** Custom color for this specific route */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol|Visuals")
	FLinearColor RouteColor = FLinearColor(0.0f, 1.0f, 1.0f); // Cyan

	/** Time to wait at each waypoint (0 = no wait) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol|Behavior", meta = (ClampMin = "0.0"))
	float WaitTimeAtWaypoints = 0.f;

	/** Whether to show direction arrows along the route */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol|Visuals")
	bool bShowDirectionArrows = true;

	/** Whether to show waypoint numbers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol|Visuals")
	bool bShowWaypointNumbers = true;

	/** Priority for rendering (higher priority routes are rendered first) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol|Visuals", meta = (ClampMin = "0"))
	int32 RenderPriority = 0;
};

/**
 * Visualization state for patrol routes.
 */
UENUM(BlueprintType)
enum class EPatrolVisualizationState : uint8
{
	/** Route is hidden */
	Hidden,
	
	/** Route is in preview mode (being created/edited) */
	Preview,
	
	/** Route is active and confirmed */
	Active,
	
	/** Route is selected/highlighted */
	Selected
};
