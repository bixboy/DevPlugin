#pragma once

#include "CoreMinimal.h"
#include "TickableWorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "RtsProductionSystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRTSProductionTaskChangedSignature, AController*, OwningController, const struct FRTSProductionTask&, Task, int32, QueueLength);

/** Production queue entry used by factories or buildings. */
USTRUCT(BlueprintType)
struct FRTSProductionTask
{
    GENERATED_BODY()

    /** Identifier of the production task (e.g. Unit archetype, upgrade id). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
    FName TaskId = NAME_None;

    /** Remaining time before the task completes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
    float RemainingTime = 0.f;

    /** Optional payload (unit class, item data asset, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
    TObjectPtr<UObject> Payload = nullptr;

    /** How many items should be produced once the timer completes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
    int32 Quantity = 1;

    /** Additional metadata for filtering and UI. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
    FGameplayTagContainer Tags;

    /** Internal progress (0..1) exposed to Blueprints. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
    float Progress = 0.f;

    /** Optional designer note. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
    FString DebugNote;

    void UpdateProgress(float DeltaTime, float SpeedMultiplier)
    {
        if (RemainingTime <= 0.f)
        {
            Progress = 1.f;
            return;
        }

        const float Delta = DeltaTime * SpeedMultiplier;
        RemainingTime = FMath::Max(0.f, RemainingTime - Delta);
        const float InitialDuration = FMath::Max(KINDA_SMALL_NUMBER, Progress <= 0.f ? RemainingTime + Delta : (RemainingTime + Delta) / (1.f - Progress));
        Progress = 1.f - (RemainingTime / InitialDuration);
    }

    bool IsComplete() const
    {
        return RemainingTime <= KINDA_SMALL_NUMBER;
    }
};

/** Tickable subsystem that simulates production queues per controller. */
UCLASS()
class JUPITERPLUGIN_API URTSProductionSystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;

    UFUNCTION(BlueprintCallable, Category = "RTS|Production")
    void EnqueueTask(AController* Controller, const FRTSProductionTask& Task);

    UFUNCTION(BlueprintCallable, Category = "RTS|Production")
    bool CancelTask(AController* Controller, FName TaskId);

    UFUNCTION(BlueprintCallable, Category = "RTS|Production")
    bool HasPendingTasks(AController* Controller) const;

    UFUNCTION(BlueprintCallable, Category = "RTS|Production")
    TArray<FRTSProductionTask> GetQueue(AController* Controller) const;

    UFUNCTION(BlueprintCallable, Category = "RTS|Production")
    void SetGlobalSpeedMultiplier(float NewSpeed);

    /** Allows designers to tweak production speed globally. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Production")
    float GlobalSpeedMultiplier = 1.f;

    /** Fired when a new task starts processing. */
    UPROPERTY(BlueprintAssignable, Category = "RTS|Production")
    FRTSProductionTaskChangedSignature OnTaskStarted;

    /** Fired when a task finishes processing. */
    UPROPERTY(BlueprintAssignable, Category = "RTS|Production")
    FRTSProductionTaskChangedSignature OnTaskFinished;

private:
    TArray<FRTSProductionTask>* FindQueue(AController* Controller);
    const TArray<FRTSProductionTask>* FindQueue(AController* Controller) const;

    void StartNextTask(AController* Controller, TArray<FRTSProductionTask>& Queue);

    TMap<TWeakObjectPtr<AController>, TArray<FRTSProductionTask>> Queues;
};

