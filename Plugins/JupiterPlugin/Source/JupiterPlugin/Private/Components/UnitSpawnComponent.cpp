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

void UUnitSpawnComponent::ServerSetUnitClass_Implementation(TSubclassOf<ASoldierRts> NewUnitClass)
{
    UnitToSpawn = NewUnitClass;
    OnRep_UnitToSpawn();
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
        ASoldierRts* SpawnedUnit = World->SpawnActor<ASoldierRts>(UnitToSpawn, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
        
        if (!bNotifyOnServerOnly && SpawnedUnit)
            OnUnitClassChanged.Broadcast(UnitToSpawn);
    }
}

void UUnitSpawnComponent::OnRep_UnitToSpawn()
{
    OnUnitClassChanged.Broadcast(UnitToSpawn);
}

