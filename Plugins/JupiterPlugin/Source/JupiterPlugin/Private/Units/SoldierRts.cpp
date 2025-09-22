#include "JupiterPlugin/Public/Units/SoldierRts.h"
#include "Units/AI/AiControllerRts.h"
#include "Components/CommandComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WeaponMaster.h"
#include "DrawDebugHelpers.h"
#include "JupiterGameState.h"
#include "Containers/Set.h"
#include "Engine/EngineTypes.h"
#include "Components/SoldierManagerComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"


// ------------------- Setup ---------------------
#pragma region Setup

class AJupiterGameState;

ASoldierRts::ASoldierRts(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AiControllerRtsClass;

    CommandComp = CreateDefaultSubobject<UCommandComponent>(TEXT("CommandComponent"));

    AllyDetectionRange = AttackRange;
}

void ASoldierRts::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    AIControllerClass = AiControllerRtsClass;
}

void ASoldierRts::BeginPlay()
{
    Super::BeginPlay();

	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ASoldierRts::TryRegisterPrompt);

	ActorsInRange.Reset();
	AllyInRange.Reset();

	DetectionElapsedTime = 0.f;

    if (WeaponClass && CurrentTeam != ETeams::HiveMind)
    {
		CurrentWeapon = Cast<UWeaponMaster>(AddComponentByClass(*WeaponClass, false, FTransform::Identity, true));
		if (CurrentWeapon)
		{
			CurrentWeapon->SetAiOwner(this);
			HaveWeapon = true;
		}
	}
}

void ASoldierRts::TryRegisterPrompt()
{
	if (GetOwner()->GetNetMode() == NM_DedicatedServer)
		return;
	
	if (AJupiterGameState* GS = GetWorld()->GetGameState<AJupiterGameState>())
		SoldierManager = GS->GetSoldierManager();

	if (SoldierManager)
		SoldierManager->RegisterSoldier(this);
}

void ASoldierRts::Destroyed()
{
    Super::Destroyed();

    for (AActor* Soldier : ActorsInRange)
    {
        if (ASoldierRts* SoldierRts = Cast<ASoldierRts>(Soldier))
                SoldierRts->UpdateActorsInArea();
    }
}

void ASoldierRts::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (USoldierManagerComponent* Manager = PC->FindComponentByClass<USoldierManagerComponent>())
			Manager->UnregisterSoldier(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ASoldierRts::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

	if (HasAuthority())
	{
		AAiControllerRts* ControllerAi = Cast<AAiControllerRts>(NewController);
		if (CommandComp && ControllerAi)
		{
			CommandComp->SetOwnerAIController(ControllerAi);
			SetAIController(ControllerAi);

			ControllerAi->OnStartAttack.AddDynamic(this, &ASoldierRts::OnStartAttack);
		}	
	}
}

auto ASoldierRts::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const -> void
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASoldierRts, CombatBehavior);
}

#pragma endregion


// ------------------- Set & Get AiController ---------------------
#pragma region Set & Get AiController

void ASoldierRts::SetAIController(AAiControllerRts* AiController)
{
	if (AiController)
		AIController = AiController;
}

AAiControllerRts* ASoldierRts::GetAiController() const
{
	return AIController;
}

UCommandComponent* ASoldierRts::GetCommandComponent() const
{
	return CommandComp;
}

#pragma endregion


void ASoldierRts::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

    if(GetCharacterMovement()->Velocity.Length() != 0 && !bIsMoving)
    {
        bIsMoving = true;
        StartWalkingEvent_Delegate.Broadcast();
    }
    else if(GetCharacterMovement()->Velocity.Length() == 0 && bIsMoving)
    {
        bIsMoving = false;
        EndWalkingEvent_Delegate.Broadcast();
    }
}


// Delegates Call
void ASoldierRts::NetMulticast_Unreliable_CallOnStartAttack_Implementation()
{
	AttackEvent_Delegate.Broadcast();
}

void ASoldierRts::NetMulticast_Unreliable_CallOnStartWalking_Implementation()
{
	StartWalkingEvent_Delegate.Broadcast();
}

void ASoldierRts::NetMulticast_Unreliable_CallOnEndWalking_Implementation()
{
	EndWalkingEvent_Delegate.Broadcast();
}


// ------------------- Selection ---------------------
#pragma region Selection

void ASoldierRts::Select()
{
	Selected = true;
	Highlight(Selected);

	OnSelected.Broadcast(Selected);
}

void ASoldierRts::Deselect()
{
	Selected = false;
	Highlight(Selected);

	OnSelected.Broadcast(Selected);
}

void ASoldierRts::Highlight(const bool Highlight)
{
	TArray<UPrimitiveComponent*> Components;
	GetComponents<UPrimitiveComponent>(Components);

	for (UPrimitiveComponent* VisualComp : Components)
	{
		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(VisualComp))
			Prim->SetRenderCustomDepth(Highlight);
	}
}

bool ASoldierRts::GetIsSelected_Implementation()
{
	return Selected;
}

#pragma endregion


// ------------------- Movement ---------------------
#pragma region Movement

void ASoldierRts::CommandMove_Implementation(FCommandData CommandData)
{
	ISelectable::CommandMove_Implementation(CommandData);
	GetCommandComponent()->CommandMoveToLocation(CommandData);
}

FCommandData ASoldierRts::GetCurrentCommand_Implementation()
{
	if (CommandComp)
		return CommandComp->GetCurrentCommand();
	
	return FCommandData();
}

#pragma endregion


// ------------------- Attack ---------------------
#pragma region Attack

void ASoldierRts::OnStartAttack(AActor* Target)
{
    if (!bCanAttack || !Target || Target == this)
		return;

    if (GetCurrentWeapon())
		GetCurrentWeapon()->AIShoot(Target);

    if (Target->Implements<UDamageable>())
    {
        IDamageable::Execute_TakeDamage(Target, this);
        NetMulticast_Unreliable_CallOnStartAttack();
    }
}

void ASoldierRts::TakeDamage_Implementation(AActor* DamageOwner)
{
    IDamageable::TakeDamage_Implementation(DamageOwner);

    if (!bCanAttack || !DamageOwner || DamageOwner == this)
		return;

	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
		FString::Printf(TEXT("[%s] Took Damage from [%s] | Behavior=%s"),
			*GetName(), *DamageOwner->GetName(), *UEnum::GetValueAsString(CombatBehavior)));
	
    const FCommandData AttackCommand = MakeAttackCommand(DamageOwner);

    if (!Execute_GetIsInAttack(this) && CombatBehavior != ECombatBehavior::Passive)
		IssueAttackOrder(AttackCommand);

    if (IsEnemyActor(DamageOwner))
		NotifyAlliesOfThreat(DamageOwner, AttackCommand);
}

bool ASoldierRts::GetIsInAttack_Implementation()
{
	if (bCanAttack)
		return GetAiController()->HasAttackTarget();

	return false;
}

bool ASoldierRts::GetCanAttack_Implementation()
{
	return bCanAttack;
}

#pragma endregion


// ------------------- Behavior ---------------------
#pragma region Behavior

void ASoldierRts::SetBehavior_Implementation(const ECombatBehavior NewBehavior)
{
	ISelectable::SetBehavior_Implementation(NewBehavior);
	CombatBehavior = NewBehavior;

	if (AIController)
	{
		AIController->SetupVariables();

		if (CombatBehavior == ECombatBehavior::Passive)
		{
			AIController->StopAttack();
		}
		else if (CombatBehavior == ECombatBehavior::Aggressive)
		{
			UpdateActorsInArea();

			for (AActor* Enemy : ActorsInRange)
			{
				if (IsEnemyActor(Enemy))
				{
					IssueAttackOrder(MakeAttackCommand(Enemy));
					break;
				}
			}
		}
	}
}

ECombatBehavior ASoldierRts::GetBehavior_Implementation()
{
	return CombatBehavior;
}

void ASoldierRts::UpdateActorsInArea()
{
    ActorsInRange.RemoveAll([this](AActor* Actor)
    {
        return !IsEnemyActor(Actor);
    });

    AllyInRange.RemoveAll([this](AActor* Actor)
    {
        return !IsFriendlyActor(Actor);
    });
}


void ASoldierRts::OnRep_CombatBehavior()
{
    OnBehaviorUpdate.Broadcast();
}

#pragma endregion


bool ASoldierRts::IsValidSelectableActor(const AActor* Actor) const
{
    return Actor && IsValid(Actor) && Actor->Implements<USelectable>();
}

bool ASoldierRts::IsFriendlyActor(AActor* Actor) const
{
    return IsValidSelectableActor(Actor) && ISelectable::Execute_GetCurrentTeam(Actor) == CurrentTeam;
}

bool ASoldierRts::IsEnemyActor(AActor* Actor) const
{
    return IsValidSelectableActor(Actor) && ISelectable::Execute_GetCurrentTeam(Actor) != CurrentTeam;
}

void ASoldierRts::ProcessDetectionResults(TArray<AActor*> NewEnemies, TArray<AActor*> NewAllies)
{
    TArray<AActor*> PreviousEnemies = ActorsInRange;

    ActorsInRange = MoveTemp(NewEnemies);
    AllyInRange = MoveTemp(NewAllies);

    if (DetectionSettings.bDebugDrawDetection)
		DrawAttackDebug(ActorsInRange, AllyInRange);

    TSet<AActor*> PreviousEnemySet;
    PreviousEnemySet.Reserve(PreviousEnemies.Num());
    for (AActor* PrevEnemy : PreviousEnemies)
    {
        if (PrevEnemy)
			PreviousEnemySet.Add(PrevEnemy);
    }

    TSet<AActor*> CurrentEnemySet;
    CurrentEnemySet.Reserve(ActorsInRange.Num());
    for (AActor* Enemy : ActorsInRange)
    {
        if (!Enemy)
			continue;

        CurrentEnemySet.Add(Enemy);

        if (!PreviousEnemySet.Contains(Enemy))
			HandleAutoEngage(Enemy);
    }

    for (AActor* PrevEnemy : PreviousEnemies)
    {
        if (PrevEnemy && !CurrentEnemySet.Contains(PrevEnemy))
			HandleTargetRemoval(PrevEnemy);
    }
}

void ASoldierRts::DrawAttackDebug(const TArray<AActor*>& DetectedEnemies, const TArray<AActor*>& DetectedAllies) const
{
    if (!GetWorld())
		return;

    const FColor DebugColor = DetectionSettings.DebugColor.ToFColor(true);
    DrawDebugSphere(GetWorld(), GetActorLocation(), AttackRange, 16, DebugColor, false, DetectionSettings.DebugDuration);

    if (AllyDetectionRange > KINDA_SMALL_NUMBER)
    {
        const FColor AllyColor = FColor::Green;
        DrawDebugSphere(GetWorld(), GetActorLocation(), AllyDetectionRange, 16, AllyColor, false, DetectionSettings.DebugDuration);
    }

    if (!DetectionSettings.bDrawTargetLines)
		return;

    const FVector StartLocation = GetActorLocation();

    for (AActor* Enemy : DetectedEnemies)
    {
        if (!Enemy)
			continue;

        DrawDebugLine(GetWorld(), StartLocation, Enemy->GetActorLocation(), DebugColor, false, DetectionSettings.DebugDuration, 0, DetectionSettings.DebugLineThickness);
    }

    for (AActor* Ally : DetectedAllies)
    {
        if (!Ally)
        	continue;

        DrawDebugLine(GetWorld(), StartLocation, Ally->GetActorLocation(), FColor::Green, false, DetectionSettings.DebugDuration, 0, DetectionSettings.DebugLineThickness);
    }
}

bool ASoldierRts::ShouldAutoEngage() const
{
    return bCanAttack && CombatBehavior == ECombatBehavior::Aggressive && AIController && CommandComp;
}

FCommandData ASoldierRts::MakeAttackCommand(AActor* Target) const
{
    FCommandData CommandData;
    CommandData.Type = ECommandType::CommandAttack;
    CommandData.Target = Target;
    CommandData.Location = Target ? Target->GetActorLocation() : GetActorLocation();
    CommandData.SourceLocation = GetActorLocation();
    CommandData.Rotation = GetActorRotation();
	
    return CommandData;
}

void ASoldierRts::IssueAttackOrder(const FCommandData& CommandData)
{
    if (!CommandComp || !CommandData.Target || !IsValid(CommandData.Target))
		return;

    GetCommandComponent()->CommandMoveToLocation(CommandData);
}

void ASoldierRts::HandleAutoEngage(AActor* Target)
{
    if (!ShouldAutoEngage() || !IsEnemyActor(Target))
		return;

    if (!AIController->HasAttackTarget())
		IssueAttackOrder(MakeAttackCommand(Target));
}

void ASoldierRts::HandleTargetRemoval(AActor* OtherActor)
{
    if (!AIController)
		return;

    UpdateActorsInArea();

    const bool bControllerAggressive = AIController->GetCombatBehavior() == ECombatBehavior::Aggressive;

    if (CombatBehavior == ECombatBehavior::Aggressive && AIController->HasAttackTarget() && AIController->GetCurrentCommand().Target == OtherActor)
    {
        for (AActor* PotentialTarget : ActorsInRange)
        {
            if (IsEnemyActor(PotentialTarget))
            {
                IssueAttackOrder(MakeAttackCommand(PotentialTarget));
                return;
            }
        }
    }

    if (ActorsInRange.Num() == 0 && bControllerAggressive)
		AIController->ResetAttack();
}

void ASoldierRts::NotifyAlliesOfThreat(AActor* Threat, const FCommandData& CommandData)
{
    if (!Threat || !IsEnemyActor(Threat))
		return;

    for (int32 Index = AllyInRange.Num() - 1; Index >= 0; --Index)
    {
        AActor* Ally = AllyInRange[Index];
        if (!IsFriendlyActor(Ally))
        {
            AllyInRange.RemoveAt(Index);
            continue;
        }

        if (!Execute_GetCanAttack(Ally))
			continue;

        const ECombatBehavior AllyBehavior = Execute_GetBehavior(Ally);
        if ((AllyBehavior == ECombatBehavior::Neutral || AllyBehavior == ECombatBehavior::Aggressive) && !ISelectable::Execute_GetIsInAttack(Ally))
        {
            Execute_CommandMove(Ally, CommandData);
        }
    }
}


// ------------------- Getter ---------------------
#pragma region Getter

float ASoldierRts::GetAttackRange() const
{
	return AttackRange;
}

float ASoldierRts::GetAllyDetectionRange() const
{
	return AllyDetectionRange;
}

float ASoldierRts::GetAttackCooldown() const
{
	return AttackCooldown;
}

ECombatBehavior ASoldierRts::GetCombatBehavior() const
{
	return CombatBehavior;
}

float ASoldierRts::GetRangedStopDistance() const
{
	return RangedStopDistance;
}

float ASoldierRts::GetMeleeStopDistanceFactor() const
{
	return MeleeStopDistanceFactor;
}

UWeaponMaster* ASoldierRts::GetCurrentWeapon()
{
	return CurrentWeapon;
}

bool ASoldierRts::GetHaveWeapon()
{
	return HaveWeapon;
}

ETeams ASoldierRts::GetCurrentTeam_Implementation()
{
	return CurrentTeam;
}

#pragma endregion

