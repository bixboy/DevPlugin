#include "Components/Placement/PresetManagerComponent.h"
#include "Data/Placement/PresetSaveGame.h"
#include "Core/JupiterGlobalData.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"


UPresetManagerComponent::UPresetManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPresetManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPresetManagerComponent, SavedPresets);
}

bool UPresetManagerComponent::IsGlobalManager() const
{
	return GetOwner() && GetOwner()->IsA<AJupiterGlobalData>();
}

void UPresetManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (IsGlobalManager())
	{
		if (bResetSavesOnBeginPlay)
		{
			UGameplayStatics::DeleteGameInSlot(SaveSlotName, 0);
			SavedPresets.Empty();
			SaveToDisk();
		}
		else
		{
			LoadFromDisk();
		}
	}
	else
	{
		BindToGlobalData();
	}
}

void UPresetManagerComponent::BindToGlobalData()
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	if (AJupiterGlobalData* GlobalData = AJupiterGlobalData::Get(this))
	{
		if (UPresetManagerComponent* GlobalManager = GlobalData->GetPresetManager())
		{
			GlobalManager->OnPresetsChanged.AddUniqueDynamic(this, &UPresetManagerComponent::OnGlobalPresetsChanged);
			return;
		}
	}

	if (!World->IsNetMode(NM_Client))
	{
		AJupiterGlobalData::EnsureExists(World);
	}

	FTimerHandle TimerHandle;
	World->GetTimerManager().SetTimer(TimerHandle, this, &UPresetManagerComponent::BindToGlobalData, 0.1f, false);
}

void UPresetManagerComponent::OnGlobalPresetsChanged()
{
	OnPresetsChanged.Broadcast();
}

void UPresetManagerComponent::OnRep_SavedPresets()
{
	OnPresetsChanged.Broadcast();
}

const TArray<FPlacementPreset>& UPresetManagerComponent::GetAllPresets() const
{
	if (IsGlobalManager())
		return SavedPresets;

	if (AJupiterGlobalData* GlobalData = AJupiterGlobalData::Get(this))
	{
		if (UPresetManagerComponent* GlobalManager = GlobalData->GetPresetManager())
		{
			return GlobalManager->GetAllPresets();
		}
	}

	return SavedPresets;
}

const FPlacementPreset* UPresetManagerComponent::FindPreset(FGuid PresetID) const
{
	if (IsGlobalManager())
	{
		return SavedPresets.FindByPredicate([&](const FPlacementPreset& P) { return P.PresetID == PresetID; });
	}

	if (AJupiterGlobalData* GlobalData = AJupiterGlobalData::Get(this))
	{
		if (UPresetManagerComponent* GlobalManager = GlobalData->GetPresetManager())
		{
			return GlobalManager->FindPreset(PresetID);
		}
	}

	return nullptr;
}

bool UPresetManagerComponent::IsPresetNameTaken(FName PresetName) const
{
	const TArray<FPlacementPreset>& AllPresets = GetAllPresets();
	for (const FPlacementPreset& Preset : AllPresets)
	{
		if (Preset.PresetName == PresetName)
		{
			return true;
		}
	}
	return false;
}

// ------------------------------------------------------------------
// Public API
// ------------------------------------------------------------------

FGuid UPresetManagerComponent::CreatePresetFromActors(const TArray<AActor*>& Actors, FName PresetName)
{
	if (Actors.Num() == 0 || IsPresetNameTaken(PresetName))
		return FGuid();

	FVector Centroid = FVector::ZeroVector;
	int32 ValidCount = 0;

	for (AActor* Actor : Actors)
	{
		if (IsValid(Actor))
		{
			Centroid += Actor->GetActorLocation();
			++ValidCount;
		}
	}

	if (ValidCount == 0)
		return FGuid();

	Centroid /= static_cast<float>(ValidCount);

	FPlacementPreset NewPreset;
	NewPreset.PresetName = PresetName;
	NewPreset.PresetID = FGuid::NewGuid();
	NewPreset.Entries.Reserve(ValidCount);

	for (AActor* Actor : Actors)
	{
		if (!IsValid(Actor))
			continue;

		FPresetActorEntry Entry;
		Entry.ActorClass = Actor->GetClass();

		const FVector RelativeLoc = Actor->GetActorLocation() - Centroid;
		const FRotator Rotation = Actor->GetActorRotation();
		const FVector Scale = Actor->GetActorScale3D();

		Entry.RelativeTransform = FTransform(Rotation, RelativeLoc, Scale);
		NewPreset.Entries.Add(MoveTemp(Entry));
	}

	if (IsGlobalManager())
	{
		SavedPresets.Add(MoveTemp(NewPreset));
		SaveToDisk();
		OnPresetsChanged.Broadcast();
	}
	else
	{
		Server_ReplicatePreset(NewPreset);
	}

	return NewPreset.PresetID;
}

void UPresetManagerComponent::DeletePreset(FGuid PresetID)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		Server_DeletePreset(PresetID);
		return;
	}

	if (IsGlobalManager())
	{
		const int32 Removed = SavedPresets.RemoveAll([&](const FPlacementPreset& P) { return P.PresetID == PresetID; });

		if (Removed > 0)
		{
			SaveToDisk();
			OnPresetsChanged.Broadcast();
		}
	}
	else if (AJupiterGlobalData* GlobalData = AJupiterGlobalData::Get(this))
	{
		if (UPresetManagerComponent* GlobalManager = GlobalData->GetPresetManager())
			GlobalManager->DeletePreset(PresetID);
	}
}

void UPresetManagerComponent::Server_DeletePreset_Implementation(FGuid PresetID)
{
	DeletePreset(PresetID);
}

void UPresetManagerComponent::RenamePreset(FGuid PresetID, FName NewName)
{
	if (IsPresetNameTaken(NewName))
	{
		UE_LOG(LogTemp, Warning, TEXT("RenamePreset: Preset name '%s' is already taken."), *NewName.ToString());
		return;
	}

	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		Server_RenamePreset(PresetID, NewName);
		return;
	}

	if (IsGlobalManager())
	{
		for (FPlacementPreset& Preset : SavedPresets)
		{
			if (Preset.PresetID == PresetID)
			{
				Preset.PresetName = NewName;
				SaveToDisk();
				OnPresetsChanged.Broadcast();
				return;
			}
		}
	}
	else if (AJupiterGlobalData* GlobalData = AJupiterGlobalData::Get(this))
	{
		if (UPresetManagerComponent* GlobalManager = GlobalData->GetPresetManager())
			GlobalManager->RenamePreset(PresetID, NewName);
	}
}

void UPresetManagerComponent::Server_RenamePreset_Implementation(FGuid PresetID, FName NewName)
{
	RenamePreset(PresetID, NewName);
}

void UPresetManagerComponent::Server_SpawnPreset_Implementation(FGuid PresetID, FVector Location, FRotator Rotation)
{
	if (!IsGlobalManager())
	{
		if (AJupiterGlobalData* GlobalData = AJupiterGlobalData::EnsureExists(GetWorld()))
		{
			if (UPresetManagerComponent* GlobalManager = GlobalData->GetPresetManager())
				GlobalManager->Server_SpawnPreset_Implementation(PresetID, Location, Rotation);
		}
		return;
	}

	const FPlacementPreset* Preset = FindPreset(PresetID);
	if (!Preset)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_SpawnPreset: Preset not found [%s]"), *PresetID.ToString());
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC && GetOwner()->IsA<APawn>())
	{
		PC = Cast<APlayerController>(Cast<APawn>(GetOwner())->GetController());
	}

	if (!ValidateSpawnRequest(PC, PresetID, Location))
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;

	const FTransform RootTransform(Rotation, Location);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = GetOwner();

	for (const FPresetActorEntry& Entry : Preset->Entries)
	{
		if (!Entry.ActorClass)
			continue;

		const FVector WorldLoc = RootTransform.TransformPosition(Entry.RelativeTransform.GetLocation());
		const FRotator WorldRot = (FQuat(Rotation) * Entry.RelativeTransform.GetRotation()).Rotator();

		AActor* Spawned = World->SpawnActor<AActor>(Entry.ActorClass, WorldLoc, WorldRot, Params);
		if (Spawned)
		{
			Spawned->SetActorScale3D(Entry.RelativeTransform.GetScale3D());
		}
	}
}

// ------------------------------------------------------------------
// Persistence
// ------------------------------------------------------------------

void UPresetManagerComponent::SaveToDisk()
{
	UPresetSaveGame* SaveGameObj = NewObject<UPresetSaveGame>();
	SaveGameObj->Presets = SavedPresets;

	FAsyncSaveGameToSlotDelegate SavedDelegate;
	SavedDelegate.BindUObject(this, &UPresetManagerComponent::OnAsyncSaveFinished);

	UGameplayStatics::AsyncSaveGameToSlot(SaveGameObj, SaveSlotName, 0, SavedDelegate);
}

void UPresetManagerComponent::LoadFromDisk()
{
	if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
		return;

	if (UPresetSaveGame* Loaded = Cast<UPresetSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0)))
	{
		SavedPresets = MoveTemp(Loaded->Presets);
		OnPresetsChanged.Broadcast();
	}
}

void UPresetManagerComponent::Server_ReplicatePreset_Implementation(const FPlacementPreset& NewPreset)
{
	if (!IsGlobalManager())
	{
		if (AJupiterGlobalData* GlobalData = AJupiterGlobalData::EnsureExists(GetWorld()))
		{
			if (UPresetManagerComponent* GlobalManager = GlobalData->GetPresetManager())
				GlobalManager->Server_ReplicatePreset_Implementation(NewPreset);
		}
		return;
	}

	if (FindPreset(NewPreset.PresetID))
		return;

	SavedPresets.Add(NewPreset);
	SaveToDisk();
	OnPresetsChanged.Broadcast();
}

bool UPresetManagerComponent::ValidateSpawnRequest(APlayerController* PC, FGuid PresetID, FVector Location)
{
	if (PC)
	{
		const double CurrentTime = GetWorld()->GetTimeSeconds();
		if (const double* LastTime = SpawnCooldowns.Find(PC))
		{
			if (CurrentTime - *LastTime < MinSpawnCooldown)
				return false;
		}
		SpawnCooldowns.Add(PC, CurrentTime);
	}

	return true;
}

void UPresetManagerComponent::OnAsyncSaveFinished(const FString& SlotName, const int32 UserIndex, bool bSuccess)
{
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("AsyncSaveGameToSlot failed for slot: %s"), *SlotName);
	}
}
