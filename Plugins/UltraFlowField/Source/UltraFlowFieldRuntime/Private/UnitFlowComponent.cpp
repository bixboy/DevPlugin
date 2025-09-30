#include "UnitFlowComponent.h"
#include "DynamicFlowFieldManager.h"
#include "EngineUtils.h"

UUnitFlowComponent::UUnitFlowComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UUnitFlowComponent::BeginPlay()
{
    Super::BeginPlay();
    ResolveManager();
}

void UUnitFlowComponent::ResolveManager()
{
    if (CachedManager.IsValid())
    {
        return;
    }

    if (FlowFieldManager.IsValid())
    {
        CachedManager = FlowFieldManager.Get();
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (TActorIterator<ADynamicFlowFieldManager> It(World); It; ++It)
    {
        CachedManager = *It;
        FlowFieldManager = CachedManager.Get();
        break;
    }
}

FVector UUnitFlowComponent::SampleDirection() const
{
    if (!CachedManager.IsValid())
    {
        const_cast<UUnitFlowComponent*>(this)->ResolveManager();
    }

    if (ADynamicFlowFieldManager* Manager = CachedManager.Get())
    {
        return Manager->GetFlowDirectionAt(GetOwner()->GetActorLocation(), PreferredLayer);
    }

    return FVector::ZeroVector;
}

void UUnitFlowComponent::SetPreferredLayer(EAgentLayer InLayer)
{
    PreferredLayer = InLayer;
}

