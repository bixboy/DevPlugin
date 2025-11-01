#pragma once

#include "CoreMinimal.h"
#include "PatrolRoute.generated.h"

class AActor;

/** Shared data structure describing a single patrol route. */
USTRUCT(BlueprintType)
struct FPatrolRoute
{
    GENERATED_BODY()

    /** All patrol points in world space order. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FVector> PatrolPoints;

    /** Units assigned to follow this patrol route. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<TObjectPtr<AActor>> UnitsInPatrol;

    /** When true the route should loop back to the first point. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsLoop = false;

    /** Colour applied to the spline visualisation for this route. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FColor RouteColor = FColor::Cyan;
};

