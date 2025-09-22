#include "Components/SoldierManagerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Units/SoldierRts.h"


USoldierManagerComponent::USoldierManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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
	if (!Soldier) return;

	if (!Soldiers.Contains(Soldier))
		Soldiers.Add(Soldier);
}


void USoldierManagerComponent::UnregisterSoldier(ASoldierRts* Soldier)
{
	if (!Soldier || Soldiers.Contains(Soldier))
		return;
	
	Soldiers.Remove(Soldier);
}

void USoldierManagerComponent::PrintSoldierCount() const
{
	UE_LOG(LogTemp, Log, TEXT("SoldierManager: %d soldiers managed"), Soldiers.Num());
}

void USoldierManagerComponent::Server_RegisterSoldier_Implementation(ASoldierRts* Soldier)
{
	if (!Soldier || Soldiers.Contains(Soldier))
		return;
	
	Soldiers.Add(Soldier);
}

void USoldierManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ENetMode Mode = GetOwner()->GetNetMode();
	if (Mode == NM_Client)
		return;

	ElapsedTime += DeltaTime;
	if (ElapsedTime >= DetectionInterval)
	{
		ElapsedTime = 0.f;
		UpdateSoldierDetections();
	}
}

void USoldierManagerComponent::UpdateSoldierDetections()
{
	TArray<ASoldierRts*> CurrentSoldiers = Soldiers;

	for (ASoldierRts* Soldier : CurrentSoldiers)
	{
		if (!IsValid(Soldier))
		{
			Soldiers.Remove(Soldier);
			continue;
		}

		// Préparer résultats
		TArray<AActor*> NewEnemies;
		TArray<AActor*> NewAllies;

		const FVector SelfLoc = Soldier->GetActorLocation();
		const float EnemyRangeSq = FMath::Square(Soldier->GetAttackRange());
		const float AllyRangeSq  = FMath::Square(Soldier->GetAllyDetectionRange());

		for (ASoldierRts* Other : CurrentSoldiers)
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

		Soldier->ProcessDetectionResults(NewEnemies,NewAllies);
	}
}

