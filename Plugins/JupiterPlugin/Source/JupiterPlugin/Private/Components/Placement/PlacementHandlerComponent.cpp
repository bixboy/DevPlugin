#include "Components/Placement/PlacementHandlerComponent.h"
#include "Data/Placement/PlacementUnitData.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"


UPlacementHandlerComponent::UPlacementHandlerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
    SetIsReplicatedByDefault(true);
}

void UPlacementHandlerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UPlacementHandlerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (SpawnQueue.IsEmpty())
		return;

	int32 SpawnsRemaining = MaxSpawnsPerFrame;

	while (SpawnsRemaining > 0 && !SpawnQueue.IsEmpty())
	{
		FAsyncSpawnBatch& Batch = SpawnQueue[0];
		
		if (!Batch.ItemData.IsValid())
		{
			SpawnQueue.RemoveAt(0);
			continue;
		}

		while (Batch.CurrentIndex < Batch.Transforms.Num() && SpawnsRemaining > 0)
		{
			const FTransform& Trans = Batch.Transforms[Batch.CurrentIndex];
			SpawnSingleActor(Batch.ItemData.Get(), Trans.GetLocation(), Trans.Rotator());
			
			Batch.CurrentIndex++;
			SpawnsRemaining--;
		}

		if (Batch.CurrentIndex >= Batch.Transforms.Num())
		{
			SpawnQueue.RemoveAt(0);
		}
	}
}

void UPlacementHandlerComponent::Server_RequestPlacement_Implementation(const UPlacementItemData* Item, const FVector& Location, const FRotator& Rotation, int32 Count, ESpawnFormation Formation, FIntPoint CustomDimensions)
{
    if (!Item)
    {
        UE_LOG(LogTemp, Warning, TEXT("Server_RequestPlacement: Item is NULL"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Server_RequestPlacement: Received request for '%s' at %s"), *Item->GetName(), *Location.ToString());

    if (const UPlacementUnitData* UnitData = Cast<UPlacementUnitData>(Item))
    {
        int32 FinalCount = (Count > 0) ? Count : UnitData->DefaultUnitCount;
        SpawnUnitGroup(UnitData, Location, Rotation, FinalCount, Formation, CustomDimensions);
    }
    else
    {
        SpawnSingleActor(Item, Location, Rotation);
    }
}

void UPlacementHandlerComponent::SpawnSingleActor(const UPlacementItemData* Item, const FVector& Location, const FRotator& Rotation)
{
    if (!Item || !Item->ActorToSpawn)
    {
        UE_LOG(LogTemp, Error, TEXT("SpawnSingleActor: Item or ActorToSpawn is NULL"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("SpawnSingleActor: Spawning %s at %s"), *Item->ActorToSpawn->GetName(), *Location.ToString());

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    Params.Owner = GetOwner();

    AActor* Spawned = GetWorld()->SpawnActor<AActor>(Item->ActorToSpawn, Location, Rotation, Params);
    if (Spawned)
    {
        // UE_LOG(LogTemp, Log, TEXT("SpawnSingleActor: SUCCESS -> %s"), *Spawned->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SpawnSingleActor: FAILED to spawn actor"));
    }
}

void UPlacementHandlerComponent::SpawnUnitGroup(const UPlacementUnitData* UnitData, const FVector& Location, const FRotator& Rotation, int32 Count, ESpawnFormation Formation, FIntPoint CustomDimensions)
{
	if (!UnitData || !UnitData->ActorToSpawn)
		return;

	TArray<FVector> Offsets;
	GenerateFormationOffsets(Offsets, Count, UnitData->FormationSpacing, Rotation, Formation, CustomDimensions);

    // Queue the spawn
    FAsyncSpawnBatch Batch;
    Batch.ItemData = UnitData;
    
    for (const FVector& Offset : Offsets)
    {
        FVector SpawnLoc = Location + Offset;
        Batch.Transforms.Add(FTransform(Rotation, SpawnLoc));
    }
    
    SpawnQueue.Add(Batch);
    
    UE_LOG(LogTemp, Log, TEXT("UIPlacementHandler: Queued batch of %d units"), Offsets.Num());
}

void UPlacementHandlerComponent::GenerateFormationOffsets(TArray<FVector>& OutOffsets, int32 Count, float Spacing, const FRotator& Facing, ESpawnFormation Formation, FIntPoint CustomDimensions) const
{
    OutOffsets.Reset();
    if (Count <= 0) 
        return;

    FVector RightDir = UKismetMathLibrary::GetRightVector(Facing);
    FVector ForwardDir = UKismetMathLibrary::GetForwardVector(Facing);

    if (Formation == ESpawnFormation::Square)
    {
        int32 RowSize = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(Count)));
        float Width = (RowSize - 1) * Spacing;
        FVector StartPos = -(RightDir * Width * 0.5f) - (ForwardDir * Width * 0.5f);

        for (int32 i = 0; i < Count; i++)
        {
            int32 Row = i / RowSize;
            int32 Col = i % RowSize;
            FVector Offset = StartPos + (RightDir * Col * Spacing) + (ForwardDir * Row * Spacing);
            OutOffsets.Add(Offset);
        }
    }
	
    else if (Formation == ESpawnFormation::Line)
    {
        float Width = (Count - 1) * Spacing;
        FVector StartPos = -(RightDir * Width * 0.5f);

        for (int32 i = 0; i < Count; i++)
        {
             OutOffsets.Add(StartPos + (RightDir * i * Spacing));
        }
    }
    else if (Formation == ESpawnFormation::Column)
    {
        float Depth = (Count - 1) * Spacing;
        FVector StartPos = -(ForwardDir * Depth * 0.5f);
        
        for (int32 i = 0; i < Count; i++)
        {
             OutOffsets.Add(StartPos + (ForwardDir * i * Spacing));
        }
    }
    else if (Formation == ESpawnFormation::Wedge)
    {
        int32 CurrentIdx = 0;
        int32 Row = 0;
        while (CurrentIdx < Count)
        {
            int32 UnitsInRow = Row + 1;
            float RowWidth = (UnitsInRow - 1) * Spacing;
            FVector RowStart = -(RightDir * RowWidth * 0.5f) - (ForwardDir * Row * Spacing);

            for(int32 k=0; k < UnitsInRow && CurrentIdx < Count; k++)
            {
                OutOffsets.Add(RowStart + (RightDir * k * Spacing));
                CurrentIdx++;
            }
            Row++;
        }
    }
    else if (Formation == ESpawnFormation::Custom)
    {
        int32 Columns = CustomDimensions.X > 0 ? CustomDimensions.X : 1;
        float Width = (Columns - 1) * Spacing;

        FVector StartPos = -(RightDir * Width * 0.5f) - (ForwardDir * ((Count/Columns) * Spacing * 0.5f));

        for (int32 i = 0; i < Count; i++)
        {
             int32 Row = i / Columns;
             int32 Col = i % Columns;
             OutOffsets.Add(StartPos + (RightDir * Col * Spacing) + (ForwardDir * Row * Spacing));
        }
    }
    else 
    {
        GenerateFormationOffsets(OutOffsets, Count, Spacing, Facing, ESpawnFormation::Square);
    }
}
