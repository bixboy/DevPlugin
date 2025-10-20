#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectPtr.h"
#include "PatrolRouteTypes.generated.h"

USTRUCT(BlueprintType)
struct FPatrolRoute
{
        GENERATED_BODY()

        /** Units assigned to this patrol. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
        TArray<TObjectPtr<AActor>> AssignedUnits;

        /** Ordered patrol points in world space. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
        TArray<FVector> PatrolPoints;

        /** Whether the patrol loops back to the start instead of ping-ponging. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
        bool bIsLoop = false;
};
