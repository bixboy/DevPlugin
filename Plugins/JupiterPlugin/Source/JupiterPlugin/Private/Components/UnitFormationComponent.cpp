#include "Components/UnitFormationComponent.h"

#include "Components/UnitSelectionComponent.h"
#include "Data/FormationDataAsset.h"
#include "Net/UnrealNetwork.h"

namespace
{
    constexpr float TwoPi = 6.28318530718f;
}

UUnitFormationComponent::UUnitFormationComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UUnitFormationComponent::BeginPlay()
{
    Super::BeginPlay();

    SelectionComponent = GetOwner() ? GetOwner()->FindComponentByClass<UUnitSelectionComponent>() : nullptr;
}

void UUnitFormationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(UUnitFormationComponent, FormationSpacing, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UUnitFormationComponent, CurrentFormation, COND_OwnerOnly);
}

void UUnitFormationComponent::SetFormation(EFormation NewFormation)
{
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        CurrentFormation = NewFormation;
        BroadcastFormationChanged();
    }
    else
    {
        ServerSetFormation(NewFormation);
    }
}

void UUnitFormationComponent::SetFormationSpacing(float NewSpacing)
{
    const float ClampedSpacing = FMath::Max(NewSpacing, MinimumSpacing);

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        FormationSpacing = ClampedSpacing;
        BroadcastFormationChanged();
    }
    else
    {
        ServerSetSpacing(ClampedSpacing);
    }
}

const FCommandData* UUnitFormationComponent::GetCachedFormationCommand() const
{
    return bHasCachedCommand ? &CachedFormationCommand : nullptr;
}

void UUnitFormationComponent::BuildFormationCommands(const FCommandData& BaseCommand, const TArray<AActor*>& Units, TArray<FCommandData>& OutCommands)
{
    OutCommands.Reset();
    OutCommands.Reserve(Units.Num());

    if (bCacheLastFormationCommand)
    {
        CacheFormationCommand(BaseCommand, Units);
    }

    for (int32 Index = 0; Index < Units.Num(); ++Index)
    {
        FCommandData Command = BaseCommand;
        FVector Offset = CalculateOffset(Index, Units.Num());

        if (bRespectCommandRotation)
        {
            Command.Location = Command.SourceLocation + Command.Rotation.RotateVector(Offset);
        }
        else
        {
            Command.Location = Command.SourceLocation + Offset;
        }

        OutCommands.Add(Command);
    }
}

bool UUnitFormationComponent::HasGroupSelection() const
{
    return SelectionComponent && SelectionComponent->HasGroupSelection();
}

void UUnitFormationComponent::ServerSetFormation_Implementation(EFormation NewFormation)
{
    CurrentFormation = NewFormation;
    BroadcastFormationChanged();
}

void UUnitFormationComponent::ServerSetSpacing_Implementation(float NewSpacing)
{
    FormationSpacing = FMath::Max(NewSpacing, MinimumSpacing);
    BroadcastFormationChanged();
}

void UUnitFormationComponent::OnRep_Formation()
{
    BroadcastFormationChanged();
}

void UUnitFormationComponent::OnRep_Spacing()
{
    BroadcastFormationChanged();
}

FVector UUnitFormationComponent::CalculateOffset(int32 Index, int32 TotalUnits) const
{
    FVector Offset = FVector::ZeroVector;
    if (const UFormationDataAsset* FormationData = GetFormationData())
    {
        Offset = FormationData->Offset;

        switch (CurrentFormation)
        {
        case EFormation::Square:
        {
            const int32 GridSize = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(TotalUnits)));
            Offset.X = (Index / GridSize) * FormationSpacing - ((GridSize - 1) * FormationSpacing * 0.5f);
            Offset.Y = (Index % GridSize) * FormationSpacing - ((GridSize - 1) * FormationSpacing * 0.5f);
            break;
        }
        case EFormation::Blob:
        {
            if (Index != 0)
            {
                FRandomStream Stream(Index * 137);
                const float Angle = (Index / static_cast<float>(TotalUnits)) * TwoPi;
                const float Radius = Stream.FRandRange(-0.5f, 0.5f) * FormationSpacing;
                Offset.X += Radius * FMath::Cos(Angle);
                Offset.Y += Radius * FMath::Sin(Angle);
            }
            break;
        }
        default:
        {
            const float Multiplier = FMath::Floor((Index + 1) / 2.f) * FormationSpacing;
            const bool bAlternate = FormationData->Alternate;
            if (bAlternate && Index % 2 == 0)
            {
                Offset.Y = -Offset.Y;
            }

            if (bAlternate)
            {
                Offset *= Multiplier;
            }
            else
            {
                Offset *= Index * FormationSpacing;
            }
            break;
        }
        }
    }
    else
    {
        Offset = FVector(0.f, Index * FormationSpacing, 0.f);
    }

    return Offset;
}

void UUnitFormationComponent::CacheFormationCommand(const FCommandData& CommandData, const TArray<AActor*>& Units)
{
    if (Units.IsEmpty())
    {
        bHasCachedCommand = false;
        return;
    }

    CachedFormationCommand = CommandData;
    bHasCachedCommand = true;
}

void UUnitFormationComponent::BroadcastFormationChanged()
{
    OnFormationStateChanged.Broadcast();
}

UFormationDataAsset* UUnitFormationComponent::GetFormationData() const
{
    for (UFormationDataAsset* Asset : FormationDataAssets)
    {
        if (Asset && Asset->FormationType == CurrentFormation)
        {
            return Asset;
        }
    }

    return nullptr;
}

