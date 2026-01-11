#include "Components/Combat/CommandComponent.h"
#include "AI/AiControllerRts.h"
#include "Units/SoldierRts.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

// -------------------------------------------------------------------------
// SETUP & LIFECYCLE
// -------------------------------------------------------------------------

UCommandComponent::UCommandComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

void UCommandComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerActor = Cast<ASoldierRts>(GetOwner());
    if (OwnerActor)
    {
        CreateMoveMarker();
        OwnerCharaMovementComp = OwnerActor->GetCharacterMovement();
        
        if (OwnerActor->OnSelected.IsBound())
        {
             OwnerActor->OnSelected.AddDynamic(this, &UCommandComponent::Client_ShowMoveMarker);
        }
    }

    InitializeMovementComponent();
}

void UCommandComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    Super::OnComponentDestroyed(bDestroyingHierarchy);
    if (MoveMarker)
    {
        MoveMarker->Destroy();
    }
}

void UCommandComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UCommandComponent, CurrentCommand);
}

void UCommandComponent::InitializeMovementComponent() const
{
    if (!OwnerCharaMovementComp) return;

    OwnerCharaMovementComp->bOrientRotationToMovement = true;
    OwnerCharaMovementComp->RotationRate = FRotator(0.f, 600.f, 0.f);
    OwnerCharaMovementComp->bConstrainToPlane = true;
    OwnerCharaMovementComp->bSnapToPlaneAtStart = true;
}

void UCommandComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (OwnerActor && OwnerActor->HasAuthority() && bShouldOrientate)
    {
        UpdateOrientation(DeltaTime);
        
        if (IsOriented())
        {
            bShouldOrientate = false;
        }
    }
}

// -------------------------------------------------------------------------
// COMMAND LOGIC
// -------------------------------------------------------------------------

void UCommandComponent::SetOwnerAIController(AAiControllerRts* Controller)
{
    OwnerAIController = Controller;
}

void UCommandComponent::CommandMoveToLocation(const FCommandData& CommandData)
{
    CurrentCommand = CommandData;
    ApplyMovementSettings(CommandData);

    bShouldOrientate = false; 

    if (CommandData.Type == ECommandType::CommandPatrol)
    {
        bHaveTargetAttack = false;
        TargetLocation = ResolveDestinationFromCommand(CommandData);
        CurrentCommand.Location = TargetLocation; 
        
        CommandPatrol(CurrentCommand);
        return;
    }

    bHaveTargetAttack = ShouldFollowCommandTarget(CommandData);
    TargetLocation = ResolveDestinationFromCommand(CommandData);
    CurrentCommand.Location = TargetLocation;

    CommandMove(CurrentCommand);
}

void UCommandComponent::CommandPatrol(const FCommandData& CommandData)
{
    if (!OwnerAIController)
    	return;

    OwnerAIController->OnReachedDestination.RemoveDynamic(this, &UCommandComponent::OnDestinationReached);
    OwnerAIController->OnReachedDestination.AddDynamic(this, &UCommandComponent::OnDestinationReached);

    OwnerAIController->CommandPatrol(CommandData);
    
    Client_UpdateMoveMarker(TargetLocation, CommandData);
}

void UCommandComponent::CommandMove(const FCommandData& CommandData)
{
    if (!OwnerAIController)
    	return;

    OwnerAIController->OnReachedDestination.RemoveDynamic(this, &UCommandComponent::OnDestinationReached);
    OwnerAIController->OnReachedDestination.AddDynamic(this, &UCommandComponent::OnDestinationReached);

    OwnerAIController->CommandMove(CommandData, bHaveTargetAttack);
    
    Client_UpdateMoveMarker(TargetLocation, CommandData);

    bHaveTargetAttack = false;
}

void UCommandComponent::OnDestinationReached(const FCommandData CommandData)
{
    Client_ShowMoveMarker(false);
    TargetOrientation = CommandData.Rotation;

    if (CommandData.Target && IsValid(CommandData.Target))
    {
        TargetOrientation = UKismetMathLibrary::FindLookAtRotation(OwnerActor->GetActorLocation(), CommandData.Target->GetActorLocation());
        TargetOrientation.Pitch = 0.f;
    }

    bShouldOrientate = true;
}


// -------------------------------------------------------------------------
// HELPERS
// -------------------------------------------------------------------------

bool UCommandComponent::ShouldFollowCommandTarget(const FCommandData& CommandData) const
{
    return CommandData.Type == CommandAttack && HasValidAttackTarget(CommandData);
}

bool UCommandComponent::HasValidAttackTarget(const FCommandData& CommandData) const
{
    return CommandData.Target && IsValid(CommandData.Target) && CommandData.Target != OwnerActor;
}

FVector UCommandComponent::ResolveDestinationFromCommand(const FCommandData& CommandData) const
{
    if (ShouldFollowCommandTarget(CommandData))
        return CommandData.Target->GetActorLocation();

    if (CommandData.Type == ECommandType::CommandPatrol && CommandData.PatrolPath.Num() > 0)
        return CommandData.PatrolPath[0];

    return CommandData.Location;
}

void UCommandComponent::ApplyMovementSettings(const FCommandData& CommandData)
{
    switch (CommandData.Type)
    {
    case ECommandType::CommandMoveSlow:
    	SetWalk();
    	break;
    	
    case ECommandType::CommandMoveFast:
    	SetSprint();
    	break;
    	
    case ECommandType::CommandAttack:
    	SetSprint();
    	break;
    	
    case ECommandType::CommandPatrol:
    	SetWalk();
    	break;
    	
    default:
    	SetRun();
    	break;
    }
}

void UCommandComponent::SetWalk() const
{
    if (OwnerCharaMovementComp)
    	OwnerCharaMovementComp->MaxWalkSpeed = MaxSpeed * 0.5f;
}

void UCommandComponent::SetRun() const
{
    if (OwnerCharaMovementComp)
    	OwnerCharaMovementComp->MaxWalkSpeed = MaxSpeed;
}

void UCommandComponent::SetSprint() const
{
    if (OwnerCharaMovementComp)
    	OwnerCharaMovementComp->MaxWalkSpeed = MaxSpeed * 1.5f;
}

// -------------------------------------------------------------------------
// ORIENTATION LOGIC
// -------------------------------------------------------------------------

void UCommandComponent::UpdateOrientation(float DeltaTime)
{
    if (!OwnerActor)
    	return;

    FRotator CurrentRot = OwnerActor->GetActorRotation();
    FRotator NewRot = UKismetMathLibrary::RInterpTo(CurrentRot, TargetOrientation, DeltaTime, 5.0f);

    OwnerActor->SetActorRotation(NewRot);
}

bool UCommandComponent::IsOriented() const
{
    if (!OwnerActor) return true;
    return FMath::IsNearlyEqual(OwnerActor->GetActorRotation().Yaw, TargetOrientation.Yaw, 1.0f);
}

// -------------------------------------------------------------------------
// VISUALS & NETWORK
// -------------------------------------------------------------------------

void UCommandComponent::CreateMoveMarker()
{
    if (!MoveMarker && MoveMarkerClass && GetWorld())
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = GetOwner();
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        
        MoveMarker = GetWorld()->SpawnActor<AActor>(MoveMarkerClass, FTransform::Identity, SpawnParams);
        
        if (MoveMarker)
        {
            MoveMarker->SetActorHiddenInGame(true);
        }
    }
}

void UCommandComponent::Client_UpdateMoveMarker_Implementation(const FVector& Location, const FCommandData& CommandData)
{
    if (!MoveMarker)
    	CreateMoveMarker();
    
    if (MoveMarker)
    {
        MoveMarker->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

        MoveMarker->SetActorTransform(GetMarkerTransformOnGround(Location));
        MoveMarker->SetActorHiddenInGame(false);

        if (bHaveTargetAttack && CommandData.Target && IsValid(CommandData.Target))
        {
             MoveMarker->AttachToActor(CommandData.Target, FAttachmentTransformRules::KeepWorldTransform);
        }
    }
}

void UCommandComponent::Client_ShowMoveMarker_Implementation(bool bShow)
{
    if (MoveMarker)
    {
        MoveMarker->SetActorHiddenInGame(!bShow);
    }
}

FTransform UCommandComponent::GetMarkerTransformOnGround(const FVector& Position) const
{
    FHitResult Hit;
    FCollisionQueryParams CollisionParams;
    CollisionParams.AddIgnoredActor(GetOwner());

    FVector TraceOrigin = Position + FVector(0.f, 0.f, 500.f);
    FVector TraceEnd = Position + FVector(0.f, 0.f, -1000.f);

    if (GetWorld()->LineTraceSingleByChannel(Hit, TraceOrigin, TraceEnd, ECC_Terrain, CollisionParams))
    {
        if (Hit.bBlockingHit)
        {
            FTransform HitTransform;
            HitTransform.SetLocation(Hit.ImpactPoint + FVector(0.f, 0.f, 2.0f));
            
            FRotator TerrainRotation = UKismetMathLibrary::MakeRotFromZX(Hit.Normal, FVector::ForwardVector);
            HitTransform.SetRotation(TerrainRotation.Quaternion());
            
            return HitTransform;
        }
    }

    return FTransform(FRotator::ZeroRotator, Position);
}