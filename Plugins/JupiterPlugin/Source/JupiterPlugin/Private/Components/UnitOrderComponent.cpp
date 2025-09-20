#include "Components/UnitOrderComponent.h"

#include "Components/UnitFormationComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "Interfaces/Selectable.h"
#include "Net/UnrealNetwork.h"

UUnitOrderComponent::UUnitOrderComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
}

void UUnitOrderComponent::BeginPlay()
{
    Super::BeginPlay();

    SelectionComponent = GetOwner() ? GetOwner()->FindComponentByClass<UUnitSelectionComponent>() : nullptr;
    FormationComponent = GetOwner() ? GetOwner()->FindComponentByClass<UUnitFormationComponent>() : nullptr;

    if (FormationComponent && bAutoReapplyCachedFormation)
    {
        FormationComponent->OnFormationStateChanged.AddDynamic(this, &UUnitOrderComponent::ReapplyCachedFormation);
    }
}

void UUnitOrderComponent::IssueOrder(const FCommandData& CommandData)
{
    if (!SelectionComponent)
    {
        return;
    }

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        DispatchOrder(CommandData);
    }
    else
    {
        ServerIssueOrder(CommandData);
    }
}

void UUnitOrderComponent::IssueSimpleOrder(ECommandType Type, FVector Location, FRotator Rotation, AActor* Target, float Radius)
{
    FCommandData CommandData;
    CommandData.Type = Type;
    CommandData.Location = Location;
    CommandData.SourceLocation = Location;
    CommandData.Rotation = Rotation;
    CommandData.Target = Target;
    CommandData.Radius = Radius;
    CommandData.RequestingController = nullptr;

    IssueOrder(CommandData);
}

void UUnitOrderComponent::UpdateBehavior(ECombatBehavior NewBehavior)
{
    if (!SelectionComponent)
    {
        return;
    }

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        ApplyBehaviorToSelection(NewBehavior);
    }
    else
    {
        ServerUpdateBehavior(NewBehavior);
    }
}

void UUnitOrderComponent::ReapplyCachedFormation()
{
    if (!FormationComponent)
    {
        return;
    }

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        if (const FCommandData* CachedCommand = FormationComponent->GetCachedFormationCommand())
        {
            DispatchOrder(*CachedCommand);
        }
    }
    else
    {
        ServerReapplyCachedFormation();
    }
}

void UUnitOrderComponent::ServerIssueOrder_Implementation(const FCommandData& CommandData)
{
    DispatchOrder(CommandData);
}

void UUnitOrderComponent::ServerReapplyCachedFormation_Implementation()
{
    if (FormationComponent)
    {
        if (const FCommandData* CachedCommand = FormationComponent->GetCachedFormationCommand())
        {
            DispatchOrder(*CachedCommand);
        }
    }
}

void UUnitOrderComponent::ServerUpdateBehavior_Implementation(ECombatBehavior NewBehavior)
{
    ApplyBehaviorToSelection(NewBehavior);
}

void UUnitOrderComponent::DispatchOrder(const FCommandData& CommandData)
{
    if (!SelectionComponent)
    {
        return;
    }

    SelectionComponent->RemoveInvalidSelections();

    TArray<AActor*> SelectedUnits = SelectionComponent->GetSelectedActors();
    if (SelectedUnits.IsEmpty())
    {
        return;
    }

    TArray<FCommandData> Commands;
    ApplyFormationToCommands(CommandData, SelectedUnits, Commands);

    const int32 NumCommands = Commands.Num();
    for (int32 Index = 0; Index < SelectedUnits.Num(); ++Index)
    {
        AActor* Unit = SelectedUnits[Index];
        if (!Unit || !Unit->Implements<USelectable>())
        {
            continue;
        }

        const FCommandData& UnitCommand = Commands.IsValidIndex(Index) ? Commands[Index] : CommandData;
        if (ShouldIgnoreTarget(Unit, UnitCommand))
        {
            continue;
        }

        ISelectable::Execute_CommandMove(Unit, UnitCommand);
    }

    OnOrdersDispatched.Broadcast(SelectedUnits, CommandData);
}

void UUnitOrderComponent::ApplyFormationToCommands(const FCommandData& BaseCommand, const TArray<AActor*>& SelectedUnits, TArray<FCommandData>& OutCommands) const
{
    OutCommands.Reset();

    const bool bShouldApplyFormation = FormationComponent && bApplyFormationToMoveOrders && BaseCommand.Type != ECommandType::CommandAttack && BaseCommand.Target == nullptr;

    if (bShouldApplyFormation)
    {
        FormationComponent->BuildFormationCommands(BaseCommand, SelectedUnits, OutCommands);
    }
    else
    {
        OutCommands.Reserve(SelectedUnits.Num());
        for (int32 Index = 0; Index < SelectedUnits.Num(); ++Index)
        {
            OutCommands.Add(BaseCommand);
        }
    }
}

void UUnitOrderComponent::ApplyBehaviorToSelection(ECombatBehavior NewBehavior)
{
    if (!SelectionComponent)
    {
        return;
    }

    const TArray<AActor*> SelectedUnits = SelectionComponent->GetSelectedActors();
    for (AActor* Unit : SelectedUnits)
    {
        if (Unit && Unit->Implements<USelectable>())
        {
            ISelectable::Execute_SetBehavior(Unit, NewBehavior);
        }
    }
}

bool UUnitOrderComponent::ShouldIgnoreTarget(AActor* Unit, const FCommandData& CommandData) const
{
    if (!bIgnoreFriendlyTargets || !CommandData.Target || !CommandData.Target->Implements<USelectable>())
    {
        return false;
    }

    if (!Unit || !Unit->Implements<USelectable>())
    {
        return false;
    }

    return ISelectable::Execute_GetCurrentTeam(Unit) == ISelectable::Execute_GetCurrentTeam(CommandData.Target);
}

