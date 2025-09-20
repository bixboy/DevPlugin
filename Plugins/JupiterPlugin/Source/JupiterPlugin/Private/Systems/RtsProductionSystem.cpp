#include "Systems/RtsProductionSystem.h"

#include "GameFramework/Controller.h"
#include "Stats/Stats.h"

void URTSProductionSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void URTSProductionSystem::Deinitialize()
{
    Queues.Empty();
    Super::Deinitialize();
}

void URTSProductionSystem::Tick(float DeltaTime)
{
    TArray<TWeakObjectPtr<AController>> InvalidControllers;

    for (TPair<TWeakObjectPtr<AController>, TArray<FRTSProductionTask>>& Pair : Queues)
    {
        if (!Pair.Key.IsValid())
        {
            InvalidControllers.Add(Pair.Key);
            continue;
        }

        if (Pair.Value.IsEmpty())
        {
            continue;
        }

        FRTSProductionTask& CurrentTask = Pair.Value[0];
        CurrentTask.UpdateProgress(DeltaTime, GlobalSpeedMultiplier);

        if (CurrentTask.IsComplete())
        {
            if (AController* Controller = Pair.Key.Get())
            {
                OnTaskFinished.Broadcast(Controller, CurrentTask, Pair.Value.Num());
                Pair.Value.RemoveAt(0);
                StartNextTask(Controller, Pair.Value);
            }
        }
    }

    for (const TWeakObjectPtr<AController>& Key : InvalidControllers)
    {
        Queues.Remove(Key);
    }
}

TStatId URTSProductionSystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(URTSProductionSystem, STATGROUP_Tickables);
}

void URTSProductionSystem::EnqueueTask(AController* Controller, const FRTSProductionTask& Task)
{
    if (!Controller || Task.TaskId.IsNone())
    {
        return;
    }

    const TWeakObjectPtr<AController> Key(Controller);
    TArray<FRTSProductionTask>& Queue = Queues.FindOrAdd(Key);
    const bool bWasIdle = Queue.IsEmpty();
    Queue.Add(Task);

    if (bWasIdle)
    {
        StartNextTask(Controller, Queue);
    }
}

bool URTSProductionSystem::CancelTask(AController* Controller, FName TaskId)
{
    if (TaskId.IsNone())
    {
        return false;
    }

    if (TArray<FRTSProductionTask>* Queue = FindQueue(Controller))
    {
        for (int32 Index = 0; Index < Queue->Num(); ++Index)
        {
            if ((*Queue)[Index].TaskId == TaskId)
            {
                const bool bWasProcessing = (Index == 0);
                const FRTSProductionTask RemovedTask = (*Queue)[Index];
                Queue->RemoveAt(Index);

                if (bWasProcessing && Controller)
                {
                    StartNextTask(Controller, *Queue);
                }

                if (Controller)
                {
                    OnTaskFinished.Broadcast(Controller, RemovedTask, Queue->Num());
                }
                return true;
            }
        }
    }

    return false;
}

bool URTSProductionSystem::HasPendingTasks(AController* Controller) const
{
    if (const TArray<FRTSProductionTask>* Queue = FindQueue(Controller))
    {
        return !Queue->IsEmpty();
    }
    return false;
}

TArray<FRTSProductionTask> URTSProductionSystem::GetQueue(AController* Controller) const
{
    if (const TArray<FRTSProductionTask>* Queue = FindQueue(Controller))
    {
        return *Queue;
    }
    return {};
}

void URTSProductionSystem::SetGlobalSpeedMultiplier(float NewSpeed)
{
    GlobalSpeedMultiplier = FMath::Max(0.01f, NewSpeed);
}

TArray<FRTSProductionTask>* URTSProductionSystem::FindQueue(AController* Controller)
{
    const TWeakObjectPtr<AController> Key(Controller);
    return Queues.Find(Key);
}

const TArray<FRTSProductionTask>* URTSProductionSystem::FindQueue(AController* Controller) const
{
    const TWeakObjectPtr<AController> Key(Controller);
    return Queues.Find(Key);
}

void URTSProductionSystem::StartNextTask(AController* Controller, TArray<FRTSProductionTask>& Queue)
{
    if (!Controller || Queue.IsEmpty())
    {
        return;
    }

    FRTSProductionTask& CurrentTask = Queue[0];
    CurrentTask.Progress = 0.f;
    OnTaskStarted.Broadcast(Controller, CurrentTask, Queue.Num());
}

