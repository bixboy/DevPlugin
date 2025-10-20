#include "Patrol/PatrolRouteComponent.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"

UPatrolRouteComponent::UPatrolRouteComponent()
{
        PrimaryComponentTick.bCanEverTick = true;
}

void UPatrolRouteComponent::BeginPlay()
{
        Super::BeginPlay();

        EnsureOwnerInRoute();
}

void UPatrolRouteComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
        Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

        if (!bHasRoute || !ActiveRoute.PatrolPoints.IsValidIndex(CurrentPointIndex))
        {
                return;
        }

        AActor* Owner = GetOwner();
        if (!Owner)
        {
                return;
        }

        const FVector TargetPoint = ActiveRoute.PatrolPoints[CurrentPointIndex];
        FVector OwnerLocation = Owner->GetActorLocation();
        const float Distance = FVector::Dist(OwnerLocation, TargetPoint);

        if (Distance <= AcceptanceRadius)
        {
                AdvanceToNextPoint();
                return;
        }

        const FVector DirectionVector = (TargetPoint - OwnerLocation).GetSafeNormal();
        OwnerLocation += DirectionVector * PatrolSpeed * DeltaTime;
        Owner->SetActorLocation(OwnerLocation);
}

void UPatrolRouteComponent::AssignRoute(const FPatrolRoute& NewRoute)
{
        ActiveRoute = NewRoute;
        bHasRoute = ActiveRoute.PatrolPoints.Num() >= 2;
        CurrentPointIndex = 0;
        Direction = 1;
        EnsureOwnerInRoute();
}

void UPatrolRouteComponent::ClearRoute()
{
        ActiveRoute = FPatrolRoute();
        bHasRoute = false;
        CurrentPointIndex = 0;
        Direction = 1;
}

void UPatrolRouteComponent::DrawRoute(UWorld* World, const FColor& LineColor, float Duration) const
{
        if (!World || !bHasRoute || ActiveRoute.PatrolPoints.Num() < 2)
        {
                return;
        }

        const int32 PointCount = ActiveRoute.PatrolPoints.Num();
        for (int32 Index = 0; Index < PointCount; ++Index)
        {
                const FVector Current = ActiveRoute.PatrolPoints[Index];
                DrawDebugSphere(World, Current, 24.0f, 12, LineColor, false, Duration, 0, 1.0f);

                if (Index + 1 < PointCount)
                {
                        DrawDebugLine(World, Current, ActiveRoute.PatrolPoints[Index + 1], LineColor, false, Duration, 0, 2.0f);
                }
        }

        if (ActiveRoute.bIsLoop)
        {
                DrawDebugLine(World, ActiveRoute.PatrolPoints.Last(), ActiveRoute.PatrolPoints[0], LineColor, false, Duration, 0, 2.0f);
        }
}

void UPatrolRouteComponent::AdvanceToNextPoint()
{
        if (!bHasRoute || ActiveRoute.PatrolPoints.Num() < 2)
        {
                return;
        }

        const int32 PointCount = ActiveRoute.PatrolPoints.Num();
        if (ActiveRoute.bIsLoop)
        {
                CurrentPointIndex = (CurrentPointIndex + 1) % PointCount;
                return;
        }

        CurrentPointIndex += Direction;
        if (CurrentPointIndex >= PointCount)
        {
                Direction = -1;
                CurrentPointIndex = PointCount - 2;
        }
        else if (CurrentPointIndex < 0)
        {
                Direction = 1;
                CurrentPointIndex = 1;
        }
}

void UPatrolRouteComponent::EnsureOwnerInRoute()
{
        if (!bHasRoute || !ActiveRoute.PatrolPoints.IsValidIndex(0))
        {
                return;
        }

        if (AActor* Owner = GetOwner())
        {
                Owner->SetActorLocation(ActiveRoute.PatrolPoints[0]);
        }
}
