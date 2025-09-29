#include "Components/SoldierManagerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Units/SoldierRts.h"


USoldierManagerComponent::USoldierManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USoldierManagerComponent::BeginPlay()
{
	UActorComponent::BeginPlay();

	TArray<AActor*> FoundSoldiers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASoldierRts::StaticClass(), FoundSoldiers);

	for (AActor* Actor : FoundSoldiers)
	{
		if (ASoldierRts* Soldier = Cast<ASoldierRts>(Actor))
			RegisterSoldier(Soldier);
	}
}

void USoldierManagerComponent::RegisterSoldier(ASoldierRts* Soldier)
{
	AddSoldierInternal(Soldier);
}

void USoldierManagerComponent::Server_RegisterSoldier_Implementation(ASoldierRts* Soldier)
{
	if (AddSoldierInternal(Soldier) && Soldiers.Num() > 0)
		SetComponentTickEnabled(true);
}

void USoldierManagerComponent::UnregisterSoldier(ASoldierRts* Soldier)
{
	RemoveSoldierInternal(Soldier);
}

void USoldierManagerComponent::PrintSoldierCount() const
{
	UE_LOG(LogTemp, Log, TEXT("SoldierManager: %d soldiers managed"), Soldiers.Num());
}

void USoldierManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ENetMode Mode = GetOwner()->GetNetMode();
	if (Mode == NM_Client)
		return;

	if (Soldiers.Num() == 0)
	{
		SetComponentTickEnabled(false);
		return;
	}
	
	if (DetectionInterval <= 0.f)
	{
		UpdateSoldierDetections(CurrentBucketIndex);
		CurrentBucketIndex = (CurrentBucketIndex + 1) % NumBuckets;
		return;
	}

	ElapsedTime += DeltaTime;
	if (ElapsedTime >= DetectionInterval)
	{
		ElapsedTime = 0.f;
		UpdateSoldierDetections(CurrentBucketIndex);
	}

	CurrentBucketIndex = (CurrentBucketIndex + 1) % NumBuckets;
}

void USoldierManagerComponent::UpdateSoldierDetections(int32 BucketIndex)
{
	if (NumBuckets <= 0)
		NumBuckets = 1;
	
	const int32 BucketSize = FMath::CeilToInt(static_cast<float>(Soldiers.Num()) / NumBuckets);

    const int32 StartIndex = BucketIndex * BucketSize;
    const int32 EndIndex   = FMath::Min(StartIndex + BucketSize, Soldiers.Num());

    for (int32 i = StartIndex; i < EndIndex; ++i)
    {
        ASoldierRts* Soldier = Soldiers[i];
        if (!IsValid(Soldier))
        {
            Soldiers.RemoveAt(i);
            --i;
            continue;
        }

        TArray<AActor*> NewEnemies;
        TArray<AActor*> NewAllies;

        const FVector SelfLoc = Soldier->GetActorLocation();
        const float EnemyRangeSq = FMath::Square(Soldier->GetAttackRange());
        const float AllyRangeSq  = FMath::Square(Soldier->GetAllyDetectionRange());

        for (ASoldierRts* Other : Soldiers)
        {
            if (!IsValid(Other) || Other == Soldier)
                continue;

            const float DistSq = FVector::DistSquared(SelfLoc, Other->GetActorLocation());

            if (Soldier->IsEnemyActor(Other) && DistSq <= EnemyRangeSq)
            {
                NewEnemies.Add(Other);
            }
            else if (Soldier->IsFriendlyActor(Other) && DistSq <= AllyRangeSq)
            {
                NewAllies.Add(Other);
            }
        }

        Soldier->ProcessDetectionResults(NewEnemies, NewAllies);
    }
}


bool USoldierManagerComponent::AddSoldierInternal(ASoldierRts* Soldier)
{
	if (!Soldier)
		return false;

	Soldiers.Remove(nullptr);

	if (Soldiers.Contains(Soldier))
		return false;

	Soldiers.Add(Soldier);
	return true;
}

bool USoldierManagerComponent::RemoveSoldierInternal(ASoldierRts* Soldier)
{
	if (!Soldier || Soldiers.Contains(Soldier))
		return false;

	Soldiers.Remove(nullptr);

	return Soldiers.Remove(Soldier) > 0;
}

