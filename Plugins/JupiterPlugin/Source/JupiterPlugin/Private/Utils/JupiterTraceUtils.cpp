#include "Utils/JupiterTraceUtils.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace JupiterTraceUtils
{
    bool ProjectPointToTerrain(UWorld* World, const FVector& SourceLocation, FVector& OutLocation, float TraceDistance)
    {
        if (!World)
        {
            return false;
        }

        FHitResult Hit;
        const FVector Start = SourceLocation + FVector::UpVector * TraceDistance;
        const FVector End = SourceLocation - FVector::UpVector * TraceDistance;
        if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
        {
            OutLocation = Hit.ImpactPoint;
            return true;
        }

        OutLocation = SourceLocation;
        return false;
    }

    bool TraceFromCamera(const APlayerController* Player, const FVector& StartOffset, FHitResult& OutHit, ECollisionChannel TraceChannel, const TArray<AActor*>& ActorsToIgnore)
    {
        if (!Player)
        {
            return false;
        }

        FVector WorldLocation = FVector::ZeroVector;
        FVector WorldDirection = FVector::ZeroVector;
        if (!Player->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
        {
            return false;
        }

        const FVector Start = WorldLocation + StartOffset;
        const FVector End = Start + WorldDirection * 1000000.f;

        FCollisionQueryParams Params(SCENE_QUERY_STAT(JupiterTraceUtils), false);
        Params.AddIgnoredActor(Player->GetPawn());
        for (AActor* IgnoredActor : ActorsToIgnore)
        {
            Params.AddIgnoredActor(IgnoredActor);
        }

        if (UWorld* World = Player->GetWorld())
        {
            return World->LineTraceSingleByChannel(OutHit, Start, End, TraceChannel, Params);
        }

        return false;
    }

    bool TraceUnderCursor(const APlayerController* Player, FHitResult& OutHit, ECollisionChannel TraceChannel, const TArray<AActor*>& ActorsToIgnore)
    {
        return TraceFromCamera(Player, FVector::ZeroVector, OutHit, TraceChannel, ActorsToIgnore);
    }
}

