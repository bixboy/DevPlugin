#include "JupiterPlugin/Public/Units/SoldierRts.h"
#include "Units/AI/AiControllerRts.h"
#include "Components/CommandComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WeaponMaster.h"
#include "DrawDebugHelpers.h"
#include "Containers/Set.h"
#include "EngineUtils.h"
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

        AllyDetectionArea = CreateDefaultSubobject<USphereComponent>(TEXT("AllyDetectionArea"));
        AllyDetectionArea->SetupAttachment(RootComponent);

        AreaAttack->OnComponentBeginOverlap.AddDynamic(this, &ASoldierRts::OnAreaAttackBeginOverlap);
        AreaAttack->OnComponentEndOverlap.AddDynamic(this, &ASoldierRts::OnAreaAttackEndOverlap);

        AllyDetectionArea->OnComponentBeginOverlap.AddDynamic(this, &ASoldierRts::OnAllyDetectionBeginOverlap);
        AllyDetectionArea->OnComponentEndOverlap.AddDynamic(this, &ASoldierRts::OnAllyDetectionEndOverlap);

        AllyDetectionRange = AttackRange;
}

void ASoldierRts::OnConstruction(const FTransform& Transform)
{
        Super::OnConstruction(Transform);

        AIControllerClass = AiControllerRtsClass;

        ConfigureDetectionComponent();
}

void ASoldierRts::BeginPlay()
{
        Super::BeginPlay();

        ConfigureDetectionComponent();

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
                        SoldierRts->UpdateActorsInArea();
        }
}

#if WITH_EDITOR
void ASoldierRts::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
        Super::PostEditChangeProperty(PropertyChangedEvent);

        ConfigureDetectionComponent();
}
#endif

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

        UpdateAttackDetection(DeltaSeconds);
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
                        Prim->SetCustomDepthStencilValue(Highlight ? SelectionStencilValue : 0);
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
        if (!bCanAttack || !Target || Target == this)
        {
                return;
        }

        if (GetCurrentWeapon())
        {
                GetCurrentWeapon()->AIShoot(Target);
        }

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

void ASoldierRts::OnAreaAttackBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                           UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
        if (!ShouldUseComponentDetection())
        {
                return;
        }

        if (OtherActor == this) return;

        if (!IsValidSelectableActor(OtherActor))
        {
                return;
        }

        UpdateActorsInArea();

        if (!IsEnemyActor(OtherActor))
        {
                return;
        }

        ActorsInRange.AddUnique(OtherActor);
        HandleAutoEngage(OtherActor);
}

void ASoldierRts::OnAreaAttackEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                         UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
        if (!ShouldUseComponentDetection())
        {
                return;
        }

        if (OtherActor == this || !bCanAttack) return;

        if (!IsValidSelectableActor(OtherActor))
        {
                return;
        }

        UpdateActorsInArea();

        if (!IsEnemyActor(OtherActor))
        {
                return;
        }

        if (ActorsInRange.Remove(OtherActor) > 0)
        {
                HandleTargetRemoval(OtherActor);
        }
}

void ASoldierRts::OnAllyDetectionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                           UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
        if (!ShouldUseComponentDetection())
        {
                return;
        }

        if (OtherActor == this)
        {
                return;
        }

        if (!IsValidSelectableActor(OtherActor))
        {
                return;
        }

        UpdateActorsInArea();

        if (!IsFriendlyActor(OtherActor))
        {
                return;
        }

        AllyInRange.AddUnique(OtherActor);
}

void ASoldierRts::OnAllyDetectionEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                         UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
        if (!ShouldUseComponentDetection())
        {
                return;
        }

        if (OtherActor == this)
        {
                return;
        }

        if (!IsValidSelectableActor(OtherActor))
        {
                return;
        }

        UpdateActorsInArea();

        if (IsFriendlyActor(OtherActor))
        {
                AllyInRange.Remove(OtherActor);
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
    if (!ShouldUseComponentDetection())
    {
        return;
    }

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

void ASoldierRts::UpdateAttackDetection(float DeltaSeconds)
{
    if (!bCanAttack || ShouldUseComponentDetection())
    {
        return;
    }

    if (!HasAuthority())
    {
        return;
    }

    if (!ensure(DetectionSettings.RefreshInterval > KINDA_SMALL_NUMBER))
    {
        return;
    }

    DetectionElapsedTime += DeltaSeconds;
    if (DetectionElapsedTime < DetectionSettings.RefreshInterval)
    {
        return;
    }

    DetectionElapsedTime = 0.f;

    EvaluateCalculatedDetection();
}

namespace
{
        struct FDetectionCandidate
        {
                FDetectionCandidate() = default;
                FDetectionCandidate(AActor* InActor, float InDistSq, bool bInEnemy)
                    : Actor(InActor)
                    , DistanceSq(InDistSq)
                    , bIsEnemy(bInEnemy)
                {
                }

                AActor* Actor = nullptr;
                float DistanceSq = 0.f;
                bool bIsEnemy = false;
        };
}

void ASoldierRts::EvaluateCalculatedDetection()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FVector SelfLocation = GetActorLocation();
    const float EnemyRangeSq = FMath::Square(AttackRange);
    const float AllyRangeSq = FMath::Square(AllyDetectionRange);

    TArray<FDetectionCandidate> Candidates;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* CandidateActor = *It;
        if (CandidateActor == nullptr || CandidateActor == this)
        {
            continue;
        }

        if (!IsValidSelectableActor(CandidateActor))
        {
            continue;
        }

        const float DistanceSq = FVector::DistSquared(SelfLocation, CandidateActor->GetActorLocation());
        const bool bIsEnemy = IsEnemyActor(CandidateActor);
        const bool bIsFriendly = !bIsEnemy && IsFriendlyActor(CandidateActor);

        if (bIsEnemy && DistanceSq <= EnemyRangeSq)
        {
            Candidates.Emplace(CandidateActor, DistanceSq, true);
        }
        else if (bIsFriendly && DistanceSq <= AllyRangeSq)
        {
            Candidates.Emplace(CandidateActor, DistanceSq, false);
        }
    }

    if (DetectionSettings.bPrioritizeClosestTargets)
    {
        Candidates.Sort([](const FDetectionCandidate& Lhs, const FDetectionCandidate& Rhs)
        {
            return Lhs.DistanceSq < Rhs.DistanceSq;
        });
    }

    TArray<AActor*> NewEnemies;
    TArray<AActor*> NewAllies;

    const int32 MaxEnemies = DetectionSettings.MaxEnemiesTracked > 0 ? DetectionSettings.MaxEnemiesTracked : TNumericLimits<int32>::Max();
    const int32 MaxAllies = DetectionSettings.MaxAlliesTracked > 0 ? DetectionSettings.MaxAlliesTracked : TNumericLimits<int32>::Max();

    for (const FDetectionCandidate& Candidate : Candidates)
    {
        if (Candidate.bIsEnemy)
        {
            if (NewEnemies.Num() >= MaxEnemies)
            {
                continue;
            }

            NewEnemies.Add(Candidate.Actor);
        }
        else
        {
            if (NewAllies.Num() >= MaxAllies)
            {
                continue;
            }

            NewAllies.Add(Candidate.Actor);
        }
    }

    ProcessDetectionResults(MoveTemp(NewEnemies), MoveTemp(NewAllies));
}

void ASoldierRts::ProcessDetectionResults(TArray<AActor*>&& NewEnemies, TArray<AActor*>&& NewAllies)
{
    TArray<AActor*> PreviousEnemies = ActorsInRange;

    ActorsInRange = MoveTemp(NewEnemies);
    AllyInRange = MoveTemp(NewAllies);

    if (DetectionSettings.bDebugDrawDetection)
    {
        DrawAttackDebug(ActorsInRange, AllyInRange);
    }

    TSet<AActor*> PreviousEnemySet;
    PreviousEnemySet.Reserve(PreviousEnemies.Num());
    for (AActor* PrevEnemy : PreviousEnemies)
    {
        if (PrevEnemy)
        {
            PreviousEnemySet.Add(PrevEnemy);
        }
    }

    TSet<AActor*> CurrentEnemySet;
    CurrentEnemySet.Reserve(ActorsInRange.Num());
    for (AActor* Enemy : ActorsInRange)
    {
        if (!Enemy)
        {
            continue;
        }

        CurrentEnemySet.Add(Enemy);

        if (!PreviousEnemySet.Contains(Enemy))
        {
            HandleAutoEngage(Enemy);
        }
    }

    for (AActor* PrevEnemy : PreviousEnemies)
    {
        if (PrevEnemy && !CurrentEnemySet.Contains(PrevEnemy))
        {
            HandleTargetRemoval(PrevEnemy);
        }
    }
}

void ASoldierRts::DrawAttackDebug(const TArray<AActor*>& DetectedEnemies, const TArray<AActor*>& DetectedAllies) const
{
    if (!GetWorld())
    {
        return;
    }

    const FColor DebugColor = DetectionSettings.DebugColor.ToFColor(true);
    DrawDebugSphere(GetWorld(), GetActorLocation(), AttackRange, 16, DebugColor, false, DetectionSettings.DebugDuration);

    const bool bShouldDrawAllySphere = AllyDetectionRange > KINDA_SMALL_NUMBER;
    if (bShouldDrawAllySphere)
    {
        const FColor AllyColor = FColor::Green;
        DrawDebugSphere(GetWorld(), GetActorLocation(), AllyDetectionRange, 16, AllyColor, false, DetectionSettings.DebugDuration);
    }

    if (!DetectionSettings.bDrawTargetLines)
    {
        return;
    }

    const FVector StartLocation = GetActorLocation();

    for (AActor* Enemy : DetectedEnemies)
    {
        if (!Enemy)
        {
            continue;
        }

        DrawDebugLine(GetWorld(), StartLocation, Enemy->GetActorLocation(), DebugColor, false, DetectionSettings.DebugDuration, 0, DetectionSettings.DebugLineThickness);
    }

    for (AActor* Ally : DetectedAllies)
    {
        if (!Ally)
        {
            continue;
        }

        DrawDebugLine(GetWorld(), StartLocation, Ally->GetActorLocation(), FColor::Green, false, DetectionSettings.DebugDuration, 0, DetectionSettings.DebugLineThickness);
    }
}

bool ASoldierRts::ShouldUseComponentDetection() const
{
    return DetectionSettings.bUseComponentOverlap;
}

void ASoldierRts::ConfigureDetectionComponent()
{
    if (!AreaAttack)
    {
        return;
    }

    AreaAttack->SetSphereRadius(AttackRange);
    if (AllyDetectionArea)
    {
        AllyDetectionArea->SetSphereRadius(AllyDetectionRange);
    }

    if (ShouldUseComponentDetection())
    {
        AreaAttack->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        AreaAttack->SetGenerateOverlapEvents(true);
        if (AllyDetectionArea)
        {
            AllyDetectionArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            AllyDetectionArea->SetGenerateOverlapEvents(true);
        }
    }
    else
    {
        AreaAttack->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        AreaAttack->SetGenerateOverlapEvents(false);
        if (AllyDetectionArea)
        {
            AllyDetectionArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            AllyDetectionArea->SetGenerateOverlapEvents(false);
        }

        ActorsInRange.Reset();
        AllyInRange.Reset();
    }

    DetectionElapsedTime = 0.f;
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

        if (!ISelectable::Execute_GetCanAttack(Ally))
			continue;

        const ECombatBehavior AllyBehavior = ISelectable::Execute_GetBehavior(Ally);
        if ((AllyBehavior == ECombatBehavior::Neutral || AllyBehavior == ECombatBehavior::Aggressive) && !ISelectable::Execute_GetIsInAttack(Ally))
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

#pragma endregion


// ------------------- Team ---------------------
#pragma region Team

ETeams ASoldierRts::GetCurrentTeam_Implementation()
{
	return CurrentTeam;
}

#pragma endregion

