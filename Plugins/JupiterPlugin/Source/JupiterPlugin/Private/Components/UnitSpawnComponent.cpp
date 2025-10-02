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

    if (CustomFormationDimensions.X <= 0)
        CustomFormationDimensions.X = 1;

    if (CustomFormationDimensions.Y <= 0)
        CustomFormationDimensions.Y = 1;

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
    DOREPLIFETIME_CONDITION(UUnitSpawnComponent, SpawnFormation, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UUnitSpawnComponent, CustomFormationDimensions, COND_OwnerOnly);
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

void UUnitSpawnComponent::SetSpawnFormation(ESpawnFormation NewFormation)
{
    if (GetOwner() && GetOwner()->HasAuthority())
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
}

void UUnitSpawnComponent::SetCustomFormationDimensions(FIntPoint NewDimensions)
{
    NewDimensions.X = FMath::Max(1, NewDimensions.X);
    NewDimensions.Y = FMath::Max(1, NewDimensions.Y);

    if (GetOwner() && GetOwner()->HasAuthority())
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

void UUnitSpawnComponent::ServerSetSpawnFormation_Implementation(ESpawnFormation NewFormation)
{
    if (SpawnFormation != NewFormation)
    {
        SpawnFormation = NewFormation;
        OnRep_SpawnFormation();
    }
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

        TArray<FVector> SpawnOffsets;
        GenerateSpawnOffsets(SpawnOffsets, SpawnCount, Spacing);

        bool bSpawnedAnyUnit = false;

        for (int32 Index = 0; Index < SpawnOffsets.Num(); ++Index)
        {
            const FVector FormationLocation = SpawnLocation + SpawnOffsets[Index];

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

void UUnitSpawnComponent::OnRep_SpawnFormation()
{
    OnSpawnFormationChanged.Broadcast(SpawnFormation);
}

void UUnitSpawnComponent::OnRep_CustomFormationDimensions()
{
    OnCustomFormationDimensionsChanged.Broadcast(CustomFormationDimensions);
}

void UUnitSpawnComponent::GenerateSpawnOffsets(TArray<FVector>& OutOffsets, int32 SpawnCount, float Spacing) const
{
    OutOffsets.Reset();
    if (SpawnCount <= 0)
    {
        return;
    }

    OutOffsets.Reserve(SpawnCount);

    switch (SpawnFormation)
    {
        case ESpawnFormation::Line:
        {
            const float HalfWidth = (SpawnCount - 1) * 0.5f * Spacing;
            for (int32 Index = 0; Index < SpawnCount; ++Index)
            {
                const float OffsetX = (Index * Spacing) - HalfWidth;
                OutOffsets.Add(FVector(OffsetX, 0.f, 0.f));
            }
            break;
        }
        case ESpawnFormation::Column:
        {
            const float HalfHeight = (SpawnCount - 1) * 0.5f * Spacing;
            for (int32 Index = 0; Index < SpawnCount; ++Index)
            {
                const float OffsetY = (Index * Spacing) - HalfHeight;
                OutOffsets.Add(FVector(0.f, OffsetY, 0.f));
            }
            break;
        }
        case ESpawnFormation::Wedge:
        {
            int32 UnitsPlaced = 0;
            int32 Row = 0;
            while (UnitsPlaced < SpawnCount)
            {
                const int32 UnitsInRow = FMath::Min(Row + 1, SpawnCount - UnitsPlaced);
                const float RowHalfWidth = (UnitsInRow - 1) * 0.5f * Spacing;
                const float RowOffsetY = Row * Spacing;
                for (int32 IndexInRow = 0; IndexInRow < UnitsInRow; ++IndexInRow)
                {
                    const float OffsetX = (IndexInRow * Spacing) - RowHalfWidth;
                    OutOffsets.Add(FVector(OffsetX, RowOffsetY, 0.f));
                }

                UnitsPlaced += UnitsInRow;
                ++Row;
            }
            break;
        }
        case ESpawnFormation::Blob:
        {
            OutOffsets.Add(FVector::ZeroVector);

            if (SpawnCount > 1)
            {
                FRandomStream RandomStream;
                RandomStream.Initialize(FMath::Rand());

                const float MaxRadius = (Spacing <= 0.f) ? 0.f : Spacing * FMath::Sqrt(static_cast<float>(SpawnCount));

                for (int32 Index = 1; Index < SpawnCount; ++Index)
                {
                    const float Angle = RandomStream.FRandRange(0.f, 2.f * PI);
                    const float Radius = (MaxRadius > 0.f) ? RandomStream.FRandRange(0.f, MaxRadius) : 0.f;
                    const float OffsetX = Radius * FMath::Cos(Angle);
                    const float OffsetY = Radius * FMath::Sin(Angle);
                    OutOffsets.Add(FVector(OffsetX, OffsetY, 0.f));
                }
            }
            break;
        }
        case ESpawnFormation::Custom:
        {
            const int32 Columns = FMath::Max(1, CustomFormationDimensions.X);
            const int32 Rows = FMath::Max(1, CustomFormationDimensions.Y);
            const float HalfWidth = (Columns - 1) * 0.5f * Spacing;
            const float HalfHeight = (Rows - 1) * 0.5f * Spacing;

            for (int32 Index = 0; Index < SpawnCount; ++Index)
            {
                const int32 Column = Index % Columns;
                const int32 RowIndex = Index / Columns;

                const float OffsetX = (Column * Spacing) - HalfWidth;
                const float OffsetY = (RowIndex * Spacing) - HalfHeight;
                OutOffsets.Add(FVector(OffsetX, OffsetY, 0.f));
            }
            break;
        }
        case ESpawnFormation::Square:
        default:
        {
            const int32 Columns = FMath::Max(1, FMath::CeilToInt(FMath::Sqrt(static_cast<float>(SpawnCount))));
            const int32 Rows = FMath::Max(1, FMath::CeilToInt(static_cast<float>(SpawnCount) / static_cast<float>(Columns)));
            const float HalfWidth = (Columns - 1) * 0.5f * Spacing;
            const float HalfHeight = (Rows - 1) * 0.5f * Spacing;

            for (int32 Index = 0; Index < SpawnCount; ++Index)
            {
                const int32 Column = Index % Columns;
                const int32 RowIndex = Index / Columns;

                const float OffsetX = (Column * Spacing) - HalfWidth;
                const float OffsetY = (RowIndex * Spacing) - HalfHeight;
                OutOffsets.Add(FVector(OffsetX, OffsetY, 0.f));
            }
            break;
        }
    }

    if (OutOffsets.Num() > 0 && (SpawnFormation == ESpawnFormation::Wedge || SpawnFormation == ESpawnFormation::Blob))
    {
        FVector AverageOffset = FVector::ZeroVector;
        for (const FVector& Offset : OutOffsets)
        {
            AverageOffset += Offset;
        }

        AverageOffset /= static_cast<float>(OutOffsets.Num());

        for (FVector& Offset : OutOffsets)
        {
            Offset -= FVector(AverageOffset.X, AverageOffset.Y, 0.f);
        }
    }
}

