#include "Player/CameraCommandSystem.h"

#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/UnitFormationComponent.h"
#include "Components/UnitOrderComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "Data/AiData.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Interfaces/Selectable.h"
#include "Player/CameraPatrolSystem.h"
#include "Player/CameraPreviewSystem.h"
#include "Player/CameraSpawnSystem.h"
#include "Player/PlayerCamera.h"
#include "Utils/JupiterInputHelpers.h"
#include "Utils/JupiterTraceUtils.h"
#include "Utils/PreviewFormationUtils.h"

void UCameraCommandSystem::Init(APlayerCamera* InOwner)
{
    Owner = InOwner;
    if (!Owner.IsValid())
    {
        return;
    }

    Player = Owner->GetPlayerController();
    SelectionComponent = Owner->GetSelectionComponent();
    OrderComponent = Owner->GetOrderComponent();
    FormationComponent = Owner->GetFormationComponent();

    CommandRotationPreview.Reset(FRotator::ZeroRotator);

    if (FormationComponent && OrderComponent)
    {
        FormationComponent->OnFormationStateChanged.RemoveDynamic(OrderComponent, &UUnitOrderComponent::ReapplyCachedFormation);
        FormationComponent->OnFormationStateChanged.AddDynamic(OrderComponent, &UUnitOrderComponent::ReapplyCachedFormation);
    }
}

void UCameraCommandSystem::Tick(float DeltaTime)
{
    if (!bCommandActionHeld)
    {
        return;
    }

    const float CurrentTime = Owner.IsValid() && Owner->GetWorld() ? Owner->GetWorld()->GetTimeSeconds() : 0.f;
    UpdateCommandPreview(CurrentTime);
}

void UCameraCommandSystem::NotifySelectionPressed(const FVector& GroundLocation)
{
    CommandLocation = GroundLocation;
}

void UCameraCommandSystem::HandleCommandActionStarted()
{
    if (!Player.IsValid())
    {
        return;
    }

    const bool bAltDown = JupiterInputHelpers::IsAltDown(Player.Get());
    if (PatrolSystem.IsValid() && PatrolSystem->HandleRightClickPressed(bAltDown))
    {
        bCommandActionHeld = false;
        return;
    }

    if (SpawnSystem.IsValid() && SpawnSystem->IsInSpawnMode())
    {
        bCommandActionHeld = false;
        return;
    }

    bCommandActionHeld = true;
    bCommandPreviewVisible = false;
    CommandRotationPreview.Deactivate();

    StartCommandPreview();
}

void UCameraCommandSystem::HandleCommandActionCompleted()
{
    if (!Player.IsValid())
    {
        bCommandActionHeld = false;
        StopCommandPreview();
        return;
    }

    const bool bAltDown = JupiterInputHelpers::IsAltDown(Player.Get());
    if (PatrolSystem.IsValid() && PatrolSystem->HandleRightClickReleased(bAltDown))
    {
        bCommandActionHeld = false;
        StopCommandPreview();
        return;
    }

    if (SpawnSystem.IsValid() && SpawnSystem->IsInSpawnMode())
    {
        bCommandActionHeld = false;
        StopCommandPreview();
        return;
    }

    if (!SelectionComponent)
    {
        bCommandActionHeld = false;
        StopCommandPreview();
        return;
    }

    const FHitResult Hit = SelectionComponent->GetMousePositionOnTerrain();
    const bool bHasValidGround = Hit.bBlockingHit;

    AActor* TargetActor = GetActorUnderCursor();
    if (TargetActor && TargetActor->Implements<USelectable>())
    {
        if (OrderComponent)
        {
            OrderComponent->IssueOrder(BuildCommandData(ECommandType::CommandAttack, TargetActor));
        }

        bCommandActionHeld = false;
        StopCommandPreview();
        return;
    }

    if (!bHasValidGround && !CommandRotationPreview.bPreviewActive)
    {
        bCommandActionHeld = false;
        StopCommandPreview();
        return;
    }

    const FVector TargetLocation = CommandRotationPreview.bPreviewActive && !CommandRotationPreview.Center.IsZero() ? CommandRotationPreview.Center : Hit.Location;
    const FRotator DefaultRotation = Owner.IsValid() && Owner->GetCameraComponent() ? FRotator(0.f, Owner->GetCameraComponent()->GetComponentRotation().Yaw, 0.f) : FRotator::ZeroRotator;
    const FRotator FinalRotation = CommandRotationPreview.bPreviewActive ? CommandRotationPreview.CurrentRotation : DefaultRotation;

    if (OrderComponent)
    {
        FCommandData Command(Player.Get(), TargetLocation, FinalRotation, ECommandType::CommandMove);
        Command.SourceLocation = TargetLocation;
        Command.Location = TargetLocation;
        Command.Rotation = FinalRotation;
        OrderComponent->IssueOrder(Command);
    }

    bCommandActionHeld = false;
    StopCommandPreview();
}

void UCameraCommandSystem::HandleDestroySelected()
{
    if (!SelectionComponent || !Owner.IsValid())
    {
        return;
    }

    const TArray<AActor*> SelectedActors = SelectionComponent->GetSelectedActors();
    if (SelectedActors.IsEmpty())
    {
        return;
    }

    Owner->Server_DestroyActor(SelectedActors);
}

void UCameraCommandSystem::HandleServerDestroyActor(const TArray<AActor*>& ActorsToDestroy)
{
    for (AActor* Actor : ActorsToDestroy)
    {
        if (Actor)
        {
            Actor->Destroy();
        }
    }
}

void UCameraCommandSystem::IssuePatrolCommand(float Radius)
{
    if (!OrderComponent)
    {
        return;
    }

    OrderComponent->IssueOrder(BuildCommandData(ECommandType::CommandPatrol, nullptr, Radius));
}

void UCameraCommandSystem::StartCommandPreview()
{
    if (!SelectionComponent || !PreviewSystem.IsValid())
    {
        return;
    }

    const FRotator BaseRotation = Owner.IsValid() && Owner->GetCameraComponent() ? FRotator(0.f, Owner->GetCameraComponent()->GetComponentRotation().Yaw, 0.f) : FRotator::ZeroRotator;
    if (CommandLocation.IsNearlyZero() && SelectionComponent)
    {
        CommandLocation = SelectionComponent->GetMousePositionOnTerrain().Location;
    }

    CommandRotationPreview.Reset(BaseRotation);
    CommandRotationPreview.Center = CommandLocation;
    CommandRotationPreview.BaseRotation = BaseRotation;
    CommandRotationPreview.CurrentRotation = BaseRotation;

    EnsurePreviewMeshForSelection();
}

void UCameraCommandSystem::StopCommandPreview()
{
    if (PreviewSystem.IsValid() && bCommandPreviewVisible)
    {
        PreviewSystem->HidePreview();
    }

    CommandRotationPreview.Deactivate();
    bCommandPreviewVisible = false;
}

void UCameraCommandSystem::UpdateCommandPreview(float CurrentTime)
{
    if (!SelectionComponent || !PreviewSystem.IsValid())
    {
        StopCommandPreview();
        bCommandActionHeld = false;
        return;
    }

    const FHitResult MouseHit = SelectionComponent->GetMousePositionOnTerrain();
    const FVector MouseLocation = MouseHit.Location;

    if (!CommandRotationPreview.bPreviewActive)
    {
        if (!TryActivateCommandPreview(CurrentTime, MouseLocation))
        {
            return;
        }
    }

    CommandRotationPreview.UpdateRotation(MouseLocation);
    BuildCommandPreviewTransforms(CommandRotationPreview.Center, CommandRotationPreview.CurrentRotation);
}

bool UCameraCommandSystem::TryActivateCommandPreview(float CurrentTime, const FVector& MouseLocation)
{
    if (!CommandRotationPreview.TryActivate(CurrentTime, RotationPreviewHoldTime, CommandRotationPreview.Center.IsZero() ? MouseLocation : CommandRotationPreview.Center, MouseLocation))
    {
        return false;
    }

    if (!EnsurePreviewMeshForSelection())
    {
        CommandRotationPreview.Deactivate();
        return false;
    }

    bCommandPreviewVisible = true;
    return true;
}

bool UCameraCommandSystem::EnsurePreviewMeshForSelection()
{
    if (!PreviewSystem.IsValid() || !SelectionComponent)
    {
        return false;
    }

    const TArray<AActor*> SelectedUnits = SelectionComponent->GetSelectedActors();
    if (SelectedUnits.IsEmpty())
    {
        PreviewSystem->HidePreview();
        bCommandPreviewVisible = false;
        return false;
    }

    USkeletalMesh* SkeletalMesh = nullptr;
    FVector SkeletalScale = FVector::OneVector;
    UStaticMesh* StaticMesh = nullptr;
    FVector StaticScale = FVector::OneVector;

    for (AActor* Unit : SelectedUnits)
    {
        if (!Unit)
        {
            continue;
        }

        if (!SkeletalMesh)
        {
            if (const USkeletalMeshComponent* SkeletalComponent = Unit->FindComponentByClass<USkeletalMeshComponent>())
            {
                if (USkeletalMesh* Mesh = SkeletalComponent->GetSkeletalMeshAsset())
                {
                    SkeletalMesh = Mesh;
                    SkeletalScale = SkeletalComponent->GetRelativeScale3D();
                }
            }
        }

        if (!StaticMesh)
        {
            if (const UStaticMeshComponent* StaticComponent = Unit->FindComponentByClass<UStaticMeshComponent>())
            {
                if (UStaticMesh* Mesh = StaticComponent->GetStaticMesh())
                {
                    StaticMesh = Mesh;
                    StaticScale = StaticComponent->GetRelativeScale3D();
                }
            }
        }

        if (SkeletalMesh || StaticMesh)
        {
            break;
        }
    }

    const int32 InstanceCount = SelectedUnits.Num();

    if (SkeletalMesh && PreviewSystem->ShowSkeletalPreview(SkeletalMesh, SkeletalScale, InstanceCount))
    {
        bCommandPreviewVisible = true;
        return true;
    }

    if (StaticMesh && PreviewSystem->ShowStaticPreview(StaticMesh, StaticScale, InstanceCount))
    {
        bCommandPreviewVisible = true;
        return true;
    }

    PreviewSystem->HidePreview();
    bCommandPreviewVisible = false;
    return false;
}

void UCameraCommandSystem::BuildCommandPreviewTransforms(const FVector& CenterLocation, const FRotator& FacingRotation)
{
    if (!PreviewSystem.IsValid())
    {
        return;
    }

    if (!SelectionComponent)
    {
        return;
    }

    const TArray<AActor*> SelectedUnits = SelectionComponent->GetSelectedActors();
    if (SelectedUnits.IsEmpty())
    {
        return;
    }

    FCommandData BaseCommand(Player.Get(), CenterLocation, FacingRotation, ECommandType::CommandMove);
    BaseCommand.SourceLocation = CenterLocation;
    BaseCommand.Location = CenterLocation;
    BaseCommand.Rotation = FacingRotation;

    TArray<FTransform> InstanceTransforms;
    PreviewFormationUtils::BuildCommandPreviewTransforms(FormationComponent, BaseCommand, SelectedUnits, InstanceTransforms);
    if (InstanceTransforms.IsEmpty())
    {
        return;
    }

    PreviewSystem->UpdateInstances(InstanceTransforms);
}

FCommandData UCameraCommandSystem::BuildCommandData(ECommandType Type, AActor* TargetActor, float Radius) const
{
    if (!Player.IsValid())
    {
        return FCommandData();
    }

    if (Type == ECommandType::CommandAttack)
    {
        return FCommandData(Player.Get(), CommandLocation, Owner.IsValid() ? Owner->GetCameraComponent()->GetComponentRotation() : FRotator::ZeroRotator, Type, TargetActor);
    }

    if (Type == ECommandType::CommandPatrol)
    {
        FCommandData Command(Player.Get(), CommandLocation, Owner.IsValid() ? Owner->GetCameraComponent()->GetComponentRotation() : FRotator::ZeroRotator, Type, nullptr, Radius * 2.f);
        return Command;
    }

    return FCommandData(Player.Get(), CommandLocation, Owner.IsValid() ? Owner->GetCameraComponent()->GetComponentRotation() : FRotator::ZeroRotator, Type);
}

AActor* UCameraCommandSystem::GetActorUnderCursor() const
{
    if (!Player.IsValid())
    {
        return nullptr;
    }

    FHitResult Hit;
    if (JupiterTraceUtils::TraceUnderCursor(Player.Get(), Hit, ECC_Visibility, { Owner.Get() }))
    {
        return Hit.GetActor();
    }

    return nullptr;
}

