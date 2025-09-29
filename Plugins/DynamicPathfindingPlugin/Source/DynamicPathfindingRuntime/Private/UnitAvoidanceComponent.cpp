#include "UnitAvoidanceComponent.h"
#include "DynamicPathManager.h"
#include "GameFramework/Actor.h"

UUnitAvoidanceComponent::UUnitAvoidanceComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;

    NeighborRadius = 200.f;
    SeparationWeight = 1.5f;
    MaxAvoidanceForce = 600.f;
    RVOWeight = 0.6f;

    SpatialCellKey = FIntVector::ZeroValue;
}

void UUnitAvoidanceComponent::BeginPlay()
{
    Super::BeginPlay();

    LastWorldLocation = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
}

void UUnitAvoidanceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ADynamicPathManager* StrongManager = Manager.Get())
    {
        StrongManager->UnregisterAvoidanceComponent(this);
    }

    Super::EndPlay(EndPlayReason);
}

void UUnitAvoidanceComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!GetOwner())
    {
        return;
    }

    const FVector CurrentLocation = GetOwner()->GetActorLocation();
    CurrentVelocity = (CurrentLocation - LastWorldLocation) / FMath::Max(DeltaTime, UE_SMALL_NUMBER);
    LastWorldLocation = CurrentLocation;

    if (ADynamicPathManager* StrongManager = Manager.Get())
    {
        StrongManager->UpdateSpatialEntry(this, CurrentLocation);
    }
}

FVector UUnitAvoidanceComponent::ComputeAvoidance(const FVector& DesiredVelocity) const
{
    if (ADynamicPathManager* StrongManager = Manager.Get())
    {
        return StrongManager->ComputeAvoidanceForComponent(this, DesiredVelocity);
    }

    return FVector::ZeroVector;
}

void UUnitAvoidanceComponent::SetManager(ADynamicPathManager* InManager)
{
    Manager = InManager;
}

void UUnitAvoidanceComponent::UpdateSpatialCellKey(const FIntVector& NewKey)
{
    SpatialCellKey = NewKey;
}
