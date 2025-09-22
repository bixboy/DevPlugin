#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SoldierManagerComponent.generated.h"

class ASoldierRts;
class APlayerController;

/**
 * Centralized manager responsible for distributing expensive RTS soldier updates.
 */
UCLASS(ClassGroup = (Custom), Blueprintable, meta = (BlueprintSpawnableComponent))
class JUPITERPLUGIN_API USoldierManagerComponent : public UActorComponent
{
        GENERATED_BODY()

public:
        USoldierManagerComponent();

        virtual void BeginPlay() override;
        virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

        /** Registers a soldier with the manager so it can receive distributed updates. */
        UFUNCTION(BlueprintCallable, Category = "Soldiers")
        void RegisterSoldier(ASoldierRts* Soldier);

        /** Removes a soldier from the manager. */
        UFUNCTION(BlueprintCallable, Category = "Soldiers")
        void UnregisterSoldier(ASoldierRts* Soldier);

        /** Returns all currently registered soldiers. */
        const TArray<TWeakObjectPtr<ASoldierRts>>& GetAllSoldiers() const { return ManagedSoldiers; }

        /** Returns the manager associated with the provided player controller, if any. */
        UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldiers")
        static USoldierManagerComponent* GetSoldierManagerFromPlayer(const APlayerController* PlayerController);

        /** Finds a soldier manager using the provided world context (player controller, pawn, world, ...). */
        UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Soldiers", meta = (WorldContext = "WorldContextObject"))
        static USoldierManagerComponent* GetSoldierManager(const UObject* WorldContextObject);

protected:
        /** Populates the manager with any soldiers that already exist in the world. */
        void RegisterExistingSoldiers();

        /** Removes invalid soldier references from the list. */
        void CompactSoldierList();

        /** Returns true when the component is allowed to run authoritative logic. */
        bool HasAuthoritativeOwner() const;

        /** Processes the next batch of soldiers scheduled for this frame. */
        void ProcessSoldierBatch();

        /** Draws optional debug helpers for the supplied soldier. */
        void DrawDebugForSoldier(ASoldierRts* Soldier) const;

        /** Optionally prints debug information. */
        void HandleDebugOutput(float DeltaTime);

protected:
        /** How many batches the manager divides the soldiers into. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Update", meta = (ClampMin = "1"))
        int32 UpdateGroupCount = 5;

        /** Enables drawing debug spheres for the soldiers that were updated during this frame. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
        bool bDrawDebugForUpdatedSoldiers = false;

        /** Enables regular debug logging for the number of managed soldiers. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
        bool bLogManagedSoldierCount = false;

        /** Duration between debug log entries when logging is enabled. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug", meta = (ClampMin = "0.1"))
        float DebugLogInterval = 2.0f;

        /** Radius of the debug spheres used to visualize updated soldiers. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug", meta = (ClampMin = "0.0"))
        float DebugSphereRadius = 25.0f;

        /** Color used for debug drawing. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
        FLinearColor DebugSphereColor = FLinearColor::Blue;

private:
        /** Array of managed soldiers. */
        TArray<TWeakObjectPtr<ASoldierRts>> ManagedSoldiers;

        /** Accumulated delta seconds for each managed soldier. */
        TArray<float> SoldierAccumulatedTime;

        /** Current update batch index. */
        int32 CurrentGroupIndex = 0;

        /** Time since the last debug log entry. */
        float TimeSinceLastDebugLog = 0.0f;

        /** Tracks whether the soldier list contains stale entries that require compaction. */
        bool bRequiresCompaction = false;
};
