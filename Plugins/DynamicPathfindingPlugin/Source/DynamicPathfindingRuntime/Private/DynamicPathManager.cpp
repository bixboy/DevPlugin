#include "DynamicPathManager.h"
#include "GridNavigationComponent.h"
#include "UnitNavigationComponent.h"
#include "UnitAvoidanceComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"

ADynamicPathManager::ADynamicPathManager()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.016f;

    GridComponent = CreateDefaultSubobject<UGridNavigationComponent>(TEXT("GridNavigation"));

    SpatialHashCellSize = 300.f;
    bDrawAvoidanceDebug = false;
}

void ADynamicPathManager::BeginPlay()
{
    Super::BeginPlay();
}

void ADynamicPathManager::PostRegisterAllComponents()
{
    Super::PostRegisterAllComponents();

    if (GridComponent)
    {
        GridComponent->RegisterComponent();
    }
}

void ADynamicPathManager::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (GridComponent)
    {
        GridComponent->DrawDebug();
    }

    if (bDrawAvoidanceDebug)
    {
        DrawAvoidanceDebug();
    }
}

void ADynamicPathManager::SetDebugEnabled(bool bEnabled)
{
    if (GridComponent)
    {
        GridComponent->SetDebugEnabled(bEnabled);
    }

    bDrawAvoidanceDebug = bEnabled;
}

void ADynamicPathManager::SetTargetLocation(const FVector& TargetLocation)
{
    if (GridComponent)
    {
        GridComponent->SetTargetLocation(TargetLocation);
    }
}

void ADynamicPathManager::ToggleDynamicPathDebug()
{
    const bool bNewEnabled = !bDrawAvoidanceDebug;
    SetDebugEnabled(bNewEnabled);
}

void ADynamicPathManager::RegisterUnit(UUnitNavigationComponent* UnitComponent)
{
    if (!UnitComponent)
    {
        return;
    }

    RegisteredUnits.Add(UnitComponent);
}

void ADynamicPathManager::UnregisterUnit(UUnitNavigationComponent* UnitComponent)
{
    if (!UnitComponent)
    {
        return;
    }

    RegisteredUnits.Remove(UnitComponent);
}

void ADynamicPathManager::RegisterAvoidanceComponent(UUnitAvoidanceComponent* Component)
{
    if (!Component)
    {
        return;
    }

    Component->SetManager(this);
    const FVector Location = Component->GetOwner() ? Component->GetOwner()->GetActorLocation() : GetActorLocation();
    const FIntVector CellKey = GetCellKey(Location);
    Component->UpdateSpatialCellKey(CellKey);

    FSpatialCell& Cell = SpatialHash.FindOrAdd(CellKey);
    if (!Cell.Components.Contains(Component))
    {
        Cell.Components.Add(Component);
    }
}

void ADynamicPathManager::UnregisterAvoidanceComponent(UUnitAvoidanceComponent* Component)
{
    if (!Component)
    {
        return;
    }

    const FIntVector CellKey = Component->GetSpatialCellKey();
    RemoveComponentFromCell(Component, CellKey);
}

void ADynamicPathManager::UpdateSpatialEntry(UUnitAvoidanceComponent* Component, const FVector& Location)
{
    if (!Component)
    {
        return;
    }

    const FIntVector NewKey = GetCellKey(Location);
    const FIntVector OldKey = Component->GetSpatialCellKey();

    if (NewKey != OldKey)
    {
        RemoveComponentFromCell(Component, OldKey);
        FSpatialCell& NewCell = SpatialHash.FindOrAdd(NewKey);
        NewCell.Components.Add(Component);
        Component->UpdateSpatialCellKey(NewKey);
    }
    else
    {
        FSpatialCell& Cell = SpatialHash.FindOrAdd(NewKey);
        if (!Cell.Components.Contains(Component))
        {
            Cell.Components.Add(Component);
        }
    }
}

FVector ADynamicPathManager::ComputeAvoidanceForComponent(const UUnitAvoidanceComponent* Component, const FVector& DesiredVelocity) const
{
    if (!Component)
    {
        return FVector::ZeroVector;
    }

    const FVector Location = Component->GetOwner() ? Component->GetOwner()->GetActorLocation() : FVector::ZeroVector;
    const float NeighborRadiusSq = FMath::Square(Component->NeighborRadius);

    FVector SeparationForce = FVector::ZeroVector;
    FVector RVOForce = FVector::ZeroVector;

    const FIntVector CellKey = GetCellKey(Location);

    for (int32 X = -1; X <= 1; ++X)
    {
        for (int32 Y = -1; Y <= 1; ++Y)
        {
            for (int32 Z = -1; Z <= 1; ++Z)
            {
                const FIntVector NeighborKey = CellKey + FIntVector(X, Y, Z);
                const FSpatialCell* Cell = SpatialHash.Find(NeighborKey);
                if (!Cell)
                {
                    continue;
                }

                for (const TWeakObjectPtr<UUnitAvoidanceComponent>& WeakOther : Cell->Components)
                {
                    UUnitAvoidanceComponent* Other = WeakOther.Get();
                    if (!Other || Other == Component)
                    {
                        continue;
                    }

                    AActor* OtherOwner = Other->GetOwner();
                    if (!OtherOwner)
                    {
                        continue;
                    }

                    const FVector OtherLocation = OtherOwner->GetActorLocation();
                    const FVector ToOther = OtherLocation - Location;
                    const float DistanceSq = ToOther.SizeSquared();

                    if (DistanceSq < KINDA_SMALL_NUMBER || DistanceSq > NeighborRadiusSq)
                    {
                        continue;
                    }

                    const float Strength = (NeighborRadiusSq - DistanceSq) / NeighborRadiusSq;
                    SeparationForce -= ToOther.GetSafeNormal() * Strength * Component->SeparationWeight;

                    // RVO term: adjust relative velocity to avoid head-on collisions
                    const FVector RelativeVelocity = Other->GetCurrentVelocity() - DesiredVelocity;
                    const float ClosingSpeed = FVector::DotProduct(RelativeVelocity, ToOther.GetSafeNormal());
                    if (ClosingSpeed < 0.f)
                    {
                        RVOForce -= RelativeVelocity * Component->RVOWeight;
                    }
                }
            }
        }
    }

    FVector Avoidance = SeparationForce + RVOForce;
    Avoidance = Avoidance.GetClampedToMaxSize(Component->MaxAvoidanceForce);
    return Avoidance;
}

FVector ADynamicPathManager::GetFlowDirectionAtLocation(const FVector& Location) const
{
    if (GridComponent)
    {
        return GridComponent->GetFlowDirectionAtLocation(Location);
    }

    return FVector::ZeroVector;
}

void ADynamicPathManager::MarkObstacle(const FVector& Location, const FVector& Extents, bool bBlocked)
{
    if (!GridComponent)
    {
        return;
    }

    const FVector Min = Location - Extents;
    const FVector Max = Location + Extents;

    const float InvVoxelSize = 1.f / FMath::Max(GridComponent->VoxelSize, 1.f);

    FIntVector MinCoords;
    MinCoords.X = FMath::FloorToInt((Min.X - GridComponent->GridOrigin.X) * InvVoxelSize);
    MinCoords.Y = FMath::FloorToInt((Min.Y - GridComponent->GridOrigin.Y) * InvVoxelSize);
    MinCoords.Z = FMath::FloorToInt((Min.Z - GridComponent->GridOrigin.Z) * InvVoxelSize);

    FIntVector MaxCoords;
    MaxCoords.X = FMath::CeilToInt((Max.X - GridComponent->GridOrigin.X) * InvVoxelSize);
    MaxCoords.Y = FMath::CeilToInt((Max.Y - GridComponent->GridOrigin.Y) * InvVoxelSize);
    MaxCoords.Z = FMath::CeilToInt((Max.Z - GridComponent->GridOrigin.Z) * InvVoxelSize);

    for (int32 X = MinCoords.X; X <= MaxCoords.X; ++X)
    {
        for (int32 Y = MinCoords.Y; Y <= MaxCoords.Y; ++Y)
        {
            for (int32 Z = MinCoords.Z; Z <= MaxCoords.Z; ++Z)
            {
                GridComponent->SetVoxelBlocked(FIntVector(X, Y, Z), bBlocked);
            }
        }
    }
}

FIntVector ADynamicPathManager::GetCellKey(const FVector& Location) const
{
    const float InvSize = 1.f / FMath::Max(SpatialHashCellSize, 1.f);
    return FIntVector(
        FMath::FloorToInt(Location.X * InvSize),
        FMath::FloorToInt(Location.Y * InvSize),
        FMath::FloorToInt(Location.Z * InvSize));
}

void ADynamicPathManager::RemoveComponentFromCell(UUnitAvoidanceComponent* Component, const FIntVector& CellKey)
{
    if (FSpatialCell* Cell = SpatialHash.Find(CellKey))
    {
        Cell->Components.Remove(Component);
        if (Cell->Components.Num() == 0)
        {
            SpatialHash.Remove(CellKey);
        }
    }
}

void ADynamicPathManager::DrawAvoidanceDebug() const
{
    if (!GetWorld())
    {
        return;
    }

    for (const auto& Pair : SpatialHash)
    {
        const FIntVector Key = Pair.Key;
        const FVector CellCenter = FVector(Key) * SpatialHashCellSize + FVector(SpatialHashCellSize * 0.5f);
        DrawDebugBox(GetWorld(), CellCenter, FVector(SpatialHashCellSize * 0.5f), FColor::Yellow, false, -1.f, 0, 2.f);
    }
}
