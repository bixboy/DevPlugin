#include "Subsystems/UnitSpatialGridSubsystem.h"
#include "GameFramework/Actor.h"

void UUnitSpatialGridSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Grid.Reset();
    UnitCellCache.Reset();
}

void UUnitSpatialGridSubsystem::Deinitialize()
{
    Grid.Reset();
    UnitCellCache.Reset();
    Super::Deinitialize();
}

FIntPoint UUnitSpatialGridSubsystem::GetCell(const FVector& Location, float InCellSize)
{
    return FIntPoint(
        FMath::FloorToInt(Location.X / InCellSize),
        FMath::FloorToInt(Location.Y / InCellSize)
    );
}

void UUnitSpatialGridSubsystem::RegisterUnit(AActor* Unit)
{
    if (!Unit) 
        return;
    
    TWeakObjectPtr<AActor> WeakUnit = Unit;
    if (UnitCellCache.Contains(WeakUnit))
    {
        UnregisterUnit(Unit);
    }

    FIntPoint Cell = GetCell(Unit->GetActorLocation(), CellSize);
    
    Grid.FindOrAdd(Cell).Add(WeakUnit);
    UnitCellCache.Add(WeakUnit, Cell);
}

void UUnitSpatialGridSubsystem::UnregisterUnit(AActor* Unit)
{
    if (!Unit) 
        return;
    
    TWeakObjectPtr<AActor> WeakUnit = Unit;
    if (FIntPoint* CellPtr = UnitCellCache.Find(WeakUnit))
    {
        FIntPoint Cell = *CellPtr;
        if (TArray<TWeakObjectPtr<AActor>>* List = Grid.Find(Cell))
        {
            List->Remove(WeakUnit);
        }

        UnitCellCache.Remove(WeakUnit);
    }
}

void UUnitSpatialGridSubsystem::UpdateUnitPosition(AActor* Unit)
{
    if (!Unit) 
        return;

    TWeakObjectPtr<AActor> WeakUnit = Unit;
    FIntPoint NewCell = GetCell(Unit->GetActorLocation(), CellSize);

    if (FIntPoint* OldCellPtr = UnitCellCache.Find(WeakUnit))
    {
        if (*OldCellPtr == NewCell)
            return;
    }
    UnregisterUnit(Unit);
    RegisterUnit(Unit);
}

TArray<AActor*> UUnitSpatialGridSubsystem::GetUnitsInBounds(const FVector2D& Min, const FVector2D& Max)
{
    TArray<AActor*> Result;
    
    FIntPoint MinCell = GetCell(FVector(Min.X, Min.Y, 0), CellSize);
    FIntPoint MaxCell = GetCell(FVector(Max.X, Max.Y, 0), CellSize);

    for (int32 X = MinCell.X; X <= MaxCell.X; ++X)
    {
        for (int32 Y = MinCell.Y; Y <= MaxCell.Y; ++Y)
        {
            FIntPoint Cell(X, Y);
            if (const TArray<TWeakObjectPtr<AActor>>* List = Grid.Find(Cell))
            {
                 for (const TWeakObjectPtr<AActor>& WeakActor : *List)
                 {
                     if (AActor* Actor = WeakActor.Get())
                     {
                         const FVector Loc = Actor->GetActorLocation();
                         if (Loc.X >= Min.X && Loc.X <= Max.X && Loc.Y >= Min.Y && Loc.Y <= Max.Y)
                         {
                             Result.Add(Actor);
                         }
                     }
                 }
            }
        }
    }

    return Result;
}
