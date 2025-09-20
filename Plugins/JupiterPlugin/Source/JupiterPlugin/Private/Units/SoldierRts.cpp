#include "JupiterPlugin/Public/Units/SoldierRts.h"
#include "Units/AI/AiControllerRts.h"
#include "Components/CommandComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WeaponMaster.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

// ------------------- Setup ---------------------
#pragma region Setup

ASoldierRts::ASoldierRts(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AiControllerRtsClass;

	CommandComp = CreateDefaultSubobject<UCommandComponent>(TEXT("CommandComponent"));
	
	AreaAttack = CreateDefaultSubobject<USphereComponent>(TEXT("AreaAttack"));
	AreaAttack->SetupAttachment(RootComponent);

	AreaAttack->OnComponentBeginOverlap.AddDynamic(this, &ASoldierRts::OnAreaAttackBeginOverlap);
	AreaAttack->OnComponentEndOverlap.AddDynamic(this, &ASoldierRts::OnAreaAttackEndOverlap);
}

void ASoldierRts::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	AIControllerClass = AiControllerRtsClass;
}

void ASoldierRts::BeginPlay()
{
	Super::BeginPlay();
	
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

void ASoldierRts::Destroyed()
{
	Super::Destroyed();

	for (AActor* Soldier : ActorsInRange)
	{
		if (ASoldierRts* SoldierRts = Cast<ASoldierRts>(Soldier))
		{
			SoldierRts->UpdateActorsInArea();
		}
	}
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
	{
		AIController = AiController;
	}
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
		{
			Prim->SetRenderCustomDepth(Highlight);
		}
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
	{
		return CommandComp->GetCurrentCommand();	
	}
	
	return FCommandData();
}

#pragma endregion


// ------------------- Attack ---------------------
#pragma region Attack

void ASoldierRts::OnStartAttack(AActor* Target)
{
	if (!bCanAttack) return;
	
	if (GetCurrentWeapon())
	{
		GetCurrentWeapon()->AIShoot(Target);
	}
	else
	{
		IDamageable::Execute_TakeDamage(Target, this);
		NetMulticast_Unreliable_CallOnStartAttack();
	}
}

void ASoldierRts::TakeDamage_Implementation(AActor* DamageOwner)
{
        IDamageable::TakeDamage_Implementation(DamageOwner);

        if (!bCanAttack || !DamageOwner || DamageOwner == this)
        {
                return;
        }

        if (CombatBehavior == ECombatBehavior::Passive)
        {
                return;
        }

        const FCommandData AttackCommand = MakeAttackCommand(DamageOwner);

        if (!ISelectable::Execute_GetIsInAttack(this))
        {
                IssueAttackOrder(AttackCommand);
        }

        if (IsEnemyActor(DamageOwner))
        {
                NotifyAlliesOfThreat(DamageOwner, AttackCommand);
        }
}

void ASoldierRts::OnAreaAttackBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                           UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
        if (OtherActor == this) return;

        if (!IsValidSelectableActor(OtherActor))
        {
                return;
        }

        UpdateActorsInArea();

        if (IsFriendlyActor(OtherActor))
        {
                AllyInRange.AddUnique(OtherActor);
                return;
        }

        ActorsInRange.AddUnique(OtherActor);
        HandleAutoEngage(OtherActor);
}

void ASoldierRts::OnAreaAttackEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                         UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
        if (OtherActor == this || !bCanAttack) return;

        if (!IsValidSelectableActor(OtherActor))
        {
                return;
        }

        UpdateActorsInArea();

        if (IsFriendlyActor(OtherActor))
        {
                AllyInRange.Remove(OtherActor);
                return;
        }

        if (ActorsInRange.Remove(OtherActor) > 0)
        {
                HandleTargetRemoval(OtherActor);
        }
}

bool ASoldierRts::GetIsInAttack_Implementation()
{
	if (bCanAttack)
	{
		return GetAiController()->HasAttackTarget();
	}

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
                        AIController->ResetAttack();
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

bool ASoldierRts::IsFriendlyActor(const AActor* Actor) const
{
        return IsValidSelectableActor(Actor) && ISelectable::Execute_GetCurrentTeam(Actor) == CurrentTeam;
}

bool ASoldierRts::IsEnemyActor(const AActor* Actor) const
{
        return IsValidSelectableActor(Actor) && ISelectable::Execute_GetCurrentTeam(Actor) != CurrentTeam;
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
        {
                return;
        }

        GetCommandComponent()->CommandMoveToLocation(CommandData);
}

void ASoldierRts::HandleAutoEngage(AActor* Target)
{
        if (!ShouldAutoEngage() || !IsEnemyActor(Target))
        {
                return;
        }

        if (!AIController->HasAttackTarget())
        {
                IssueAttackOrder(MakeAttackCommand(Target));
        }
}

void ASoldierRts::HandleTargetRemoval(AActor* OtherActor)
{
        if (!AIController)
        {
                return;
        }

        UpdateActorsInArea();

        const bool bControllerAggressive = AIController->GetCombatBehavior() == ECombatBehavior::Aggressive;

        if (CombatBehavior == ECombatBehavior::Aggressive && AIController->HasAttackTarget() &&
                AIController->GetCurrentCommand().Target == OtherActor)
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
        {
                AIController->ResetAttack();
        }
}

void ASoldierRts::NotifyAlliesOfThreat(AActor* Threat, const FCommandData& CommandData)
{
        if (!Threat || !IsEnemyActor(Threat))
        {
                return;
        }

        for (int32 Index = AllyInRange.Num() - 1; Index >= 0; --Index)
        {
                AActor* Ally = AllyInRange[Index];
                if (!IsFriendlyActor(Ally))
                {
                        AllyInRange.RemoveAt(Index);
                        continue;
                }

                if (!ISelectable::Execute_GetCanAttack(Ally))
                {
                        continue;
                }

                const ECombatBehavior AllyBehavior = ISelectable::Execute_GetBehavior(Ally);
                if ((AllyBehavior == ECombatBehavior::Neutral || AllyBehavior == ECombatBehavior::Aggressive) &&
                        !ISelectable::Execute_GetIsInAttack(Ally))
                {
                        ISelectable::Execute_CommandMove(Ally, CommandData);
                }
        }
}


// ------------------- Getter ---------------------
#pragma region Getter

float ASoldierRts::GetAttackRange() const
{
	return AttackRange;
}

float ASoldierRts::GetAttackCooldown() const
{
	return AttackCooldown;
}

ECombatBehavior ASoldierRts::GetCombatBehavior() const
{
	return CombatBehavior;
}

UWeaponMaster* ASoldierRts::GetCurrentWeapon()
{
	return CurrentWeapon;
}

bool ASoldierRts::GetHaveWeapon()
{
	return HaveWeapon;
}

#pragma endregion


// ------------------- Team ---------------------
#pragma region Team

ETeams ASoldierRts::GetCurrentTeam_Implementation()
{
	return CurrentTeam;
}

#pragma endregion

