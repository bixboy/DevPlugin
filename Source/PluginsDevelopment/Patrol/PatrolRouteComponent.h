#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PatrolRouteTypes.h"
#include "PatrolRouteComponent.generated.h"

/** Actor component that drives an owner along an assigned patrol route. */
UCLASS(ClassGroup = (Patrol), meta = (BlueprintSpawnableComponent))
class PLUGINSDEVELOPMENT_API UPatrolRouteComponent : public UActorComponent
{
        GENERATED_BODY()

public:
        UPatrolRouteComponent();

        virtual void BeginPlay() override;
        virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

        /** Assigns a new patrol route to this component. */
        UFUNCTION(BlueprintCallable, Category = "Patrol")
        void AssignRoute(const FPatrolRoute& NewRoute);

        /** Clears the current patrol route. */
        UFUNCTION(BlueprintCallable, Category = "Patrol")
        void ClearRoute();

        /** Returns true if a patrol route is active. */
        UFUNCTION(BlueprintPure, Category = "Patrol")
        bool HasRoute() const { return bHasRoute; }

        /** Provides read-only access to the current route. */
        const FPatrolRoute& GetRoute() const { return ActiveRoute; }

        /** Draws the patrol using debug helpers. */
        void DrawRoute(UWorld* World, const FColor& LineColor, float Duration = 0.0f) const;

private:
        /** Advances the current target index depending on loop or ping-pong behaviour. */
        void AdvanceToNextPoint();

        /** Ensures the assigned units array contains the owner. */
        void EnsureOwnerInRoute();

private:
        /** Current patrol definition. */
        UPROPERTY(VisibleAnywhere, Category = "Patrol")
        FPatrolRoute ActiveRoute;

        /** Whether the component is following the route in reverse (for ping-pong). */
        bool bReverseTraversal = false;

        /** Whether a route is currently assigned. */
        bool bHasRoute = false;

        /** Index of the point currently targeted. */
        int32 CurrentPointIndex = 0;

private:
        /** Movement speed while following patrol points (units per second). */
        UPROPERTY(EditAnywhere, Category = "Patrol")
        float PatrolSpeed = 400.0f;

        /** Acceptance radius for switching to the next patrol point. */
        UPROPERTY(EditAnywhere, Category = "Patrol")
        float AcceptanceRadius = 75.0f;
};
