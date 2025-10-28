#include "Components/CommandComponent.h"
#include "Units/AI/AiControllerRts.h"
#include "Units/SoldierRts.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

// Setup
#pragma region Setup

UCommandComponent::UCommandComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCommandComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerActor = Cast<ASoldierRts>(GetOwner());
	if (OwnerActor)
	{
		OwnerActor->OnSelected.AddDynamic(this, &UCommandComponent::ShowMoveMarker);
		OwnerCharaMovementComp = OwnerActor->GetCharacterMovement();
		
		CreatMoveMarker();
	}

	InitializeMovementComponent();
}

void UCommandComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);
    
	if (MoveMarker)
		MoveMarker->Destroy();
}

void UCommandComponent::InitializeMovementComponent() const
{
	if (!OwnerCharaMovementComp)
		return;

	OwnerCharaMovementComp->bOrientRotationToMovement = true;
	OwnerCharaMovementComp->RotationRate = FRotator(0.f, 640.f, 0.f);
	OwnerCharaMovementComp->bConstrainToPlane = true;
	OwnerCharaMovementComp->bSnapToPlaneAtStart = true;
}

#pragma endregion

void UCommandComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (OwnerActor->HasAuthority() && ShouldOrientate)
	{
		SetOrientation(DeltaTime);
		
		if (IsOrientated())
			ShouldOrientate = false;
	}
}

// Create & Start Commands
#pragma region Create & Start Commands

void UCommandComponent::SetOwnerAIController(AAiControllerRts* Controller)
{
	OwnerAIController = Controller;
}

FVector UCommandComponent::GetCommandLocation() const
{
	return TargetLocation;
}

FCommandData UCommandComponent::GetCurrentCommand() const
{
	return CurrentCommand;
}

void UCommandComponent::CommandMoveToLocation(const FCommandData CommandData)
{
    CurrentCommand = CommandData;

    ApplyMovementSettings(CommandData);

    if (CommandData.Type == ECommandType::CommandPatrol)
    {
        HaveTargetAttack = false;
        TargetLocation = ResolveDestinationFromCommand(CommandData);
        CurrentCommand.Location = TargetLocation;
        CommandPatrol(CurrentCommand);

        return;
    }

    HaveTargetAttack = ShouldFollowCommandTarget(CommandData);
    TargetLocation = ResolveDestinationFromCommand(CommandData);
    CurrentCommand.Location = TargetLocation;

    CommandMove(CurrentCommand);
}

void UCommandComponent::CommandPatrol(const FCommandData CommandData)
{
    if (!OwnerAIController)
        return;

    OwnerAIController->OnReachedDestination.Clear();
    if (!OwnerAIController->OnReachedDestination.IsBound())
            OwnerAIController->OnReachedDestination.AddDynamic(this, &UCommandComponent::DestinationReached);

    OwnerAIController->CommandPatrol(CommandData);
    SetMoveMarker(TargetLocation, CommandData);
}

void UCommandComponent::CommandMove(const FCommandData CommandData)
{
    if (!OwnerAIController)
    	return;

    OwnerAIController->OnReachedDestination.Clear();
    if (!OwnerAIController->OnReachedDestination.IsBound())
            OwnerAIController->OnReachedDestination.AddDynamic(this, &UCommandComponent::DestinationReached);

    OwnerAIController->CommandMove(CommandData, HaveTargetAttack);
    SetMoveMarker(TargetLocation, CommandData);

    HaveTargetAttack = false;
}

void UCommandComponent::DestinationReached(const FCommandData CommandData)
{
    ShowMoveMarker(false);
    TargetOrientation = CommandData.Rotation;

    if (CommandData.Target && IsValid(CommandData.Target))
		TargetOrientation = UKismetMathLibrary::FindLookAtRotation(OwnerActor->GetActorLocation(), CommandData.Target->GetActorLocation());

    ShouldOrientate = true;
}

#pragma endregion

bool UCommandComponent::ShouldFollowCommandTarget(const FCommandData& CommandData) const
{
    return CommandData.Type == ECommandType::CommandAttack && HasValidAttackTarget(CommandData);
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

 
// Walk Speed
#pragma region Walk Speed

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
		OwnerCharaMovementComp->MaxWalkSpeed = MaxSpeed * 1.25f;
}

#pragma endregion

// Orientation
#pragma region Orientation

void UCommandComponent::SetOrientation(float DeltaTime)
{
	if (!OwnerActor)
		return;

	FRotator InterpolatedRotation = UKismetMathLibrary::RInterpTo(
		FRotator(OwnerActor->GetActorRotation().Pitch, OwnerActor->GetActorRotation().Yaw, 0.f),
		TargetOrientation,
		DeltaTime,
		2.f
	);

	OwnerActor->SetActorRotation(InterpolatedRotation);
}

bool UCommandComponent::IsOrientated() const
{
	if (!OwnerActor)
		return false;

	const FRotator CurrentRotation = OwnerActor->GetActorRotation();
	return FMath::IsNearlyEqual(CurrentRotation.Yaw, TargetOrientation.Yaw, 0.25f);
}

#pragma endregion

// Marker
#pragma region Marker

void UCommandComponent::SetMoveMarker_Implementation(const FVector Location, const FCommandData CommandData)
{
	if (!MoveMarker)
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveMarker is null"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("MoveMarker is Valid"));

	if (CommandData.RequestingController && CommandData.RequestingController->IsLocalController())
		ShowMoveMarker(true);
	
	MoveMarker->SetActorLocation(Location);
    
    if (HaveTargetAttack && CommandData.Target && IsValid(CommandData.Target))
		MoveMarker->AttachToActor(CommandData.Target, FAttachmentTransformRules::KeepWorldTransform);
}

void UCommandComponent::ShowMoveMarker_Implementation(bool bIsSelected)
{
	if (MoveMarker)
		MoveMarker->SetActorHiddenInGame(!bIsSelected);
}

// Spawn Move Marker
void UCommandComponent::CreatMoveMarker()
{
	if (!MoveMarker && MoveMarkerClass && OwnerActor)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Instigator = OwnerActor;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        
		MoveMarker = GetWorld()->SpawnActor<AActor>(MoveMarkerClass, GetPositionTransform(OwnerActor->GetActorLocation()), SpawnParams);
		if (MoveMarker)
			ShowMoveMarker(false);
	}
	
}

// Get position For Marker
FTransform UCommandComponent::GetPositionTransform(const FVector Position) const
{
	FHitResult Hit;
	FCollisionQueryParams CollisionParams;
	FVector TraceOrigin = Position + FVector(0.f, 0.f, 10000.f);
	FVector TraceEnd = Position + FVector(0.f, 0.f, -10000.f);

	if (UWorld* World = GetWorld())
	{
		if (World->LineTraceSingleByChannel(Hit, TraceOrigin, TraceEnd, ECollisionChannel::ECC_GameTraceChannel1, CollisionParams))
		{
			if (Hit.bBlockingHit)
			{
				FTransform HitTransform;
				HitTransform.SetLocation(Hit.ImpactPoint + FVector(1.f, 1.f, 1.25f));
				FRotator TerrainRotation = UKismetMathLibrary::MakeRotFromZX(Hit.Normal, FVector::UpVector);
				TerrainRotation += FRotator(90.f, 0.f, 0.f);
				HitTransform.SetRotation(TerrainRotation.Quaternion());
				
				return HitTransform;
			}
		}
	}

	return FTransform::Identity;
}

#pragma endregion
