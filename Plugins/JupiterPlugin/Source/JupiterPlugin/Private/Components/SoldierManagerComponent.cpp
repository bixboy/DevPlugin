#include "Components/SoldierManagerComponent.h"

#include "DrawDebugHelpers.h"
#include "Components/ActorComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "JupiterPlugin/Public/Units/SoldierRts.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoldierManager, Log, All);

USoldierManagerComponent::USoldierManagerComponent()
{
        PrimaryComponentTick.bCanEverTick = true;
        PrimaryComponentTick.bStartWithTickEnabled = true;
        PrimaryComponentTick.TickInterval = 0.0f;
}

void USoldierManagerComponent::BeginPlay()
{
        Super::BeginPlay();

        ManagedSoldiers.Reset();
        SoldierAccumulatedTime.Reset();
        RegisterExistingSoldiers();
}

void USoldierManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
        Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

        if (bRequiresCompaction)
        {
                CompactSoldierList();
        }

        for (float& PendingTime : SoldierAccumulatedTime)
        {
                PendingTime += DeltaTime;
        }

        if (HasAuthoritativeOwner())
        {
                ProcessSoldierBatch();
        }

        HandleDebugOutput(DeltaTime);
}

void USoldierManagerComponent::RegisterSoldier(ASoldierRts* Soldier)
{
        if (!IsValid(Soldier))
        {
                return;
        }

        if (bRequiresCompaction)
        {
                CompactSoldierList();
        }

        const int32 ExistingIndex = ManagedSoldiers.IndexOfByPredicate([Soldier](const TWeakObjectPtr<ASoldierRts>& Entry)
        {
                return Entry.Get() == Soldier;
        });

        if (ExistingIndex != INDEX_NONE)
        {
                SoldierAccumulatedTime[ExistingIndex] = 0.0f;
                Soldier->OnSoldierManagerRegistered(this);
                return;
        }

        ManagedSoldiers.Add(Soldier);
        SoldierAccumulatedTime.Add(0.0f);
        Soldier->OnSoldierManagerRegistered(this);
}

void USoldierManagerComponent::UnregisterSoldier(ASoldierRts* Soldier)
{
        if (!Soldier)
        {
                return;
        }

        const int32 ExistingIndex = ManagedSoldiers.IndexOfByPredicate([Soldier](const TWeakObjectPtr<ASoldierRts>& Entry)
        {
                return Entry.Get() == Soldier;
        });

        if (ExistingIndex != INDEX_NONE)
        {
                ManagedSoldiers[ExistingIndex] = nullptr;
                SoldierAccumulatedTime[ExistingIndex] = 0.0f;
                Soldier->OnSoldierManagerUnregistered(this);
                bRequiresCompaction = true;
        }
}

USoldierManagerComponent* USoldierManagerComponent::GetSoldierManagerFromPlayer(const APlayerController* PlayerController)
{
        if (!PlayerController)
        {
                return nullptr;
        }

        return PlayerController->FindComponentByClass<USoldierManagerComponent>();
}

USoldierManagerComponent* USoldierManagerComponent::GetSoldierManager(const UObject* WorldContextObject)
{
        if (!WorldContextObject)
        {
                return nullptr;
        }

        if (const UActorComponent* Component = Cast<UActorComponent>(WorldContextObject))
        {
                if (const AActor* Owner = Component->GetOwner())
                {
                        return GetSoldierManager(Owner);
                }
        }

        if (const APlayerController* PlayerController = Cast<APlayerController>(WorldContextObject))
        {
                return GetSoldierManagerFromPlayer(PlayerController);
        }

        if (const APawn* Pawn = Cast<APawn>(WorldContextObject))
        {
                if (USoldierManagerComponent* ManagerFromPawn = Pawn->FindComponentByClass<USoldierManagerComponent>())
                {
                        return ManagerFromPawn;
                }

                if (const APlayerController* OwningController = Cast<APlayerController>(Pawn->GetController()))
                {
                        if (USoldierManagerComponent* Manager = GetSoldierManagerFromPlayer(OwningController))
                        {
                                return Manager;
                        }
                }
        }

        if (const AActor* Actor = Cast<AActor>(WorldContextObject))
        {
                if (USoldierManagerComponent* ManagerFromActor = Actor->FindComponentByClass<USoldierManagerComponent>())
                {
                        return ManagerFromActor;
                }

                if (const APawn* OwningPawn = Cast<APawn>(Actor))
                {
                        if (const APlayerController* OwningController = Cast<APlayerController>(OwningPawn->GetController()))
                        {
                                if (USoldierManagerComponent* Manager = GetSoldierManagerFromPlayer(OwningController))
                                {
                                        return Manager;
                                }
                        }
                }
        }

        UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
        if (!World)
        {
                return nullptr;
        }

        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
        {
                if (const APlayerController* PlayerController = It->Get())
                {
                        if (USoldierManagerComponent* Manager = GetSoldierManagerFromPlayer(PlayerController))
                        {
                                return Manager;
                        }
                }
        }

        return nullptr;
}

void USoldierManagerComponent::RegisterExistingSoldiers()
{
        UWorld* World = GetWorld();
        if (!World)
        {
                return;
        }

        for (TActorIterator<ASoldierRts> It(World); It; ++It)
        {
                RegisterSoldier(*It);
        }
}

void USoldierManagerComponent::CompactSoldierList()
{
        for (int32 Index = ManagedSoldiers.Num() - 1; Index >= 0; --Index)
        {
                if (!ManagedSoldiers[Index].IsValid())
                {
                        ManagedSoldiers.RemoveAtSwap(Index, 1, false);
                        SoldierAccumulatedTime.RemoveAtSwap(Index, 1, false);
                }
        }

        if (ManagedSoldiers.Num() == 0)
        {
                CurrentGroupIndex = 0;
        }

        bRequiresCompaction = false;
}

bool USoldierManagerComponent::HasAuthoritativeOwner() const
{
        const AActor* OwnerActor = GetOwner();
        return OwnerActor == nullptr || OwnerActor->HasAuthority();
}

void USoldierManagerComponent::ProcessSoldierBatch()
{
        const int32 SoldierCount = ManagedSoldiers.Num();
        if (SoldierCount == 0)
        {
                CurrentGroupIndex = 0;
                return;
        }

        const int32 GroupCount = FMath::Max(UpdateGroupCount, 1);
        const int32 SoldiersPerGroup = FMath::CeilToInt(static_cast<float>(SoldierCount) / static_cast<float>(GroupCount));
        int32 StartIndex = SoldiersPerGroup * CurrentGroupIndex;
        if (StartIndex >= SoldierCount)
        {
                CurrentGroupIndex = 0;
                StartIndex = 0;
        }

        const int32 EndIndex = FMath::Min(StartIndex + SoldiersPerGroup, SoldierCount);

        for (int32 Index = StartIndex; Index < EndIndex && Index < ManagedSoldiers.Num(); ++Index)
        {
                ASoldierRts* Soldier = ManagedSoldiers[Index].Get();
                if (!IsValid(Soldier))
                {
                        SoldierAccumulatedTime[Index] = 0.0f;
                        bRequiresCompaction = true;
                        continue;
                }

                const float AccumulatedDelta = SoldierAccumulatedTime[Index];
                if (AccumulatedDelta > 0.0f)
                {
                        Soldier->UpdateAttackDetection(AccumulatedDelta);
                }

                SoldierAccumulatedTime[Index] = 0.0f;

                if (bDrawDebugForUpdatedSoldiers)
                {
                        DrawDebugForSoldier(Soldier);
                }
        }

        CurrentGroupIndex = (CurrentGroupIndex + 1) % FMath::Max(GroupCount, 1);
}

void USoldierManagerComponent::DrawDebugForSoldier(ASoldierRts* Soldier) const
{
        if (!Soldier || !GetWorld())
        {
                return;
        }

        const FColor DebugColor = DebugSphereColor.ToFColor(true);
        DrawDebugSphere(GetWorld(), Soldier->GetActorLocation(), DebugSphereRadius, 12, DebugColor, false, 0.1f);
}

void USoldierManagerComponent::HandleDebugOutput(float DeltaTime)
{
        if (!bLogManagedSoldierCount)
        {
                return;
        }

        TimeSinceLastDebugLog += DeltaTime;
        if (TimeSinceLastDebugLog < DebugLogInterval)
        {
                return;
        }

        TimeSinceLastDebugLog = 0.0f;

        const int32 SoldierCount = ManagedSoldiers.Num();
        UE_LOG(LogSoldierManager, Log, TEXT("SoldierManager managing %d soldiers"), SoldierCount);

        if (GEngine)
        {
                const uint64 DebugMessageKey = reinterpret_cast<uint64>(this);
                const FString DebugText = FString::Printf(TEXT("Managed Soldiers: %d"), SoldierCount);
                GEngine->AddOnScreenDebugMessage(DebugMessageKey, DebugLogInterval, FColor::Yellow, DebugText);
        }
}
