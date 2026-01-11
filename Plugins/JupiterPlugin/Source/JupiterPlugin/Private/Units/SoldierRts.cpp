#include "Units/SoldierRts.h"
#include "Components/Combat/CommandComponent.h"
#include "Components/Unit/SoldierManagerComponent.h"
#include "Components/Combat/WeaponMaster.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "AI/AiControllerRts.h"
#include "Containers/Set.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Core/JupiterGameState.h"


ASoldierRts::ASoldierRts(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
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

    ActorsInRange.Reset();
    AllyInRange.Reset();
    bIsMoving = false;

    if (HasAuthority())
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimerForNextTick(this, &ASoldierRts::TryRegisterPrompt);
        }
    }

    if (WeaponClass && CurrentTeam != ETeams::HiveMind)
    {
        CurrentWeapon = Cast<UWeaponMaster>(AddComponentByClass(*WeaponClass, false, FTransform::Identity, true));
        if (CurrentWeapon)
        {
            CurrentWeapon->SetAiOwner(this);
            bHasWeapon = true;
        }
    }
}

void ASoldierRts::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (HasAuthority() && SoldierManager)
    {
        SoldierManager->UnregisterSoldier(this);
        SoldierManager = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void ASoldierRts::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    if (!HasAuthority())
        return;

    if (AAiControllerRts* ControllerAi = Cast<AAiControllerRts>(NewController))
    {
        if (CommandComp)
            CommandComp->SetOwnerAIController(ControllerAi);

        SetAIController(ControllerAi);
        ControllerAi->OnStartAttack.AddDynamic(this, &ASoldierRts::OnStartAttack);
    }
}

void ASoldierRts::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ASoldierRts, CombatBehavior);
}

void ASoldierRts::TryRegisterPrompt()
{
    if (!HasAuthority())
        return;

    UWorld* World = GetWorld();
    if (!World)
        return;

    if (AJupiterGameState* GameState = World->GetGameState<AJupiterGameState>())
    {
        SoldierManager = GameState->GetSoldierManager();
        if (SoldierManager)
        {
            SoldierManager->RegisterSoldier(this);
            return;
        }
    }

    World->GetTimerManager().SetTimerForNextTick(this, &ASoldierRts::TryRegisterPrompt);
}

void ASoldierRts::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    const bool bCurrentlyMoving = !GetCharacterMovement()->Velocity.IsNearlyZero();
    if (bCurrentlyMoving == bIsMoving)
        return;

    bIsMoving = bCurrentlyMoving;
    if (bIsMoving)
    {
        StartWalkingEvent_Delegate.Broadcast();
    }
    else
    {
        EndWalkingEvent_Delegate.Broadcast();
    }
}

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

void ASoldierRts::Select()
{
    bIsSelected = true;
    Highlight(bIsSelected);
    OnSelected.Broadcast(bIsSelected);
}

void ASoldierRts::Deselect()
{
    bIsSelected = false;
    Highlight(bIsSelected);
    OnSelected.Broadcast(bIsSelected);
}

void ASoldierRts::Highlight(bool bHighlight)
{
    TArray<UPrimitiveComponent*> Components;
    GetComponents<UPrimitiveComponent>(Components);

    for (UPrimitiveComponent* VisualComp : Components)
    {
        if (!VisualComp)
            continue;

        VisualComp->SetRenderCustomDepth(bHighlight);
        VisualComp->SetCustomDepthStencilValue(bHighlight ? SelectionStencilValue : 0);
    }
}

bool ASoldierRts::GetIsSelected_Implementation()
{
    return bIsSelected;
}

void ASoldierRts::CommandMove_Implementation(FCommandData CommandData)
{
    ISelectable::CommandMove_Implementation(CommandData);

    if (CommandComp)
        CommandComp->CommandMoveToLocation(CommandData);
}

FCommandData ASoldierRts::GetCurrentCommand_Implementation()
{
    return CommandComp ? CommandComp->GetCurrentCommand() : FCommandData();
}

void ASoldierRts::OnStartAttack(AActor* Target)
{
    if (!bCanAttack || !Target || Target == this)
        return;

    if (UWeaponMaster* Weapon = GetCurrentWeapon())
    {
        Weapon->AIShoot(Target);
    }

    if (Target->Implements<UDamageable>())
    {
        Execute_TakeDamage(Target, this);
        NetMulticast_Unreliable_CallOnStartAttack();
    }
}

void ASoldierRts::TakeDamage_Implementation(AActor* DamageOwner)
{
    IDamageable::TakeDamage_Implementation(DamageOwner);

    if (!bCanAttack || !DamageOwner || DamageOwner == this)
        return;

    UE_LOG(LogTemp, Log, TEXT("[%s] Took damage from [%s] | Behavior=%s"),
        *GetName(),
        *DamageOwner->GetName(),
        *UEnum::GetValueAsString(CombatBehavior));

    const FCommandData AttackCommand = MakeAttackCommand(DamageOwner);

    if (!Execute_GetIsInAttack(this) && CombatBehavior != ECombatBehavior::Passive)
    {
        IssueAttackOrder(AttackCommand);
    }

    if (IsEnemyActor(DamageOwner))
    {
        NotifyAlliesOfThreat(DamageOwner, AttackCommand);
    }
}

bool ASoldierRts::GetIsInAttack_Implementation()
{
    return bCanAttack && AIController && AIController->HasAttackTarget();
}

bool ASoldierRts::GetCanAttack_Implementation()
{
    return bCanAttack;
}

void ASoldierRts::SetAIController(AAiControllerRts* AiController)
{
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

void ASoldierRts::SetBehavior_Implementation(const ECombatBehavior NewBehavior)
{
    ISelectable::SetBehavior_Implementation(NewBehavior);
    CombatBehavior = NewBehavior;

    if (!AIController)
        return;

    AIController->SetupVariables();

    switch (CombatBehavior)
    {
        case ECombatBehavior::Passive:
            AIController->StopAttack();
            break;

        case ECombatBehavior::Aggressive:
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
            break;
        }

        default:
            break;
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
        return !IsValid(Actor) || !IsEnemyActor(Actor);
    });

    AllyInRange.RemoveAll([this](AActor* Actor)
    {
        return !IsValid(Actor) || !IsFriendlyActor(Actor);
    });
}

void ASoldierRts::OnRep_CombatBehavior()
{
    OnBehaviorUpdate.Broadcast();
}

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
    if (DetectionSettings.bDebugDrawDetection)
    {
        DrawAttackDebug(NewEnemies, NewAllies);
    }

    TSet<AActor*> PreviousEnemies;
    PreviousEnemies.Reserve(ActorsInRange.Num());
    
    for (AActor* PrevEnemy : ActorsInRange)
    {
        if (PrevEnemy)
            PreviousEnemies.Add(PrevEnemy);
    }

    TSet<AActor*> CurrentEnemies;
    CurrentEnemies.Reserve(NewEnemies.Num());
    
    for (AActor* Enemy : NewEnemies)
    {
        if (!Enemy)
            continue;

        CurrentEnemies.Add(Enemy);

        if (!PreviousEnemies.Contains(Enemy))
            HandleAutoEngage(Enemy);
    }

    for (AActor* PrevEnemy : ActorsInRange)
    {
        if (PrevEnemy && !CurrentEnemies.Contains(PrevEnemy))
            HandleTargetRemoval(PrevEnemy);
    }

    ActorsInRange = MoveTemp(NewEnemies);
    AllyInRange = MoveTemp(NewAllies);
}

void ASoldierRts::DrawAttackDebug(const TArray<AActor*>& DetectedEnemies, const TArray<AActor*>& DetectedAllies) const
{
    if (!GetWorld())
        return;

    const FColor DebugColor = DetectionSettings.DebugColor.ToFColor(true);
    DrawDebugSphere(GetWorld(), GetActorLocation(), AttackRange, 16, DebugColor, false, DetectionSettings.DebugDuration);

    if (AllyDetectionRange > KINDA_SMALL_NUMBER)
    {
        DrawDebugSphere(GetWorld(), GetActorLocation(), AllyDetectionRange, 16, FColor::Green, false, DetectionSettings.DebugDuration);
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
    CommandData.Type = CommandAttack;
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

    CommandComp->CommandMoveToLocation(CommandData);
}

void ASoldierRts::HandleAutoEngage(AActor* Target)
{
    if (!ShouldAutoEngage() || !IsEnemyActor(Target) || !AIController)
        return;

    if (!AIController->HasAttackTarget())
    {
        IssueAttackOrder(MakeAttackCommand(Target));
    }
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

    if (ActorsInRange.IsEmpty() && bControllerAggressive)
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
    return bHasWeapon;
}

ETeams ASoldierRts::GetCurrentTeam_Implementation()
{
    return CurrentTeam;
}
