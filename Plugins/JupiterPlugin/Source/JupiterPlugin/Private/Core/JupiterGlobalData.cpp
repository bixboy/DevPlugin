#include "Core/JupiterGlobalData.h"
#include "Components/Placement/PresetManagerComponent.h"
#include "Components/Patrol/UnitPatrolComponent.h"
#include "EngineUtils.h"

AJupiterGlobalData::AJupiterGlobalData()
{
	PrimaryActorTick.bCanEverTick = false;
	bAlwaysRelevant = true;
	bReplicates = true;

	PresetManager = CreateDefaultSubobject<UPresetManagerComponent>(TEXT("PresetManager"));
	PresetManager->SetIsReplicated(true);

	PatrolComponent = CreateDefaultSubobject<UUnitPatrolComponent>(TEXT("PatrolComponent"));
	PatrolComponent->SetIsReplicated(true);
}

AJupiterGlobalData* AJupiterGlobalData::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
		return nullptr;

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
		return nullptr;

	for (TActorIterator<AJupiterGlobalData> It(World); It; ++It)
	{
		return *It;
	}

	return nullptr;
}

AJupiterGlobalData* AJupiterGlobalData::EnsureExists(UWorld* World)
{
	if (!World)
		return nullptr;

	if (AJupiterGlobalData* Existing = Get(World))
		return Existing;

	if (World->IsNetMode(NM_Client))
		return nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<AJupiterGlobalData>(StaticClass(), SpawnParams);
}
