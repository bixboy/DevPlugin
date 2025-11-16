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
// Set Unit Class
// ------------------------------------------------------------
void UUnitSpawnComponent::SetUnitToSpawn(TSubclassOf<ASoldierRts> NewUnitClass)
{
    if (GetOwner()->HasAuthority())
    {
        UnitToSpawn = NewUnitClass;
        OnRep_UnitToSpawn();
    }
    else
    {
        ServerSetUnitClass(NewUnitClass);
    }
}

// ------------------------------------------------------------
// SpawnUnits From Mouse
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

// ------------------------------------------------------------
// SpawnUnits (with transform)
// ------------------------------------------------------------
void UUnitSpawnComponent::SpawnUnitsWithTransform(const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
    if (!UnitToSpawn)
        return;

    if (GetOwner()->HasAuthority())
    {
	    ServerSpawnUnits(SpawnLocation, SpawnRotation);
    }
    else
    {
    	ServerSpawnUnits(SpawnLocation, SpawnRotation);   
    }
}

// ------------------------------------------------------------
// UnitsPerSpawn
// ------------------------------------------------------------
void UUnitSpawnComponent::SetUnitsPerSpawn(int32 NewSpawnCount)
{
    NewSpawnCount = FMath::Max(1, NewSpawnCount);

    if (SpawnFormation == ESpawnFormation::Custom)
    {
        NewSpawnCount = FMath::Max(1, CustomFormationDimensions.X * CustomFormationDimensions.Y);
    }

    if (GetOwner()->HasAuthority())
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

// ------------------------------------------------------------
// BuildSpawnFormationOffsets
// ------------------------------------------------------------
void UUnitSpawnComponent::BuildSpawnFormationOffsets(int32 Count, float Spacing, TArray<FVector>& OutOffsets, const FRotator& Facing) const
{
    GenerateFormationOffsets(OutOffsets, Count, Spacing);

    if (OutOffsets.IsEmpty())
        return;

    // Apply facing rotation once (cheap)
    const FQuat Rot = FRotator(0.f, Facing.Yaw, 0.f).Quaternion();
    for (FVector& V : OutOffsets)
        V = Rot.RotateVector(V);
}

// ------------------------------------------------------------
// SetSpawnFormation
// ------------------------------------------------------------
void UUnitSpawnComponent::SetSpawnFormation(ESpawnFormation NewFormation)
{
    if (GetOwner()->HasAuthority())
    {
        if (SpawnFormation != NewFormation)
        {
            SpawnFormation = NewFormation;
            OnRep_SpawnFormation();
        }
    }
    else
    {
        ServerSetSpawnFormation(NewFormation);
    }

    UpdateSpawnCountFromCustomFormation();
}

// ------------------------------------------------------------
// SetCustomFormationDimensions
// ------------------------------------------------------------
void UUnitSpawnComponent::SetCustomFormationDimensions(FIntPoint NewDimensions)
{
    NewDimensions.X = FMath::Max(1, NewDimensions.X);
    NewDimensions.Y = FMath::Max(1, NewDimensions.Y);

    if (GetOwner()->HasAuthority())
    {
        if (CustomFormationDimensions != NewDimensions)
        {
            CustomFormationDimensions = NewDimensions;
            OnRep_CustomFormationDimensions();
        }
    }
    else
    {
        ServerSetCustomFormationDimensions(NewDimensions);
    }

    UpdateSpawnCountFromCustomFormation();
}

// ------------------------------------------------------------
// SERVER IMPLEMENTATIONS
// ------------------------------------------------------------
void UUnitSpawnComponent::ServerSetUnitClass_Implementation(TSubclassOf<ASoldierRts> NewUnitClass)
{
    UnitToSpawn = NewUnitClass;
    OnRep_UnitToSpawn();
}

void UUnitSpawnComponent::ServerSetUnitsPerSpawn_Implementation(int32 NewCount)
{
    NewCount = FMath::Max(1, NewCount);

    if (SpawnFormation == ESpawnFormation::Custom)
    {
        NewCount = FMath::Max(1, CustomFormationDimensions.X * CustomFormationDimensions.Y);
    }

    if (UnitsPerSpawn != NewCount)
    {
        UnitsPerSpawn = NewCount;
        OnRep_UnitsPerSpawn();
    }
}

void UUnitSpawnComponent::ServerSetSpawnFormation_Implementation(ESpawnFormation NewFormation)
{
    if (SpawnFormation != NewFormation)
    {
        SpawnFormation = NewFormation;
        OnRep_SpawnFormation();
    }

    UpdateSpawnCountFromCustomFormation();
}

void UUnitSpawnComponent::ServerSetCustomFormationDimensions_Implementation(FIntPoint NewDimensions)
{
    NewDimensions.X = FMath::Max(1, NewDimensions.X);
    NewDimensions.Y = FMath::Max(1, NewDimensions.Y);

    if (CustomFormationDimensions != NewDimensions)
    {
        CustomFormationDimensions = NewDimensions;
        OnRep_CustomFormationDimensions();
    }

    UpdateSpawnCountFromCustomFormation();
}

void UUnitSpawnComponent::ServerSpawnUnits_Implementation(const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
    if (!UnitToSpawn)
        return;

    if (UWorld* World = GetWorld())
    {
        const int32 Count = FMath::Max(UnitsPerSpawn, 1);
        const float Spacing = FMath::Max(FormationSpacing, 0.0f);

        TArray<FVector> Offsets;
        GenerateFormationOffsets(Offsets, Count, Spacing);

        const FRotator FinalRot(0.f, SpawnRotation.Yaw, 0.f);

        FActorSpawnParameters Params;
        Params.Owner = GetOwner();
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        bool bSpawned = false;

        for (const FVector& Offset : Offsets)
        {
            const FVector FinalPos = SpawnLocation + Offset;

            ASoldierRts* Spawned = World->SpawnActor<ASoldierRts>(UnitToSpawn, FinalPos, FinalRot, Params);
            bSpawned |= (Spawned != nullptr);
        }

        if (bSpawned && !bNotifyOnServerOnly)
        {
            OnUnitClassChanged.Broadcast(UnitToSpawn);
        }
    }
}

// ------------------------------------------------------------
// REPLICATION NOTIFY
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

// ------------------------------------------------------------
// Custom formation auto-correction
// ------------------------------------------------------------
void UUnitSpawnComponent::UpdateSpawnCountFromCustomFormation()
{
    if (SpawnFormation != ESpawnFormation::Custom)
        return;

    const int32 Desired = FMath::Max(1, CustomFormationDimensions.X * CustomFormationDimensions.Y);

    if (UnitsPerSpawn != Desired)
        SetUnitsPerSpawn(Desired);
}

// ------------------------------------------------------------
// Generate offsets for formation
// ------------------------------------------------------------
void UUnitSpawnComponent::GenerateFormationOffsets(TArray<FVector>& OutOffsets, int32 Count, float Spacing) const
{
    OutOffsets.Reset();
    if (Count <= 0)
        return;

    OutOffsets.Reserve(Count);
    const float HalfStep = Spacing * 0.5f;

    switch (SpawnFormation)
    {
        case ESpawnFormation::Line:
        {
            const float Half = (Count - 1) * HalfStep;
            for (int32 i = 0; i < Count; ++i)
                OutOffsets.Add(FVector(i * Spacing - Half, 0, 0));
        		
            break;
        }

        case ESpawnFormation::Column:
        {
            const float Half = (Count - 1) * HalfStep;
            for (int32 i = 0; i < Count; ++i)
                OutOffsets.Add(FVector(0, i * Spacing - Half, 0));
        		
            break;
        }

        case ESpawnFormation::Wedge:
        {
            int32 Placed = 0;
            for (int32 Row = 0; Placed < Count; ++Row)
            {
                const int32 RowCount = FMath::Min(Row + 1, Count - Placed);
                const float Half = (RowCount - 1) * HalfStep;

                for (int32 c = 0; c < RowCount; ++c)
                    OutOffsets.Add(FVector(c * Spacing - Half, Row * Spacing, 0));

                Placed += RowCount;
            }
            break;
        }

        case ESpawnFormation::Blob:
        {
            OutOffsets.Add(FVector::ZeroVector);

            if (Count > 1)
            {
                const float MaxRadius = Spacing * FMath::Sqrt(static_cast<float>(Count));
                FRandomStream R(FMath::Rand());

                for (int32 i = 1; i < Count; ++i)
                {
                    const float Angle = R.FRandRange(0.f, 2 * PI);
                    const float Radius = R.FRandRange(0.f, MaxRadius);
                    OutOffsets.Add(FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), 0.f));
                }
            }

            // Recenter
            FVector Avg = FVector::ZeroVector;
            for (const FVector& V : OutOffsets)
                Avg += V;

            Avg /= OutOffsets.Num();

            for (FVector& V : OutOffsets)
                V -= FVector(Avg.X, Avg.Y, 0);

            break;
        }

        case ESpawnFormation::Custom:
        {
            const int32 W = CustomFormationDimensions.X;
            const int32 H = CustomFormationDimensions.Y;
            const float HW = (W - 1) * HalfStep;
            const float HH = (H - 1) * HalfStep;

            for (int32 i = 0; i < Count; ++i)
            {
                const int32 X = i % W;
                const int32 Y = i / W;
                OutOffsets.Add(FVector(X * Spacing - HW, Y * Spacing - HH, 0));
            }
            break;
        }

        default: // Square
        {
            const int32 W = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(Count)));
            const int32 H = FMath::CeilToInt(float(Count) / W);
            const float HW = (W - 1) * HalfStep;
            const float HH = (H - 1) * HalfStep;

            for (int32 i = 0; i < Count; ++i)
            {
                const int32 X = i % W;
                const int32 Y = i / W;
                OutOffsets.Add(FVector(X * Spacing - HW, Y * Spacing - HH, 0));
            }
        		
            break;
        }
    }
}
