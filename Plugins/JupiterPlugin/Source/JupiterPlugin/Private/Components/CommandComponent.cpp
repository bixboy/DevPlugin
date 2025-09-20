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

                TargetOrientation = OwnerActor->GetActorRotation();
                CreateMoveMarker();
        }

        InitializeMovementComponent();
}

void UCommandComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);
    
	if (MoveMarker) MoveMarker->Destroy();
}

void UCommandComponent::InitializeMovementComponent() const
{
	if (!OwnerCharaMovementComp) return;

	OwnerCharaMovementComp->bOrientRotationToMovement = true;
	OwnerCharaMovementComp->RotationRate = FRotator(0.f, 640.f, 0.f);
	OwnerCharaMovementComp->bConstrainToPlane = true;
	OwnerCharaMovementComp->bSnapToPlaneAtStart = true;
}

#pragma endregion

void UCommandComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
        Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

        if (!OwnerActor)
        {
                return;
        }

        UpdateTrackedTarget();

        if (OwnerActor->HasAuthority() && ShouldOrientate)
        {
                SetOrientation(DeltaTime);
                if (IsOrientated())
                {
                        ShouldOrientate = false;
		}
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
        FCommandData WorkingCommand = CommandData;

        ApplyMovementSettings(WorkingCommand);
        PrepareCommand(WorkingCommand);

        if (WorkingCommand.Type == ECommandType::CommandPatrol)
        {
                BeginPatrol(WorkingCommand);
                return;
        }

        BeginMove(WorkingCommand);
}

void UCommandComponent::CommandPatrol(const FCommandData CommandData)
{
        if (OwnerAIController)
        {
                OwnerAIController->OnReachedDestination.Clear();
                OwnerAIController->CommandPatrol(CommandData);
        }
}

void UCommandComponent::CommandMove(const FCommandData CommandData)
{
        if (!OwnerAIController) return;

        OwnerAIController->OnReachedDestination.Clear();

        if (!OwnerAIController->OnReachedDestination.IsBound())
                OwnerAIController->OnReachedDestination.AddDynamic(this, &UCommandComponent::DestinationReached);

        OwnerAIController->CommandMove(CommandData, bTrackCommandTarget);
        UpdateMoveMarkerAttachment(CommandData);
}

void UCommandComponent::PrepareCommand(FCommandData& CommandData)
{
        if (!HasValidAttackTarget(CommandData))
        {
                CommandData.Target = nullptr;
        }

        bTrackCommandTarget = ShouldFollowCommandTarget(CommandData);
        bAttachMarkerToCommandTarget = bTrackCommandTarget;

        TargetLocation = ResolveDestinationFromCommand(CommandData);
        CommandData.Location = TargetLocation;
        CurrentCommand = CommandData;

        if (!bTrackCommandTarget)
        {
                ResetMoveMarkerAttachment();
        }

        TargetOrientation = CommandData.Rotation;
        ShouldOrientate = false;
}

void UCommandComponent::BeginPatrol(const FCommandData& CommandData)
{
        bTrackCommandTarget = false;
        bAttachMarkerToCommandTarget = false;
        ResetMoveMarkerAttachment();
        ShowMoveMarker(false);

        CommandPatrol(CommandData);
}

void UCommandComponent::BeginMove(const FCommandData& CommandData)
{
        CommandMove(CommandData);
}

void UCommandComponent::UpdateTrackedTarget()
{
        if (!bTrackCommandTarget)
        {
                return;
        }

        if (!CurrentCommand.Target || !IsValid(CurrentCommand.Target))
        {
                bTrackCommandTarget = false;
                bAttachMarkerToCommandTarget = false;
                CurrentCommand.Target = nullptr;
                ResetMoveMarkerAttachment();
                return;
        }

        const FVector NewLocation = CurrentCommand.Target->GetActorLocation();
        if (!NewLocation.Equals(TargetLocation, TargetUpdateTolerance))
        {
                TargetLocation = NewLocation;
                CurrentCommand.Location = TargetLocation;
                UpdateMoveMarkerAttachment(CurrentCommand);
        }
}

void UCommandComponent::UpdateMoveMarkerAttachment(const FCommandData& CommandData)
{
        if (OwnerActor && OwnerActor->HasAuthority())
        {
                SetMoveMarker(TargetLocation, CommandData);
        }
}

void UCommandComponent::ResetMoveMarkerAttachment()
{
        if (MoveMarker && MoveMarker->GetAttachParentActor())
        {
                MoveMarker->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        }
}

void UCommandComponent::DestinationReached(const FCommandData CommandData)
{
        ResetMoveMarkerAttachment();
        ShowMoveMarker(false);
        TargetOrientation = CommandData.Rotation;

        if (CommandData.Target && IsValid(CommandData.Target))
        {
                TargetOrientation = UKismetMathLibrary::FindLookAtRotation(OwnerActor->GetActorLocation(), CommandData.Target->GetActorLocation());
        }

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
        {
                return CommandData.Target->GetActorLocation();
        }

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
	{
		OwnerCharaMovementComp->MaxWalkSpeed = MaxSpeed * 0.5f;
	}
}

void UCommandComponent::SetRun() const
{
	if (OwnerCharaMovementComp)
	{
		OwnerCharaMovementComp->MaxWalkSpeed = MaxSpeed;
	}
}

void UCommandComponent::SetSprint() const
{
	if (OwnerCharaMovementComp)
	{
		OwnerCharaMovementComp->MaxWalkSpeed = MaxSpeed * 1.25f;
	}
}

#pragma endregion

// Orientation
#pragma region Orientation

void UCommandComponent::SetOrientation(float DeltaTime)
{
	if (!OwnerActor) return;

        FRotator InterpolatedRotation = UKismetMathLibrary::RInterpTo(
                FRotator(OwnerActor->GetActorRotation().Pitch, OwnerActor->GetActorRotation().Yaw, 0.f),
                TargetOrientation,
                DeltaTime,
                OrientationInterpSpeed
        );

	OwnerActor->SetActorRotation(InterpolatedRotation);
}

bool UCommandComponent::IsOrientated() const
{
	if (!OwnerActor) return false;

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
                return;
        }

        if (CommandData.RequestingController && CommandData.RequestingController->IsLocalController())
                ShowMoveMarker(true);

        MoveMarker->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        MoveMarker->SetActorLocation(Location);

        if (bAttachMarkerToCommandTarget && CommandData.Target && IsValid(CommandData.Target))
        {
                MoveMarker->AttachToActor(CommandData.Target, FAttachmentTransformRules::KeepWorldTransform);
        }
}

void UCommandComponent::ShowMoveMarker_Implementation(bool bIsSelected)
{
        if (!MoveMarker)
        {
                return;
        }

        if (bIsSelected)
        {
                if (bAttachMarkerToCommandTarget && CurrentCommand.Target && IsValid(CurrentCommand.Target))
                {
                        MoveMarker->AttachToActor(CurrentCommand.Target, FAttachmentTransformRules::KeepWorldTransform);
                }

                MoveMarker->SetActorHiddenInGame(false);
                MoveMarker->SetActorLocation(TargetLocation);
        }
        else
        {
                ResetMoveMarkerAttachment();
                MoveMarker->SetActorHiddenInGame(true);
        }
}

// Spawn Move Marker
void UCommandComponent::CreateMoveMarker()
{
        if (!MoveMarker && MoveMarkerClass && OwnerActor)
        {
                FActorSpawnParameters SpawnParams;
                SpawnParams.Instigator = OwnerActor;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        
		MoveMarker = GetWorld()->SpawnActor<AActor>(MoveMarkerClass, GetPositionTransform(OwnerActor->GetActorLocation()), SpawnParams);
		if (MoveMarker)
		{
			ShowMoveMarker(false);
		}
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
