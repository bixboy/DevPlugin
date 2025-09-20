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

	if (CombatBehavior == ECombatBehavior::Neutral || CombatBehavior == ECombatBehavior::Aggressive)
	{
		FCommandData NewCommandData;
		NewCommandData.Type = ECommandType::CommandAttack;
		NewCommandData.Target = DamageOwner;

		if (!ISelectable::Execute_GetIsInAttack(this))
		{
			GetCommandComponent()->CommandMoveToLocation(NewCommandData);	
		}

		// Handle Ally Support
		for (AActor* Ally : AllyInRange)
		{
			if (!ISelectable::Execute_GetCanAttack(Ally))
				continue;
			
			const ECombatBehavior TempEnum = Execute_GetBehavior(Ally);
			
			if (DamageOwner && Execute_GetCurrentTeam(Ally) == CurrentTeam && !ISelectable::Execute_GetIsInAttack(Ally))
			{
				if (TempEnum == ECombatBehavior::Neutral || TempEnum == ECombatBehavior::Aggressive)
				{
					ISelectable::Execute_CommandMove(Ally, NewCommandData);	
				}
			}
		}
	}
}

void ASoldierRts::OnAreaAttackBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                           UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == this) return;

	if (OtherActor->Implements<USelectable>())
	{
		if (Execute_GetCurrentTeam(OtherActor) == CurrentTeam)
		{
			AllyInRange.AddUnique(OtherActor);
			return;
		}

		UpdateActorsInArea();
		ActorsInRange.AddUnique(OtherActor);

		if (AIController && bCanAttack && CombatBehavior == ECombatBehavior::Aggressive && !AIController->HasAttackTarget())
		{
			FCommandData CommandData;
			CommandData.Type = ECommandType::CommandAttack;
			CommandData.Target = OtherActor;
			GetCommandComponent()->CommandMoveToLocation(CommandData);
		}
	}
}

void ASoldierRts::OnAreaAttackEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                         UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == this || !bCanAttack) return;

	if (OtherActor->Implements<USelectable>())
	{
		if (Execute_GetCurrentTeam(OtherActor) == CurrentTeam)
		{
			AllyInRange.Remove(OtherActor);
			return;
		}

		UpdateActorsInArea();

		if (ActorsInRange.Contains(OtherActor))
			ActorsInRange.Remove(OtherActor);

		// Handle Aggressive Combat Behavior
		if (CombatBehavior == ECombatBehavior::Aggressive && AIController && AIController->HasAttackTarget() &&
			AIController->GetCurrentCommand().Target == OtherActor)
		{
			AActor* NewTarget = nullptr;
			if (ActorsInRange.Num() > 0)
			{
				NewTarget = ActorsInRange[0];
			}

			if (NewTarget)
			{
				FCommandData NewCommandData;
				NewCommandData.Type = ECommandType::CommandAttack;
				NewCommandData.Target = NewTarget;
				GetCommandComponent()->CommandMoveToLocation(NewCommandData);
			}
			else if (AIController->GetCombatBehavior() == ECombatBehavior::Aggressive)
			{
				AIController->ResetAttack();
			}
		}

		if (ActorsInRange.Num() == 0 && AIController)
		{
			if (AIController->GetCombatBehavior() == ECombatBehavior::Aggressive)
				AIController->ResetAttack();
		}
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
			if (ActorsInRange.Num() > 0)
			{
				FCommandData NewCommandData;
				NewCommandData.Type = ECommandType::CommandAttack;
				NewCommandData.Target = ActorsInRange[0];
				GetCommandComponent()->CommandMoveToLocation(NewCommandData);
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
	// Remove invalid actors from area lists
	ActorsInRange.RemoveAll([](AActor* Actor) { return !IsValid(Actor); });
	AllyInRange.RemoveAll([](AActor* Actor) { return !IsValid(Actor); });
}


void ASoldierRts::OnRep_CombatBehavior()
{
	OnBehaviorUpdate.Broadcast();
}

#pragma endregion


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

