#include "Components/Placement/PlacementHandlerComponent.h"
#include "Data/Placement/PlacementUnitData.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"


UPlacementHandlerComponent::UPlacementHandlerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UPlacementHandlerComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UPlacementHandlerComponent::Server_RequestPlacement_Validate(const UPlacementItemData* Item, const FVector& Location, const FRotator& Rotation, int32 Count, ESpawnFormation Formation, FIntPoint CustomDimensions)
{
    if (!Item)
        return false;

    if (Count > 200 || Count < -1)
        return false;

    return true;
}

void UPlacementHandlerComponent::Server_RequestPlacement_Implementation(const UPlacementItemData* Item, const FVector& Location, const FRotator& Rotation, int32 Count, ESpawnFormation Formation, FIntPoint CustomDimensions)
{
    if (!Item)
    {
        UE_LOG(LogTemp, Warning, TEXT("Server_RequestPlacement: Item is NULL"));
        return;
    }

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
    if (!Item->ActorToSpawn)
        return;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    Params.Owner = GetOwner();

    GetWorld()->SpawnActor<AActor>(Item->ActorToSpawn, Location, Rotation, Params);
}

void UPlacementHandlerComponent::SpawnUnitGroup(const UPlacementUnitData* UnitData, const FVector& Location, const FRotator& Rotation, int32 Count, ESpawnFormation Formation, FIntPoint CustomDimensions)
{
    if (!UnitData || !UnitData->ActorToSpawn)
        return;

    TArray<FVector> Offsets;
    GenerateFormationOffsets(Offsets, Count, UnitData->FormationSpacing, Rotation, Formation, CustomDimensions);

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    for (const FVector& Offset : Offsets)
    {
        FVector SpawnLoc = Location + Offset;
        GetWorld()->SpawnActor<AActor>(UnitData->ActorToSpawn, SpawnLoc, Rotation, Params);
    }
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
        int32 RowSize = FMath::CeilToInt(FMath::Sqrt((float)Count));
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
