#include "Components/Unit/UnitOrderComponent.h"
#include "Components/Unit/UnitFormationComponent.h"
#include "Components/Unit/UnitSelectionComponent.h"
#include "Components/Patrol/UnitPatrolComponent.h"
#include "Interfaces/Selectable.h"


UUnitOrderComponent::UUnitOrderComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
}

void UUnitOrderComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* Owner = GetOwner())
    {
        SelectionComponent = Owner->FindComponentByClass<UUnitSelectionComponent>();
        FormationComponent = Owner->FindComponentByClass<UUnitFormationComponent>();
    }

    if (FormationComponent && bAutoReapplyCachedFormation)
    {
        FormationComponent->OnFormationStateChanged.AddDynamic(this, &UUnitOrderComponent::ReapplyCachedFormation);
    }
}

// -------------------------------------------------------------------------
// PUBLIC API
// -------------------------------------------------------------------------

void UUnitOrderComponent::IssueOrder(const FCommandData& CommandData)
{
    if (!SelectionComponent)
    	return;

    SelectionComponent->RemoveInvalidSelections();
    TArray<AActor*> SelectedUnits = SelectionComponent->GetSelectedActors();
    
    if (SelectedUnits.IsEmpty())
    	return;

    if (GetOwner()->HasAuthority())
    {
        DispatchOrder(CommandData, SelectedUnits);
    }
    else
    {
        ServerIssueOrder(CommandData, SelectedUnits);
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
    // CommandData.RequestingController = ... (Peut être rempli ici si besoin)

    IssueOrder(CommandData);
}

void UUnitOrderComponent::UpdateBehavior(ECombatBehavior NewBehavior)
{
    if (!SelectionComponent)
    	return;

    TArray<AActor*> SelectedUnits = SelectionComponent->GetSelectedActors();
    if (SelectedUnits.IsEmpty())
    	return;

    if (GetOwner()->HasAuthority())
    {
	    ApplyBehaviorToSelection(NewBehavior, SelectedUnits);
    }
    else
    {
	    ServerUpdateBehavior(NewBehavior, SelectedUnits);
    }
}

void UUnitOrderComponent::ReapplyCachedFormation()
{
    if (!FormationComponent || !SelectionComponent)
    	return;
    
    TArray<AActor*> SelectedUnits = SelectionComponent->GetSelectedActors();
    if (SelectedUnits.IsEmpty())
    	return;

    if (GetOwner()->HasAuthority())
    {
        if (const FCommandData* CachedCommand = FormationComponent->GetCachedFormationCommand())
            DispatchOrder(*CachedCommand, SelectedUnits);
    }
    else
    {
        ServerReapplyCachedFormation(SelectedUnits);
    }
}

// -------------------------------------------------------------------------
// SERVER RPCS
// -------------------------------------------------------------------------

void UUnitOrderComponent::ServerIssueOrder_Implementation(const FCommandData& CommandData, const TArray<AActor*>& UnitsToCommand)
{
    DispatchOrder(CommandData, UnitsToCommand);
}

void UUnitOrderComponent::ServerUpdateBehavior_Implementation(ECombatBehavior NewBehavior, const TArray<AActor*>& UnitsToCommand)
{
    ApplyBehaviorToSelection(NewBehavior, UnitsToCommand);
}

void UUnitOrderComponent::ServerReapplyCachedFormation_Implementation(const TArray<AActor*>& UnitsToCommand)
{
    if (FormationComponent)
    {
        if (const FCommandData* CachedCommand = FormationComponent->GetCachedFormationCommand())
            DispatchOrder(*CachedCommand, UnitsToCommand);
    }
}

// -------------------------------------------------------------------------
// INTERNAL LOGIC
// -------------------------------------------------------------------------

void UUnitOrderComponent::DispatchOrder(const FCommandData& OriginalCommandData, const TArray<AActor*>& Units)
{
    if (Units.IsEmpty())
    	return;

    FCommandData FinalCommandData = OriginalCommandData;
    if (FinalCommandData.Type == CommandPatrol)
    {
        PreparePatrolCommand(FinalCommandData, Units);
    }

    TArray<FCommandData> Commands;
    ApplyFormationToCommands(FinalCommandData, Units, Commands);

    for (int32 Index = 0; Index < Units.Num(); ++Index)
    {
        AActor* Unit = Units[Index];
        
        if (!IsValid(Unit) || !Unit->Implements<USelectable>())
        	continue;

        const FCommandData& UnitCommand = Commands.IsValidIndex(Index) ? Commands[Index] : FinalCommandData;
        
        if (ShouldIgnoreTarget(Unit, UnitCommand))
        	continue;

        ISelectable::Execute_CommandMove(Unit, UnitCommand);
    }

    OnOrdersDispatched.Broadcast(Units, FinalCommandData);
}

bool UUnitOrderComponent::PreparePatrolCommand(FCommandData& InOutCommand, const TArray<AActor*>& Units)
{
    if (InOutCommand.PatrolPath.Num() >= 2 && !InOutCommand.PatrolID.IsValid())
    {
        if (UUnitPatrolComponent* PatrolComp = GetOwner()->FindComponentByClass<UUnitPatrolComponent>())
        {
            FPatrolCreationParams Params;
            Params.Points = InOutCommand.PatrolPath;
            Params.Units = Units;
            Params.Type = InOutCommand.bPatrolLoop ? EPatrolType::Loop : EPatrolType::Once;
            Params.Name = FName("Patrol"); 
            Params.Color = FLinearColor::Blue;
            
            FGuid NewID = PatrolComp->CreatePatrol(Params);
            if (NewID.IsValid())
            {
                InOutCommand.PatrolID = NewID;
                return true;
            }
        }
    }
	
    return false;
}

void UUnitOrderComponent::ApplyFormationToCommands(const FCommandData& BaseCommand, const TArray<AActor*>& SelectedUnits, TArray<FCommandData>& OutCommands) const
{
    OutCommands.Reset();
	
    const bool bShouldApplyFormation = FormationComponent && bApplyFormationToMoveOrders &&
    	BaseCommand.Type != CommandAttack && BaseCommand.Target == nullptr;
	
    if (bShouldApplyFormation)
    {
        FormationComponent->BuildFormationCommands(BaseCommand, SelectedUnits, OutCommands);
    }
    else
    {
        OutCommands.Reserve(SelectedUnits.Num());
        // Fallback simple : tout le monde reçoit le même ordre
        for (int32 Index = 0; Index < SelectedUnits.Num(); ++Index)
        {
            OutCommands.Add(BaseCommand);
        }
    }
}

void UUnitOrderComponent::ApplyBehaviorToSelection(ECombatBehavior NewBehavior, const TArray<AActor*>& Units)
{
    for (AActor* Unit : Units)
    {
        if (IsValid(Unit) && Unit->Implements<USelectable>())
        {
            ISelectable::Execute_SetBehavior(Unit, NewBehavior);
        }
    }
}

bool UUnitOrderComponent::ShouldIgnoreTarget(AActor* Unit, const FCommandData& CommandData) const
{
    if (!bIgnoreFriendlyTargets || !CommandData.Target || !CommandData.Target->Implements<USelectable>())
        return false;

    if (IsValid(Unit) && Unit->Implements<USelectable>())
    {
    	const ETeams MyTeam = ISelectable::Execute_GetCurrentTeam(Unit);
    	const ETeams TargetTeam = ISelectable::Execute_GetCurrentTeam(CommandData.Target);
        
        return MyTeam == TargetTeam;
    }

    return false;
}