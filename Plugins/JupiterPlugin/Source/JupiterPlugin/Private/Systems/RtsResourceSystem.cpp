#include "Systems/RtsResourceSystem.h"

#include "GameFramework/Controller.h"

void URTSResourceSystem::Deinitialize()
{
    PlayerResources.Empty();
    Super::Deinitialize();
}

int32 URTSResourceSystem::GetResourceAmount(AController* OwningController, FName ResourceId) const
{
    if (const FRTSResourceEntry* Entry = FindEntry(OwningController, ResourceId))
    {
        return Entry->Amount;
    }
    return 0;
}

int32 URTSResourceSystem::GetResourceCapacity(AController* OwningController, FName ResourceId) const
{
    if (const FRTSResourceEntry* Entry = FindEntry(OwningController, ResourceId))
    {
        return Entry->Capacity;
    }
    return -1;
}

void URTSResourceSystem::AddResource(AController* OwningController, FName ResourceId, int32 DeltaAmount, int32 OptionalNewCapacity)
{
    if (!OwningController || ResourceId.IsNone())
    {
        return;
    }

    FRTSResourceEntry& Entry = FindOrAddEntry(OwningController, ResourceId);

    if (OptionalNewCapacity >= 0)
    {
        Entry.Capacity = OptionalNewCapacity;
    }

    Entry.AddAmount(DeltaAmount);

    OnResourceChanged.Broadcast(OwningController, ResourceId, Entry.Amount, Entry.Capacity);
}

bool URTSResourceSystem::ConsumeResource(AController* OwningController, FName ResourceId, int32 AmountToConsume)
{
    if (!OwningController || ResourceId.IsNone())
    {
        return false;
    }

    FRTSResourceEntry& Entry = FindOrAddEntry(OwningController, ResourceId);
    if (!Entry.CanConsume(AmountToConsume))
    {
        return false;
    }

    Entry.AddAmount(-AmountToConsume);
    OnResourceChanged.Broadcast(OwningController, ResourceId, Entry.Amount, Entry.Capacity);
    return true;
}

void URTSResourceSystem::ResetResources(AController* OwningController)
{
    if (!OwningController)
    {
        return;
    }

    const TWeakObjectPtr<AController> Key(OwningController);
    if (TMap<FName, FRTSResourceEntry>* ResourceMap = PlayerResources.Find(Key))
    {
        for (const TPair<FName, FRTSResourceEntry>& Pair : *ResourceMap)
        {
            OnResourceChanged.Broadcast(OwningController, Pair.Key, 0, Pair.Value.Capacity);
        }
        ResourceMap->Empty();
    }
}

TArray<FRTSResourceEntry> URTSResourceSystem::GetAllResources(AController* OwningController) const
{
    TArray<FRTSResourceEntry> OutEntries;
    const TWeakObjectPtr<AController> Key(OwningController);
    if (const TMap<FName, FRTSResourceEntry>* ResourceMap = PlayerResources.Find(Key))
    {
        ResourceMap->GenerateValueArray(OutEntries);
    }
    return OutEntries;
}

FRTSResourceEntry& URTSResourceSystem::FindOrAddEntry(AController* OwningController, FName ResourceId)
{
    const TWeakObjectPtr<AController> Key(OwningController);
    TMap<FName, FRTSResourceEntry>& ResourceMap = PlayerResources.FindOrAdd(Key);
    FRTSResourceEntry& Entry = ResourceMap.FindOrAdd(ResourceId);
    Entry.ResourceId = ResourceId;
    return Entry;
}

const FRTSResourceEntry* URTSResourceSystem::FindEntry(AController* OwningController, FName ResourceId) const
{
    const TWeakObjectPtr<AController> Key(OwningController);
    if (const TMap<FName, FRTSResourceEntry>* ResourceMap = PlayerResources.Find(Key))
    {
        return ResourceMap->Find(ResourceId);
    }
    return nullptr;
}

