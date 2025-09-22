#include "JupiterPlugin/Public/Units/SoldierRts.h"
#include "Units/AI/AiControllerRts.h"
#include "Components/CommandComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WeaponMaster.h"
#include "DrawDebugHelpers.h"
#include "Containers/Set.h"
#include "Algo/BinarySearch.h"
#include "Algo/Sort.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
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
			Prim->SetCustomDepthStencilWriteMask(Highlight ? ERendererStencilMask::ERSM_255 : ERendererStencilMask::ERSM_Default);
			Prim->SetRenderCustomDepth(Highlight);
			Prim->SetCustomDepthStencilValue(Highlight ? SelectionStencilValue : 0);
			Prim->MarkRenderStateDirty();
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

void ASoldierRts::OnAreaAttackBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!ShouldUseComponentDetection())
		return;

    if (OtherActor == this) return;

    if (!IsValidSelectableActor(OtherActor))
		return;

    UpdateActorsInArea();

    if (!IsEnemyActor(OtherActor))
		return;

    ActorsInRange.AddUnique(OtherActor);
    HandleAutoEngage(OtherActor);
}

void ASoldierRts::OnAreaAttackEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!ShouldUseComponentDetection())
		return;

    if (OtherActor == this || !bCanAttack)
        return;

    if (!IsValidSelectableActor(OtherActor))
		return;

    UpdateActorsInArea();

    if (!IsEnemyActor(OtherActor))
		return;

    if (ActorsInRange.Remove(OtherActor) > 0)
		HandleTargetRemoval(OtherActor);
}

void ASoldierRts::OnAllyDetectionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!ShouldUseComponentDetection())
		return;

    if (OtherActor == this)
		return;

    if (!IsValidSelectableActor(OtherActor))
		return;

    UpdateActorsInArea();

    if (!IsFriendlyActor(OtherActor))
        return;

    AllyInRange.AddUnique(OtherActor);
}

void ASoldierRts::OnAllyDetectionEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!ShouldUseComponentDetection())
		return;

    if (OtherActor == this)
		return;

    if (!IsValidSelectableActor(OtherActor))
		return;

    UpdateActorsInArea();

    if (IsFriendlyActor(OtherActor))
		AllyInRange.Remove(OtherActor);
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
		return;

    if (!HasAuthority())
		return;

    if (!ensure(DetectionSettings.RefreshInterval > KINDA_SMALL_NUMBER))
		return;

    DetectionElapsedTime += DeltaSeconds;
    if (DetectionElapsedTime < DetectionSettings.RefreshInterval)
		return;

    DetectionElapsedTime = 0.f;
    EvaluateCalculatedDetection();
}

namespace
{
    struct FDetectionCandidate
    {
        FDetectionCandidate() = default;
        FDetectionCandidate(AActor* InActor, float InDistSq)
            : Actor(InActor)
            , DistanceSq(InDistSq)
        {
        }

        AActor* Actor = nullptr;
        float DistanceSq = 0.f;
    };

    template <typename AllocatorType>
    void InsertCandidateSorted(TArray<FDetectionCandidate, AllocatorType>& Candidates, AActor* Actor, float DistanceSq, int32 MaxCount)
    {
        const int32 InsertIndex = Algo::LowerBoundBy(Candidates, DistanceSq, &FDetectionCandidate::DistanceSq);
        Candidates.Insert(FDetectionCandidate(Actor, DistanceSq), InsertIndex);

        if (MaxCount > 0 && Candidates.Num() > MaxCount)
        {
            Candidates.Pop(false);
        }
    }
}

void ASoldierRts::EvaluateCalculatedDetection()
{
    UWorld* World = GetWorld();
    if (!World)
                return;

    const FVector SelfLocation = GetActorLocation();
    const float EnemyRangeSq = FMath::Square(AttackRange);
    const float AllyRangeSq = FMath::Square(AllyDetectionRange);
    const float MaxDetectionRange = FMath::Max(AttackRange, AllyDetectionRange);

    if (MaxDetectionRange <= KINDA_SMALL_NUMBER)
    {
                TArray<AActor*> EmptyEnemies;
                TArray<AActor*> EmptyAllies;
                ProcessDetectionResults(MoveTemp(EmptyEnemies), MoveTemp(EmptyAllies));
                return;
    }

    TArray<FOverlapResult, TInlineAllocator<32>> Overlaps;

    const FCollisionShape DetectionSphere = FCollisionShape::MakeSphere(MaxDetectionRange);
    FCollisionObjectQueryParams ObjectQueryParams;
    ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SoldierEvaluateCalculatedDetection), false, this);
    QueryParams.bReturnPhysicalMaterial = false;
    QueryParams.AddIgnoredActor(this);

    if (!World->OverlapMultiByObjectType(Overlaps, SelfLocation, FQuat::Identity, ObjectQueryParams, DetectionSphere, QueryParams))
    {
        TArray<AActor*> EmptyEnemies;
        TArray<AActor*> EmptyAllies;
        ProcessDetectionResults(MoveTemp(EmptyEnemies), MoveTemp(EmptyAllies));
        return;
    }

    const bool bPrioritizeClosestTargets = DetectionSettings.bPrioritizeClosestTargets;
    const int32 EnemyLimit = DetectionSettings.MaxEnemiesTracked;
    const int32 AllyLimit = DetectionSettings.MaxAlliesTracked;
    const bool bUnlimitedEnemies = EnemyLimit <= 0;
    const bool bUnlimitedAllies = AllyLimit <= 0;
    const int32 EnemyReserve = bUnlimitedEnemies ? Overlaps.Num() : FMath::Min(Overlaps.Num(), EnemyLimit);
    const int32 AllyReserve = bUnlimitedAllies ? Overlaps.Num() : FMath::Min(Overlaps.Num(), AllyLimit);

    TArray<AActor*> NewEnemies;
    TArray<AActor*> NewAllies;

    if (bPrioritizeClosestTargets)
    {
        TArray<FDetectionCandidate, TInlineAllocator<32>> EnemyCandidates;
        TArray<FDetectionCandidate, TInlineAllocator<32>> AllyCandidates;
        EnemyCandidates.Reserve(EnemyReserve);
        AllyCandidates.Reserve(AllyReserve);
        const bool bCapEnemies = EnemyLimit > 0;
        const bool bCapAllies = AllyLimit > 0;

        for (const FOverlapResult& Overlap : Overlaps)
        {
            AActor* CandidateActor = Overlap.GetActor();
            if (CandidateActor == nullptr || CandidateActor == this)
            {
                continue;
            }

            if (!IsValidSelectableActor(CandidateActor))
            {
                continue;
            }

            const float DistanceSq = FVector::DistSquared(SelfLocation, CandidateActor->GetActorLocation());
            const bool bWithinEnemyRange = DistanceSq <= EnemyRangeSq;
            const bool bWithinAllyRange = DistanceSq <= AllyRangeSq;

            if (!bWithinEnemyRange && !bWithinAllyRange)
            {
                continue;
            }

            const ETeams CandidateTeam = ISelectable::Execute_GetCurrentTeam(CandidateActor);
            if (bWithinEnemyRange && CandidateTeam != CurrentTeam)
            {
                if (bCapEnemies)
                {
                    if (EnemyCandidates.Num() >= EnemyLimit && DistanceSq >= EnemyCandidates.Last().DistanceSq)
                    {
                        continue;
                    }

                    InsertCandidateSorted(EnemyCandidates, CandidateActor, DistanceSq, EnemyLimit);
                }
                else
                {
                    EnemyCandidates.Emplace(CandidateActor, DistanceSq);
                }
            }
            else if (bWithinAllyRange && CandidateTeam == CurrentTeam)
            {
                if (bCapAllies)
                {
                    if (AllyCandidates.Num() >= AllyLimit && DistanceSq >= AllyCandidates.Last().DistanceSq)
                    {
                        continue;
                    }

                    InsertCandidateSorted(AllyCandidates, CandidateActor, DistanceSq, AllyLimit);
                }
                else
                {
                    AllyCandidates.Emplace(CandidateActor, DistanceSq);
                }
            }
        }

        if (!bCapEnemies && EnemyCandidates.Num() > 1)
        {
            Algo::SortBy(EnemyCandidates, &FDetectionCandidate::DistanceSq);
        }

        if (!bCapAllies && AllyCandidates.Num() > 1)
        {
            Algo::SortBy(AllyCandidates, &FDetectionCandidate::DistanceSq);
        }

        NewEnemies.Reserve(EnemyCandidates.Num());
        for (const FDetectionCandidate& Candidate : EnemyCandidates)
        {
            NewEnemies.Add(Candidate.Actor);
        }

        NewAllies.Reserve(AllyCandidates.Num());
        for (const FDetectionCandidate& Candidate : AllyCandidates)
        {
            NewAllies.Add(Candidate.Actor);
        }
    }
    else
    {
        NewEnemies.Reserve(EnemyReserve);
        NewAllies.Reserve(AllyReserve);

        for (const FOverlapResult& Overlap : Overlaps)
        {
            AActor* CandidateActor = Overlap.GetActor();
            if (CandidateActor == nullptr || CandidateActor == this)
            {
                continue;
            }

            if (!IsValidSelectableActor(CandidateActor))
            {
                continue;
            }

            const float DistanceSq = FVector::DistSquared(SelfLocation, CandidateActor->GetActorLocation());
            const bool bWithinEnemyRange = DistanceSq <= EnemyRangeSq;
            const bool bWithinAllyRange = DistanceSq <= AllyRangeSq;

            if (!bWithinEnemyRange && !bWithinAllyRange)
            {
                continue;
            }

            const ETeams CandidateTeam = ISelectable::Execute_GetCurrentTeam(CandidateActor);
            if (bWithinEnemyRange && CandidateTeam != CurrentTeam)
            {
                if (bUnlimitedEnemies || NewEnemies.Num() < EnemyLimit)
                {
                    NewEnemies.Add(CandidateActor);
                }
            }
            else if (bWithinAllyRange && CandidateTeam == CurrentTeam)
            {
                if (bUnlimitedAllies || NewAllies.Num() < AllyLimit)
                {
                    NewAllies.Add(CandidateActor);
                }
            }

            if (!bUnlimitedEnemies && !bUnlimitedAllies &&
                NewEnemies.Num() >= EnemyLimit && NewAllies.Num() >= AllyLimit)
            {
                break;
            }
        }
    }

    ProcessDetectionResults(MoveTemp(NewEnemies), MoveTemp(NewAllies));
}

void ASoldierRts::ProcessDetectionResults(TArray<AActor*>&& NewEnemies, TArray<AActor*>&& NewAllies)
{
    TArray<AActor*> PreviousEnemies = MoveTemp(ActorsInRange);

    ActorsInRange = MoveTemp(NewEnemies);
    AllyInRange = MoveTemp(NewAllies);

    if (DetectionSettings.bDebugDrawDetection)
                DrawAttackDebug(ActorsInRange, AllyInRange);

    if (PreviousEnemies.Num() == 0 && ActorsInRange.Num() == 0)
                return;

    TSet<AActor*, DefaultKeyFuncs<AActor*>, TInlineSetAllocator<8>> PreviousEnemySet;
    PreviousEnemySet.Reserve(PreviousEnemies.Num());
    for (AActor* PrevEnemy : PreviousEnemies)
    {
        if (PrevEnemy)
                        PreviousEnemySet.Add(PrevEnemy);
    }

    for (AActor* Enemy : ActorsInRange)
    {
        if (!Enemy)
                        continue;

        if (PreviousEnemySet.Remove(Enemy) == 0)
                        HandleAutoEngage(Enemy);
    }

    for (AActor* PrevEnemy : PreviousEnemySet)
    {
        if (PrevEnemy)
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

bool ASoldierRts::ShouldUseComponentDetection() const
{
    return DetectionSettings.bUseComponentOverlap;
}

void ASoldierRts::ConfigureDetectionComponent()
{
    if (!AreaAttack)
		return;

    AreaAttack->SetSphereRadius(AttackRange);
    if (AllyDetectionArea)
		AllyDetectionArea->SetSphereRadius(AllyDetectionRange);

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

