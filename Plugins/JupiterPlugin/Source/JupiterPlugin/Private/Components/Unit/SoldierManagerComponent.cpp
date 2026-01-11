#include "Components/Unit/SoldierManagerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Units/SoldierRts.h"

USoldierManagerComponent::USoldierManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true; 
}

void USoldierManagerComponent::BeginPlay()
{
    Super::BeginPlay();

    // RECOMMENDED: Attach this component to the GameState for global management, 
    // rather than the PlayerController, to ensure it persists across possession changes and handles all soldiers server-wide.

    if (GetOwner()->HasAuthority())
    {
        TArray<AActor*> FoundSoldiers;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASoldierRts::StaticClass(), FoundSoldiers);

        for (AActor* Actor : FoundSoldiers)
        {
            if (ASoldierRts* Soldier = Cast<ASoldierRts>(Actor))
            {
                RegisterSoldier(Soldier);
            }
        }
    }
}

void USoldierManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (GetOwner()->GetNetMode() == NM_Client || Soldiers.Num() == 0)
        return;
	
    const int32 SafeBuckets = FMath::Max(1, TargetBuckets);
    const float TimePerBucket = FullLoopDuration / static_cast<float>(SafeBuckets);

    TimeSinceLastBucket += DeltaTime;

    if (TimeSinceLastBucket >= TimePerBucket)
    {
        ProcessDetectionBucket(CurrentBucketIndex);
        
        CurrentBucketIndex = (CurrentBucketIndex + 1) % SafeBuckets;
        TimeSinceLastBucket -= TimePerBucket;
    }
}

void USoldierManagerComponent::ProcessDetectionBucket(int32 BucketIndex)
{
    if (Soldiers.Num() == 0)
    	return;

    const int32 TotalSoldiers = Soldiers.Num();
    const int32 SafeBuckets = FMath::Max(1, TargetBuckets);
    const int32 BucketSize = FMath::CeilToInt(static_cast<float>(TotalSoldiers) / static_cast<float>(SafeBuckets));
    
    const int32 StartIndex = BucketIndex * BucketSize;
    const int32 EndIndex = FMath::Min(StartIndex + BucketSize, TotalSoldiers);

    if (StartIndex >= TotalSoldiers)
    	return;

    TArray<AActor*> FoundEnemies;
    TArray<AActor*> FoundAllies;
    FoundEnemies.Reserve(20);
    FoundAllies.Reserve(20);

    for (int32 i = StartIndex; i < EndIndex; ++i)
    {
        ASoldierRts* Subject = Soldiers[i];

        if (!IsValid(Subject))
        {
            Soldiers.RemoveAtSwap(i);
            --i;
        	
            continue;
        }

        const FVector SubjectLoc = Subject->GetActorLocation();
        const float EnemyRangeSq = FMath::Square(Subject->GetAttackRange());
        const float AllyRangeSq = FMath::Square(Subject->GetAllyDetectionRange());
        
        FoundEnemies.Reset();
        FoundAllies.Reset();
    	
        for (ASoldierRts* Other : Soldiers)
        {
            if (Other == Subject || !IsValid(Other))
            	continue;

            const float DistSq = FVector::DistSquared(SubjectLoc, Other->GetActorLocation());

            if (DistSq <= EnemyRangeSq && Subject->IsEnemyActor(Other))
            {
                FoundEnemies.Add(Other);
            }
            else if (DistSq <= AllyRangeSq && Subject->IsFriendlyActor(Other))
            {
                FoundAllies.Add(Other);
            }
        }

        Subject->ProcessDetectionResults(FoundEnemies, FoundAllies);
    }
}

// --------------------------------------------------------
// REGISTRATION
// --------------------------------------------------------

void USoldierManagerComponent::RegisterSoldier(ASoldierRts* Soldier)
{
    if (GetOwner()->HasAuthority())
    {
        if (IsValid(Soldier) && !Soldiers.Contains(Soldier))
        {
            Soldiers.Add(Soldier);
        }
    }
    else
    {
        Server_RegisterSoldier(Soldier);
    }
}

void USoldierManagerComponent::UnregisterSoldier(ASoldierRts* Soldier)
{
    if (IsValid(Soldier))
    {
        Soldiers.RemoveSwap(Soldier);
    }
}

void USoldierManagerComponent::Server_RegisterSoldier_Implementation(ASoldierRts* Soldier)
{
    RegisterSoldier(Soldier);
}