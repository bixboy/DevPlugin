#include "Components/UnitSpawnComponent.h"

#include "Components/UnitSelectionComponent.h"
#include "Net/UnrealNetwork.h"
#include "Units/SoldierRts.h"

UUnitSpawnComponent::UUnitSpawnComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UUnitSpawnComponent::BeginPlay()
{
    Super::BeginPlay();

    SelectionComponent = GetOwner() ? GetOwner()->FindComponentByClass<UUnitSelectionComponent>() : nullptr;

    if (UnitsPerSpawn <= 0)
        UnitsPerSpawn = 1;

    if (GetOwner() && GetOwner()->HasAuthority() && !UnitToSpawn && DefaultUnitClass)
    {
        UnitToSpawn = DefaultUnitClass;
        OnRep_UnitToSpawn();
    }
}

void UUnitSpawnComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(UUnitSpawnComponent, UnitToSpawn, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UUnitSpawnComponent, UnitsPerSpawn, COND_OwnerOnly);
}

void UUnitSpawnComponent::SetUnitToSpawn(TSubclassOf<ASoldierRts> NewUnitClass)
{
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        UnitToSpawn = NewUnitClass;
        OnRep_UnitToSpawn();
    }
    else
    {
        ServerSetUnitClass(NewUnitClass);
    }
}

void UUnitSpawnComponent::SpawnUnits()
{
    if (!SelectionComponent)
        return;

    const FHitResult HitResult = SelectionComponent->GetMousePositionOnTerrain();

    if (bRequireGroundHit && !HitResult.bBlockingHit)
        return;

    const FVector SpawnLocation = HitResult.bBlockingHit ? HitResult.Location : HitResult.TraceEnd;

    if (GetOwner() && GetOwner()->HasAuthority())
        ServerSpawnUnits(SpawnLocation);
    else
        ServerSpawnUnits(SpawnLocation);
}

void UUnitSpawnComponent::SetUnitsPerSpawn(int32 NewSpawnCount)
{
    NewSpawnCount = FMath::Max(1, NewSpawnCount);

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        if (UnitsPerSpawn != NewSpawnCount)
        {
            UnitsPerSpawn = NewSpawnCount;
            OnRep_UnitsPerSpawn();
        }
    }
    else
    {
        ServerSetUnitsPerSpawn(NewSpawnCount);
    }
}

void UUnitSpawnComponent::ServerSetUnitClass_Implementation(TSubclassOf<ASoldierRts> NewUnitClass)
{
    UnitToSpawn = NewUnitClass;
    OnRep_UnitToSpawn();
}

void UUnitSpawnComponent::ServerSetUnitsPerSpawn_Implementation(int32 NewSpawnCount)
{
    NewSpawnCount = FMath::Max(1, NewSpawnCount);

    if (UnitsPerSpawn != NewSpawnCount)
    {
        UnitsPerSpawn = NewSpawnCount;
        OnRep_UnitsPerSpawn();
    }
}

void UUnitSpawnComponent::ServerSpawnUnits_Implementation(const FVector& SpawnLocation)
{
    if (!UnitToSpawn)
        return;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    SpawnParams.Owner = GetOwner();

    if (UWorld* World = GetWorld())
    {
        const int32 SpawnCount = FMath::Max(UnitsPerSpawn, 1);
        const float Spacing = FMath::Max(FormationSpacing, 0.f);

        const int32 Columns = FMath::Max(1, FMath::CeilToInt(FMath::Sqrt(static_cast<float>(SpawnCount))));

        const int32 Rows = FMath::Max(1, FMath::CeilToInt(static_cast<float>(SpawnCount) / static_cast<float>(Columns)));
        const float HalfWidth = (Columns - 1) * 0.5f * Spacing;
        const float HalfHeight = (Rows - 1) * 0.5f * Spacing;

        bool bSpawnedAnyUnit = false;

        for (int32 Index = 0; Index < SpawnCount; ++Index)
        {
            const int32 Row = Index / Columns;
            const int32 Column = Index % Columns;

            const float OffsetX = (Column * Spacing) - HalfWidth;
            const float OffsetY = (Row * Spacing) - HalfHeight;

            const FVector FormationOffset(OffsetX, OffsetY, 0.f);
            const FVector FormationLocation = SpawnLocation + FormationOffset;

            ASoldierRts* SpawnedUnit = World->SpawnActor<ASoldierRts>(UnitToSpawn, FormationLocation, FRotator::ZeroRotator, SpawnParams);

            bSpawnedAnyUnit |= (SpawnedUnit != nullptr);
        }

        if (!bNotifyOnServerOnly && bSpawnedAnyUnit)
        {
            OnUnitClassChanged.Broadcast(UnitToSpawn);
        }
    }
}

void UUnitSpawnComponent::OnRep_UnitToSpawn()
{
    OnUnitClassChanged.Broadcast(UnitToSpawn);
}

void UUnitSpawnComponent::OnRep_UnitsPerSpawn()
{
    OnSpawnCountChanged.Broadcast(UnitsPerSpawn);
}

