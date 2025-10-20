#include "Patrol/PatrolRouteComponent.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"

UPatrolRouteComponent::UPatrolRouteComponent()
{
        PrimaryComponentTick.bCanEverTick = true;
        PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UPatrolRouteComponent::BeginPlay()
{
        Super::BeginPlay();
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
        const FVector ToTarget = TargetPoint - OwnerLocation;
        const float Distance = ToTarget.Size();

        if (Distance <= AcceptanceRadius)
        {
                AdvanceToNextPoint();
                return;
        }

        if (!ToTarget.IsNearlyZero())
        {
                const FVector Direction = ToTarget / Distance;
                OwnerLocation += Direction * PatrolSpeed * DeltaTime;
                Owner->SetActorLocation(OwnerLocation, true);
        }
}

void UPatrolRouteComponent::AssignRoute(const FPatrolRoute& NewRoute)
{
        ActiveRoute = NewRoute;
        EnsureOwnerInRoute();
        bHasRoute = ActiveRoute.PatrolPoints.Num() >= 2;
        bReverseTraversal = false;
        CurrentPointIndex = bHasRoute ? 0 : INDEX_NONE;
}

void UPatrolRouteComponent::ClearRoute()
{
        ActiveRoute = FPatrolRoute();
        bHasRoute = false;
        bReverseTraversal = false;
        CurrentPointIndex = INDEX_NONE;
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
                DrawDebugSphere(World, Current, 32.0f, 12, LineColor, false, Duration, 0, 1.5f);

                if (Index < PointCount - 1)
                {
                        DrawDebugLine(World, Current, ActiveRoute.PatrolPoints[Index + 1], LineColor, false, Duration, 0, 2.0f);
                }

                if (ActiveRoute.bIsLoop && Index == PointCount - 1)
                {
                        DrawDebugLine(World, Current, ActiveRoute.PatrolPoints[0], LineColor, false, Duration, 0, 2.0f);
                }
        }
}

void UPatrolRouteComponent::AdvanceToNextPoint()
{
        if (!bHasRoute || ActiveRoute.PatrolPoints.Num() < 2)
        {
                return;
        }

        if (ActiveRoute.bIsLoop)
        {
                CurrentPointIndex = (CurrentPointIndex + 1) % ActiveRoute.PatrolPoints.Num();
                return;
        }

        if (!bReverseTraversal)
        {
                ++CurrentPointIndex;
                if (CurrentPointIndex >= ActiveRoute.PatrolPoints.Num())
                {
                        bReverseTraversal = true;
                        CurrentPointIndex = ActiveRoute.PatrolPoints.Num() - 2;
                }
        }
        else
        {
                --CurrentPointIndex;
                if (CurrentPointIndex < 0)
                {
                        bReverseTraversal = false;
                        CurrentPointIndex = 1;
                }
        }
}

void UPatrolRouteComponent::EnsureOwnerInRoute()
{
        AActor* Owner = GetOwner();
        if (Owner && !ActiveRoute.AssignedUnits.Contains(Owner))
        {
                ActiveRoute.AssignedUnits.Add(Owner);
        }
}
