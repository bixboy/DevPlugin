#pragma once

#include "CoreMinimal.h"
#include "PatrolData.generated.h"

/**
 * Quality levels for patrol route visualization.
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Patrol")
	TArray<FVector> PatrolPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Patrol")
	bool bIsLoop = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Patrol")
	FName RouteName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Patrol|Visuals")
	FLinearColor RouteColor = FLinearColor(0.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Patrol|Behavior", meta = (ClampMin = "0.0"))
	float WaitTimeAtWaypoints = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Patrol|Visuals")
	bool bShowDirectionArrows = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Patrol|Visuals")
	bool bShowWaypointNumbers = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Patrol|Visuals", meta = (ClampMin = "0"))
	int32 RenderPriority = 0;
};


/**
 * Visualization state for patrol routes.
 */
UENUM(BlueprintType)
enum class EPatrolVisualizationState : uint8
{
	Hidden,
	
	Preview,
	
	Active,
	
	Selected
};
