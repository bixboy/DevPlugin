#pragma once

#include "CoreMinimal.h"

class APlayerCamera;
class APlayerController;

namespace JupiterTraceUtils
{
    /** Projects a location onto the terrain by raycasting downwards. */
    JUPITERPLUGIN_API bool ProjectPointToTerrain(UWorld* World, const FVector& SourceLocation, FVector& OutLocation, float TraceDistance = 10000.f);

    /** Performs a trace from the cursor into the world. */
    JUPITERPLUGIN_API bool TraceUnderCursor(const APlayerController* Player, FHitResult& OutHit, ECollisionChannel TraceChannel = ECC_Visibility, const TArray<AActor*>& ActorsToIgnore = {});

    /** Performs a trace from the camera position using an offset. */
    JUPITERPLUGIN_API bool TraceFromCamera(const APlayerController* Player, const FVector& StartOffset, FHitResult& OutHit, ECollisionChannel TraceChannel = ECC_Visibility, const TArray<AActor*>& ActorsToIgnore = {});
}

