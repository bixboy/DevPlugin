#pragma once

#include "CoreMinimal.h"
#include "PatrolRouteTypes.generated.h"

class AActor;

/** Describes a patrol route assigned to one or more units. */
USTRUCT(BlueprintType)
struct JUPITERPLUGIN_API FPatrolRoute
{
        GENERATED_BODY()

        /** Units following this patrol route. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
        TArray<AActor*> AssignedUnits;

        /** Ordered world positions that make up the patrol path. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
        TArray<FVector> PatrolPoints;

        /** True when the route forms a closed loop instead of ping-pong motion. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
        bool bIsLoop = false;
};
