// Copyright 2026

#include "Actors/JupiterFOBCore.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "Inventory/Legacy/AC_StorageComponent.h"
#include "DataClasses/Entities/Items/PDA_ItemClass.h"

AJupiterFOBCore::AJupiterFOBCore()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;

	BuildRadiusComponent = CreateDefaultSubobject<USphereComponent>(TEXT("BuildRadiusComponent"));
	RootComponent = BuildRadiusComponent;
	BuildRadiusComponent->SetSphereRadius(5000.f); // 50 meters default radius
	BuildRadiusComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);

	StorageComponent = CreateDefaultSubobject<UAC_StorageComponent>(TEXT("StorageComponent"));
	StorageComponent->SetIsReplicated(true);
}

void AJupiterFOBCore::BeginPlay()
{
	Super::BeginPlay();
}

bool AJupiterFOBCore::CanAffordResource_Implementation(const TSoftObjectPtr<UPDA_ItemClass>& ResourceItem, int32 Amount) const
{
	UPDA_ItemClass* RequestedItem = ResourceItem.IsNull() ? nullptr : const_cast<TSoftObjectPtr<UPDA_ItemClass>&>(ResourceItem).LoadSynchronous();
	if (!StorageComponent || !RequestedItem)
	{
		UE_LOG(LogTemp, Error, TEXT("[FOBCore] CanAfford EARLY OUT: StorageComp=%s, RequestedItem=%s"),
			StorageComponent ? TEXT("valid") : TEXT("null"),
			RequestedItem ? TEXT("valid") : TEXT("null"));
		return false;
	}
	int32 TotalFound = 0;
	const FStorageInventoryArray& Inventory = StorageComponent->GetReplicatedInventory();
	
	UE_LOG(LogTemp, Display, TEXT("[FOBCore] CanAfford checking '%s' x%d. Storage has %d entries."),
		RequestedItem ? *RequestedItem->GetName() : TEXT("NULL_PTR"), Amount, Inventory.Items.Num());
	
	for (const FStorageUnitItemEntry& Entry : Inventory.Items)
	{
		if (Entry.m_IsUsed)
		{
			UE_LOG(LogTemp, Display, TEXT("[FOBCore]   Slot[%d]: ItemData=%s, Count=%d"),
				Entry.m_SlotID,
				Entry.M_ItemSlot.M_ItemData ? *Entry.M_ItemSlot.M_ItemData->GetName() : TEXT("null"),
				Entry.M_ItemSlot.M_ItemCount);
		}
		
		if (Entry.m_IsUsed && Entry.M_ItemSlot.M_ItemData == RequestedItem)
		{
			TotalFound += Entry.M_ItemSlot.M_ItemCount;
			if (TotalFound >= Amount)
			{
				return true;
			}
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[FOBCore] CanAfford: Found %d / %d needed."), TotalFound, Amount);
	return false;
}

bool AJupiterFOBCore::ConsumeResource_Implementation(const TSoftObjectPtr<UPDA_ItemClass>& ResourceItem, int32 Amount)
{
	if (!CanAffordResource_Implementation(ResourceItem, Amount))
		return false;

	UPDA_ItemClass* ResolvedItem = const_cast<TSoftObjectPtr<UPDA_ItemClass>&>(ResourceItem).LoadSynchronous();
	if (!ResolvedItem)
		return false;

	int32 AmountLeftToConsume = Amount;

	// Loop over items and consume
	const FStorageInventoryArray& Inventory = StorageComponent->GetReplicatedInventory();
	
	for (const FStorageUnitItemEntry& Entry : Inventory.Items)
	{
		if (Entry.m_IsUsed && Entry.M_ItemSlot.M_ItemData == ResolvedItem)
		{
			int32 AmountToTake = FMath::Min(AmountLeftToConsume, Entry.M_ItemSlot.M_ItemCount);
			
			// We can use the server function to remove the item
			StorageComponent->Server_Reliable_RemoveAmountItem(Entry.m_SlotID, false, AmountToTake);
			
			AmountLeftToConsume -= AmountToTake;
			
			if (AmountLeftToConsume <= 0)
			{
				break;
			}
		}
	}

	return true;
}

void AJupiterFOBCore::AddResource_Implementation(const TSoftObjectPtr<UPDA_ItemClass>& ResourceItem, int32 Amount)
{
	UPDA_ItemClass* ResolvedItem = ResourceItem.IsNull() ? nullptr : const_cast<TSoftObjectPtr<UPDA_ItemClass>&>(ResourceItem).LoadSynchronous();
	
	UE_LOG(LogTemp, Display, TEXT("[FOBCore] AddResource_Implementation called! Amount=%d, Item=%s"),
		Amount, ResolvedItem ? *ResolvedItem->GetName() : TEXT("NULL"));
	
	if (Amount <= 0 || !StorageComponent || !ResolvedItem)
	{
		UE_LOG(LogTemp, Error, TEXT("[FOBCore] AddResource EARLY OUT: Amount=%d, StorageComp=%s, Item=%s"),
			Amount, StorageComponent ? TEXT("valid") : TEXT("null"),
			ResolvedItem ? TEXT("valid") : TEXT("null"));
		return;
	}
	
	// Add item to storage
	FItemSlot NewItem(ResolvedItem, Amount);
	StorageComponent->Server_Reliable_AddItemAtNextIndex(NewItem);
	
	UE_LOG(LogTemp, Display, TEXT("[FOBCore] Added %s x%d to storage."), *ResolvedItem->GetName(), Amount);
}

float AJupiterFOBCore::GetBuildRadius() const
{
	return BuildRadiusComponent ? BuildRadiusComponent->GetScaledSphereRadius() : 0.f;
}

void AJupiterFOBCore::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AJupiterFOBCore, ReplicatedMesh);
}

void AJupiterFOBCore::OnRep_BuildingMesh()
{
	if (MeshComponent && ReplicatedMesh)
	{
		MeshComponent->SetStaticMesh(ReplicatedMesh);
	}
}

void AJupiterFOBCore::SetFOBMesh(UStaticMesh* InMesh)
{
	if (HasAuthority())
	{
		ReplicatedMesh = InMesh;
		OnRep_BuildingMesh();
	}
}
