#include "JupiterPlugin/Public/Player/PlayerCamera.h"

#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/UnitFormationComponent.h"
#include "Components/UnitOrderComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "Components/UnitPatrolComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/Selectable.h"
#include "Player/Selections/SphereRadius.h"
#include "Utilities/PreviewPoseMesh.h"


void APlayerCamera::Input_PatrolZone(const FInputActionValue& ActionValue)
{
    const float Value = ActionValue.Get<float>();

    if (!bAltIsPressed)
    {
        if (SphereRadius)
			SphereRadius->End();
    	
        SphereRadiusEnable = false;
        return;
    }

    if (!Player || Value == 0.f)
    {
        if (SphereRadius)
			SphereRadius->End();
    
        return;
    }

    if (Player->GetInputKeyTimeDown(EKeys::RightMouseButton) >= LeftMouseHoldThreshold && SphereRadius)
    {
        if (!SphereRadiusEnable && SphereRadius)
        {
            SphereRadius->Start(LeftMouseHitLocation, TargetRotation);
            SphereRadiusEnable = true;
        }
    }
}

void APlayerCamera::Input_OnDestroySelected()
{
    if (!Player)
		return;

    if (UUnitSelectionComponent* Selection = GetSelectionComponentChecked())
    {
        const TArray<AActor*> SelectedActors = Selection->GetSelectedActors();
        if (SelectedActors.Num() > 0)
        {
            Server_DestroyActor(SelectedActors);
        }
    }
}

void APlayerCamera::Server_DestroyActor_Implementation(const TArray<AActor*>& ActorToDestroy)
{
    for (AActor* Actor : ActorToDestroy)
    {
        if (Actor)
			Actor->Destroy();
    }
}

void APlayerCamera::HandleCommandActionStarted()
{
    if (PatrolComponent)
    {
        const bool bAltDown = Player && (Player->IsInputKeyDown(EKeys::LeftAlt) || Player->IsInputKeyDown(EKeys::RightAlt));
        if (PatrolComponent->HandleRightClickPressed(bAltDown))
        {
            bIsCommandActionHeld = false;
            return;
        }
    }

    if (!Player || bIsInSpawnUnits)
    {
        bIsCommandActionHeld = false;
        return;
    }

    bIsCommandActionHeld = true;

    const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
    CommandPreviewInstanceCount = 0;
    bCommandPreviewVisible = false;

    CommandStart();

    const FRotator BaseRotation(0.f, CameraComponent->GetComponentRotation().Yaw, 0.f);
    CommandRotationPreview.BeginHold(CurrentTime, CommandLocation, BaseRotation);
}

void APlayerCamera::CommandStart()
{
    if (!Player || bIsInSpawnUnits)
		return;

    if (UUnitSelectionComponent* Selection = GetSelectionComponentChecked())
    {
        const FVector MouseLocation = Selection->GetMousePositionOnTerrain().Location;
        CommandLocation = FVector(MouseLocation.X, MouseLocation.Y, MouseLocation.Z);
    }
}

void APlayerCamera::Command()
{
    const bool bAltDown = Player && (Player->IsInputKeyDown(EKeys::LeftAlt) || Player->IsInputKeyDown(EKeys::RightAlt));
    if (PatrolComponent && PatrolComponent->HandleRightClickReleased(bAltDown))
    {
        bIsCommandActionHeld = false;
        StopCommandPreview();
        return;
    }

    if (!Player || bIsInSpawnUnits || bAltIsPressed)
    {
        bIsCommandActionHeld = false;
        StopCommandPreview();
        return;
    }

    FHitResult Hit;
    if (UUnitSelectionComponent* Selection = GetSelectionComponentChecked())
    {
        Hit = Selection->GetMousePositionOnTerrain();
    }
    else
    {
        bIsCommandActionHeld = false;
        StopCommandPreview();
        return;
    }

    AActor* ActorEnemy = GetSelectedObject();

    if (ActorEnemy && ActorEnemy->Implements<USelectable>())
    {
        if (OrderComponent)
        {
            OrderComponent->IssueOrder(CreateCommandData(CommandAttack, ActorEnemy));
        }

        bIsCommandActionHeld = false;
        StopCommandPreview();
        return;
    }

    const bool bHasValidGround = Hit.bBlockingHit;
    const bool bUseRotationPreview = CommandRotationPreview.bPreviewActive;

    if (!bHasValidGround && !bUseRotationPreview)
    {
        bIsCommandActionHeld = false;
        StopCommandPreview();
        return;
    }

    const FVector TempTargetLocation = bUseRotationPreview && !CommandRotationPreview.Center.IsZero() ? CommandRotationPreview.Center : Hit.Location;

    if (OrderComponent)
    {
        const FRotator DefaultRotation(0.f, CameraComponent->GetComponentRotation().Yaw, 0.f);
        const FRotator FinalRotation = bUseRotationPreview ? CommandRotationPreview.CurrentRotation : DefaultRotation;

        FCommandData CommandData(Player, TempTargetLocation, FinalRotation, CommandMove);
        CommandData.SourceLocation = TempTargetLocation;
        CommandData.Rotation = FinalRotation;

        OrderComponent->IssueOrder(CommandData);
    }

    bIsCommandActionHeld = false;
    StopCommandPreview();
}

FCommandData APlayerCamera::CreateCommandData(const ECommandType Type, AActor* Enemy, const float Radius) const
{
        if (!Player)
        {
                return FCommandData();
        }

        if (Type == CommandAttack)
        {
                return FCommandData(Player, CommandLocation, CameraComponent->GetComponentRotation(), Type, Enemy);
        }

        if (Type == CommandPatrol)
        {
                return FCommandData(Player, CommandLocation, CameraComponent->GetComponentRotation(), Type, nullptr, Radius * 2.f);
        }

        return FCommandData(Player, CommandLocation, CameraComponent->GetComponentRotation(), Type);
}

void APlayerCamera::UpdateCommandPreview()
{
        if (!bIsCommandActionHeld)
        {
                return;
        }

        if (!Player)
        {
                bIsCommandActionHeld = false;
                StopCommandPreview();
                return;
        }

        UUnitSelectionComponent* Selection = GetSelectionComponentChecked();
        if (!Selection)
        {
                StopCommandPreview();
                return;
        }

        const UWorld* World = GetWorld();
        const float CurrentTime = World ? World->GetTimeSeconds() : 0.f;

        const FHitResult MouseHit = Selection->GetMousePositionOnTerrain();
        const FVector MouseLocation = MouseHit.Location;

        if (!CommandRotationPreview.bPreviewActive)
        {
                if (!CommandRotationPreview.TryActivate(CurrentTime, RotationPreviewHoldTime, CommandRotationPreview.Center, MouseLocation))
                {
                        return;
                }

                BeginCommandRotationPreview(MouseLocation);

                if (!CommandRotationPreview.bPreviewActive)
                {
                        return;
                }
        }

        CommandRotationPreview.UpdateRotation(MouseLocation);

        if (!PreviewUnit)
        {
                return;
        }

        TArray<FTransform> InstanceTransforms;
        BuildCommandPreviewTransforms(CommandRotationPreview.Center, CommandRotationPreview.CurrentRotation, InstanceTransforms);

        if (!InstanceTransforms.IsEmpty())
        {
                PreviewUnit->UpdateInstances(InstanceTransforms);
        }
}

void APlayerCamera::BeginCommandRotationPreview(const FVector& MouseLocation)
{
    if (!SelectionComponent)
		return;

    const TArray<AActor*> SelectedUnits = SelectionComponent->GetSelectedActors();
    if (SelectedUnits.IsEmpty())
    {
        CommandRotationPreview.Deactivate();
        return;
    }

    if (CommandRotationPreview.Center.IsZero() && MouseLocation != FVector::ZeroVector)
    {
        CommandRotationPreview.Center = MouseLocation;
    }

    if (!EnsureCommandPreviewMesh(SelectedUnits))
    {
        CommandRotationPreview.Deactivate();
        return;
    }

    CommandRotationPreview.bPreviewActive = true;
    CommandRotationPreview.BaseRotation = FRotator(0.f, CameraComponent->GetComponentRotation().Yaw, 0.f);
    CommandRotationPreview.CurrentRotation = CommandRotationPreview.BaseRotation;
    CommandRotationPreview.InitialDirection = FRotationPreviewState::ResolvePlanarDirection(MouseLocation - CommandRotationPreview.Center, CommandRotationPreview.BaseRotation);

    if (PreviewUnit)
    {
        TArray<FTransform> InstanceTransforms;
        BuildCommandPreviewTransforms(CommandRotationPreview.Center, CommandRotationPreview.CurrentRotation, InstanceTransforms);
    	
        if (!InstanceTransforms.IsEmpty())
        {
            PreviewUnit->UpdateInstances(InstanceTransforms);
        }
    }
}

void APlayerCamera::StopCommandPreview()
{
    if (bCommandPreviewVisible && PreviewUnit && !bIsInSpawnUnits)
    {
        PreviewUnit->HidePreview();
    }

    CommandRotationPreview.Deactivate();
    bCommandPreviewVisible = false;
    CommandPreviewInstanceCount = 0;
}

bool APlayerCamera::EnsureCommandPreviewMesh(const TArray<AActor*>& SelectedUnits)
{
    if (SelectedUnits.IsEmpty())
		return false;

    EnsurePreviewUnit();

    if (!HasPreviewUnits())
		return false;

    USkeletalMesh* SkeletalMesh = nullptr;
    FVector SkeletalScale = FVector::OneVector;
    UStaticMesh* StaticMesh = nullptr;
    FVector StaticScale = FVector::OneVector;

    for (AActor* Unit : SelectedUnits)
    {
        if (!Unit)
			continue;

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
			break;
    }

    const int32 InstanceCount = SelectedUnits.Num();

    if (SkeletalMesh)
    {
        PreviewUnit->ShowPreview(SkeletalMesh, SkeletalScale, InstanceCount);
        bCommandPreviewVisible = true;
        CommandPreviewInstanceCount = InstanceCount;
        return true;
    }

    if (StaticMesh)
    {
        PreviewUnit->ShowPreview(StaticMesh, StaticScale, InstanceCount);
        bCommandPreviewVisible = true;
        CommandPreviewInstanceCount = InstanceCount;
        return true;
    }

	return false;
}

void APlayerCamera::BuildCommandPreviewTransforms(const FVector& CenterLocation, const FRotator& FacingRotation, TArray<FTransform>& OutTransforms) const
{
    OutTransforms.Reset();

    if (!SelectionComponent)
    {
        OutTransforms.Add(FTransform(FacingRotation, CenterLocation));
        return;
    }

    const TArray<AActor*> SelectedUnits = SelectionComponent->GetSelectedActors();
    if (SelectedUnits.IsEmpty())
    {
        OutTransforms.Add(FTransform(FacingRotation, CenterLocation));
        return;
    }

    if (FormationComponent)
    {
        FCommandData BaseCommand(Player, CenterLocation, FacingRotation, ECommandType::CommandMove);
        BaseCommand.SourceLocation = CenterLocation;
        BaseCommand.Location = CenterLocation;
        BaseCommand.Rotation = FacingRotation;

        TArray<FCommandData> Commands;
        FormationComponent->BuildFormationCommands(BaseCommand, SelectedUnits, Commands);

        if (!Commands.IsEmpty())
        {
            OutTransforms.Reserve(Commands.Num());
            for (const FCommandData& Command : Commands)
            {
                OutTransforms.Add(FTransform(FacingRotation, Command.Location));
            }
        	
            return;
        }
    }

    OutTransforms.Reserve(SelectedUnits.Num());
    for (int32 Index = 0; Index < SelectedUnits.Num(); ++Index)
    {
        OutTransforms.Add(FTransform(FacingRotation, CenterLocation));
    }
}

