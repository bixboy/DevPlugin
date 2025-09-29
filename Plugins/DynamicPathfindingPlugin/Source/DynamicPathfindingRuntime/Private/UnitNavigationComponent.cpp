#include "UnitNavigationComponent.h"
#include "DynamicPathManager.h"
#include "UnitAvoidanceComponent.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h"

UUnitNavigationComponent::UUnitNavigationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    MoveSpeed = 600.f;
    AvoidanceStrength = 1.0f;
    MaxSpeed = 900.f;

    AvoidanceComponent = nullptr;
}

void UUnitNavigationComponent::BeginPlay()
{
    Super::BeginPlay();

    AvoidanceComponent = GetOwner() ? GetOwner()->FindComponentByClass<UUnitAvoidanceComponent>() : nullptr;
    EnsureManagerReference();

    if (ADynamicPathManager* StrongManager = Manager.Get())
    {
        StrongManager->RegisterUnit(this);
        if (AvoidanceComponent)
        {
            StrongManager->RegisterAvoidanceComponent(AvoidanceComponent);
        }
    }
}

void UUnitNavigationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ADynamicPathManager* StrongManager = Manager.Get())
    {
        StrongManager->UnregisterUnit(this);
        if (AvoidanceComponent)
        {
            StrongManager->UnregisterAvoidanceComponent(AvoidanceComponent);
        }
    }

    Super::EndPlay(EndPlayReason);
}

void UUnitNavigationComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!GetOwner())
    {
        return;
    }

    if (!Manager.IsValid())
    {
        EnsureManagerReference();
    }

    ADynamicPathManager* StrongManager = Manager.Get();
    if (!StrongManager)
    {
        return;
    }

    const FVector FlowDirection = StrongManager->GetFlowDirectionAtLocation(GetOwner()->GetActorLocation());
    FVector DesiredVelocity = FlowDirection * MoveSpeed;

    if (AvoidanceComponent)
    {
        const FVector AvoidanceVelocity = AvoidanceComponent->ComputeAvoidance(DesiredVelocity);
        DesiredVelocity += AvoidanceVelocity * AvoidanceStrength;
    }

    DesiredVelocity = DesiredVelocity.GetClampedToMaxSize(MaxSpeed);

    CurrentVelocity = DesiredVelocity;

    ApplyMovement(CurrentVelocity, DeltaTime);
}

void UUnitNavigationComponent::EnsureManagerReference()
{
    if (Manager.IsValid())
    {
        return;
    }

    if (!GetWorld())
    {
        return;
    }

    for (TActorIterator<ADynamicPathManager> It(GetWorld()); It; ++It)
    {
        Manager = *It;
        break;
    }
}

void UUnitNavigationComponent::SetManager(ADynamicPathManager* InManager)
{
    Manager = InManager;
}

void UUnitNavigationComponent::ApplyMovement(const FVector& Velocity, float DeltaTime)
{
    if (AActor* OwnerActor = GetOwner())
    {
        const FVector Delta = Velocity * DeltaTime;
        OwnerActor->AddActorWorldOffset(Delta, true);
    }
}
