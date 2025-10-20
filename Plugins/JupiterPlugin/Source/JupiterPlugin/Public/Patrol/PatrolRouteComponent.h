#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Patrol/PatrolRouteTypes.h"
#include "PatrolRouteComponent.generated.h"

/** Component driving an actor along a configurable patrol route. */
UCLASS(ClassGroup = (RTS), meta = (BlueprintSpawnableComponent))
class JUPITERPLUGIN_API UPatrolRouteComponent : public UActorComponent
{
        GENERATED_BODY()

public:
        UPatrolRouteComponent();

        virtual void BeginPlay() override;
        virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

        /** Assigns a new patrol route to the component. */
        UFUNCTION(BlueprintCallable, Category = "Patrol")
        void AssignRoute(const FPatrolRoute& NewRoute);

        /** Clears the current patrol and stops movement. */
        UFUNCTION(BlueprintCallable, Category = "Patrol")
        void ClearRoute();

        /** Returns the currently active route. */
        UFUNCTION(BlueprintPure, Category = "Patrol")
        const FPatrolRoute& GetRoute() const { return ActiveRoute; }

        /** Draws the patrol path using debug lines. */
        UFUNCTION(BlueprintCallable, Category = "Patrol|Debug")
        void DrawRoute(UWorld* World, const FColor& LineColor, float Duration = 0.0f) const;

protected:
        /** Advances to the next waypoint depending on loop / ping-pong mode. */
        void AdvanceToNextPoint();

        /** Keeps the owning actor snapped to the route when assigned. */
        void EnsureOwnerInRoute();

protected:
        /** Active patrol definition driving movement. */
        UPROPERTY(VisibleAnywhere, Category = "Patrol")
        FPatrolRoute ActiveRoute;

        /** True when a patrol is currently active. */
        UPROPERTY(VisibleAnywhere, Category = "Patrol")
        bool bHasRoute = false;

        /** Current waypoint index. */
        UPROPERTY(VisibleAnywhere, Category = "Patrol")
        int32 CurrentPointIndex = 0;

        /** Direction multiplier used for ping-pong behaviour. */
        UPROPERTY(VisibleAnywhere, Category = "Patrol")
        int32 Direction = 1;

        /** Movement speed along the route (units per second). */
        UPROPERTY(EditAnywhere, Category = "Patrol")
        float PatrolSpeed = 400.0f;

        /** Distance threshold to consider we reached a waypoint. */
        UPROPERTY(EditAnywhere, Category = "Patrol")
        float AcceptanceRadius = 50.0f;
};
