#include "Components/Unit/UnitSpawnComponent.h"

#include "Components/Unit/UnitSelectionComponent.h"
#include "Library/FormationMathLibrary.h"
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

    UnitsPerSpawn = FMath::Max(1, UnitsPerSpawn);
    CustomFormationDimensions.X = FMath::Max(1, CustomFormationDimensions.X);
    CustomFormationDimensions.Y = FMath::Max(1, CustomFormationDimensions.Y);

    if (GetOwner()->HasAuthority() && !UnitToSpawn && DefaultUnitClass)
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
    DOREPLIFETIME_CONDITION(UUnitSpawnComponent, SpawnFormation, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UUnitSpawnComponent, CustomFormationDimensions, COND_OwnerOnly);
}

// ------------------------------------------------------------
// SETTERS
// ------------------------------------------------------------

void UUnitSpawnComponent::SetUnitToSpawn(TSubclassOf<ASoldierRts> NewUnitClass)
{
    // 1. Prédiction Locale 
    if (UnitToSpawn != NewUnitClass)
    {
        UnitToSpawn = NewUnitClass;
        OnRep_UnitToSpawn();
    }

    // 2. Envoi au Serveur
    if (!GetOwner()->HasAuthority())
    {
        ServerSetUnitClass(NewUnitClass);
    }
}

void UUnitSpawnComponent::SetUnitsPerSpawn(int32 NewSpawnCount)
{
    NewSpawnCount = FMath::Max(1, NewSpawnCount);
    if (SpawnFormation == ESpawnFormation::Custom)
    {
        NewSpawnCount = FMath::Max(1, CustomFormationDimensions.X * CustomFormationDimensions.Y);
    }

    if (UnitsPerSpawn != NewSpawnCount)
    {
        UnitsPerSpawn = NewSpawnCount;
        OnRep_UnitsPerSpawn();
    }

    if (!GetOwner()->HasAuthority())
    {
        ServerSetUnitsPerSpawn(NewSpawnCount);
    }
}

void UUnitSpawnComponent::SetSpawnFormation(ESpawnFormation NewFormation)
{
    if (SpawnFormation != NewFormation)
    {
        SpawnFormation = NewFormation;
        OnRep_SpawnFormation();
    }

    UpdateSpawnCountFromCustomFormation();

    if (!GetOwner()->HasAuthority())
    {
        ServerSetSpawnFormation(NewFormation);
    }
}

void UUnitSpawnComponent::SetCustomFormationDimensions(FIntPoint NewDimensions)
{
    NewDimensions.X = FMath::Max(1, NewDimensions.X);
    NewDimensions.Y = FMath::Max(1, NewDimensions.Y);

    if (CustomFormationDimensions != NewDimensions)
    {
        CustomFormationDimensions = NewDimensions;
        OnRep_CustomFormationDimensions();
    }
    
    UpdateSpawnCountFromCustomFormation();

    if (!GetOwner()->HasAuthority())
    {
        ServerSetCustomFormationDimensions(NewDimensions);
    }
}

// ------------------------------------------------------------
// ACTIONS
// ------------------------------------------------------------

void UUnitSpawnComponent::SpawnUnits()
{
    if (!SelectionComponent)
    	return;

    const FHitResult Hit = SelectionComponent->GetMousePositionOnTerrain();
    if (bRequireGroundHit && !Hit.bBlockingHit)
    	return;

    const FVector SpawnLocation = Hit.bBlockingHit ? Hit.Location : Hit.TraceEnd;
    SpawnUnitsWithTransform(SpawnLocation, FRotator::ZeroRotator);
}

void UUnitSpawnComponent::SpawnUnitsWithTransform(const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
    if (!UnitToSpawn)
    	return;
    
    ServerSpawnUnits(SpawnLocation, SpawnRotation);
}


// ------------------------------------------------------------
// SERVER IMPLEMENTATIONS
// ------------------------------------------------------------

void UUnitSpawnComponent::ServerSpawnUnits_Implementation(const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
    if (!UnitToSpawn || !GetWorld())
    	return;

    const int32 Count = FMath::Max(UnitsPerSpawn, 1);
    const float Spacing = FMath::Max(FormationSpacing, 0.0f);

    TArray<FVector> Offsets;
    BuildSpawnFormationOffsets(Count, Spacing, Offsets, SpawnRotation);
    const FRotator UnitRotation(0.f, SpawnRotation.Yaw, 0.f);

    FActorSpawnParameters Params;
    Params.Owner = GetOwner();
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    bool bSpawned = false;

    for (const FVector& Offset : Offsets)
    {
        const FVector FinalPos = SpawnLocation + Offset;
        ASoldierRts* Spawned = GetWorld()->SpawnActor<ASoldierRts>(UnitToSpawn, FinalPos, UnitRotation, Params);
        if (Spawned)
        {
            bSpawned = true;
        }
    }

    if (bSpawned && !bNotifyOnServerOnly)
    {
        OnUnitClassChanged.Broadcast(UnitToSpawn);
    }
}

void UUnitSpawnComponent::ServerSetUnitClass_Implementation(TSubclassOf<ASoldierRts> NewUnit)
{
	UnitToSpawn = NewUnit; OnRep_UnitToSpawn();
}
void UUnitSpawnComponent::ServerSetUnitsPerSpawn_Implementation(int32 NewCount)
{
	UnitsPerSpawn = FMath::Clamp(NewCount, 1, 100); 
    OnRep_UnitsPerSpawn();
}
void UUnitSpawnComponent::ServerSetSpawnFormation_Implementation(ESpawnFormation NewFormation)
{
	SpawnFormation = NewFormation; OnRep_SpawnFormation(); UpdateSpawnCountFromCustomFormation();
}
void UUnitSpawnComponent::ServerSetCustomFormationDimensions_Implementation(FIntPoint NewDimensions)
{
    NewDimensions.X = FMath::Clamp(NewDimensions.X, 1, 20);
    NewDimensions.Y = FMath::Clamp(NewDimensions.Y, 1, 20);
    
	CustomFormationDimensions = NewDimensions; 
    OnRep_CustomFormationDimensions(); 
    UpdateSpawnCountFromCustomFormation();
}


// ------------------------------------------------------------
// CORE LOGIC
// ------------------------------------------------------------

void UUnitSpawnComponent::BuildSpawnFormationOffsets(int32 Count, float Spacing, TArray<FVector>& OutOffsets, const FRotator& Facing) const
{
    GenerateFormationOffsets(OutOffsets, Count, Spacing);

    if (OutOffsets.IsEmpty())
    	return;

    const FQuat Rot = FRotator(0.f, Facing.Yaw, 0.f).Quaternion();
    for (FVector& V : OutOffsets)
    {
        V = Rot.RotateVector(V);
    }
}

void UUnitSpawnComponent::GenerateFormationOffsets(TArray<FVector>& OutOffsets, int32 Count, float Spacing) const
{
    switch (SpawnFormation)
    {
        case ESpawnFormation::Line:
            UFormationMathLibrary::GetLineOffsets(Count, Spacing, OutOffsets);
            break;
    	
        case ESpawnFormation::Column:
            UFormationMathLibrary::GetColumnOffsets(Count, Spacing, OutOffsets);
            break;
    	
        case ESpawnFormation::Square:
            UFormationMathLibrary::GetSquareOffsets(Count, Spacing, OutOffsets);
            break;
    	
        case ESpawnFormation::Wedge:
            UFormationMathLibrary::GetWedgeOffsets(Count, Spacing, OutOffsets);
            break;
    	
        case ESpawnFormation::Blob:
            UFormationMathLibrary::GetBlobOffsets(Count, Spacing, OutOffsets);
            break;
    	
        case ESpawnFormation::Custom:
            UFormationMathLibrary::GetCustomGridOffsets(Count, CustomFormationDimensions.Y, CustomFormationDimensions.X, Spacing, OutOffsets);
            break;
    	
        default:
            UFormationMathLibrary::GetSquareOffsets(Count, Spacing, OutOffsets);
            break;
    }
}

// ------------------------------------------------------------
// REP NOTIFIES 
// ------------------------------------------------------------
void UUnitSpawnComponent::OnRep_UnitToSpawn()
{
	OnUnitClassChanged.Broadcast(UnitToSpawn);
}
void UUnitSpawnComponent::OnRep_UnitsPerSpawn()
{
	OnSpawnCountChanged.Broadcast(UnitsPerSpawn);
}
void UUnitSpawnComponent::OnRep_SpawnFormation()
{
	OnSpawnFormationChanged.Broadcast(SpawnFormation);
}
void UUnitSpawnComponent::OnRep_CustomFormationDimensions()
{
	OnCustomFormationDimensionsChanged.Broadcast(CustomFormationDimensions);
}

void UUnitSpawnComponent::UpdateSpawnCountFromCustomFormation()
{
    if (SpawnFormation == ESpawnFormation::Custom)
    {
        const int32 Desired = CustomFormationDimensions.X * CustomFormationDimensions.Y;
        if (UnitsPerSpawn != Desired)
        	SetUnitsPerSpawn(Desired);
    }
}