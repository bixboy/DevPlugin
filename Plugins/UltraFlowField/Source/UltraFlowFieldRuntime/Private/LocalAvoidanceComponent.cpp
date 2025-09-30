#include "LocalAvoidanceComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "EngineUtils.h"

ULocalAvoidanceComponent::ULocalAvoidanceComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.f;
}

void ULocalAvoidanceComponent::BeginPlay()
{
    Super::BeginPlay();
    TimeAccumulator = 0.f;
}

void ULocalAvoidanceComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    TimeAccumulator += DeltaTime;
    if (TimeAccumulator >= Settings.UpdateInterval)
    {
        UpdateAvoidance();
        TimeAccumulator = 0.f;
    }
}

void ULocalAvoidanceComponent::UpdateAvoidance()
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FVector Accumulated = FVector::ZeroVector;
    int32 NeighborCount = 0;

    const FVector OwnerLocation = Owner->GetActorLocation();

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Other = *It;
        if (Other == Owner)
        {
            continue;
        }

        const FVector OtherLocation = Other->GetActorLocation();
        const float DistSq = FVector::DistSquared(OwnerLocation, OtherLocation);
        if (DistSq <= FMath::Square(Settings.QueryRadius))
        {
            const FVector ToOwner = OwnerLocation - OtherLocation;
            if (!ToOwner.IsNearlyZero())
            {
                Accumulated += ToOwner.GetSafeNormal() / FMath::Max(1.f, DistSq);
                ++NeighborCount;
            }
        }
    }

    if (NeighborCount > 0)
    {
        CachedAvoidance = Accumulated.GetClampedToMaxSize(Settings.MaxForce);
    }
    else
    {
        CachedAvoidance = FVector::ZeroVector;
    }
}

