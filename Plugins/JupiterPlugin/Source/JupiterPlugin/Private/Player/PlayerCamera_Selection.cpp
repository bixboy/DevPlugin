#include "JupiterPlugin/Public/Player/PlayerCamera.h"

#include "Components/UnitOrderComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "Components/UnitSpawnComponent.h"
#include "Components/UnitPatrolComponent.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/Selectable.h"
#include "Player/Selections/SelectionBox.h"
#include "Player/Selections/SphereRadius.h"
#include "Units/SoldierRts.h"

void APlayerCamera::Input_LeftMouseReleased()
{
        if (!MouseProjectionIsGrounded)
        {
                return;
        }

        HandleLeftMouse(IE_Released, 0.f);
}

void APlayerCamera::Input_LeftMouseInputHold(const FInputActionValue& ActionValue)
{
        if (!MouseProjectionIsGrounded)
        {
                return;
        }

        const float Value = ActionValue.Get<float>();
        HandleLeftMouse(IE_Repeat, Value);
}

void APlayerCamera::Input_SquareSelection()
{
        if (!Player)
        {
                return;
        }

        BoxSelect = false;

        if (SpawnComponent)
        {
                SpawnComponent->SetUnitToSpawn(nullptr);
        }

        HidePreview();

        if (UUnitSelectionComponent* Selection = GetSelectionComponentChecked())
        {
                Selection->Handle_Selection(nullptr);

                const FHitResult Hit = Selection->GetMousePositionOnTerrain();
                MouseProjectionIsGrounded = Hit.bBlockingHit;

                if (MouseProjectionIsGrounded)
                {
                        LeftMouseHitLocation = Hit.Location;
                }
        }
        else
        {
                MouseProjectionIsGrounded = false;
        }
}

void APlayerCamera::HandleLeftMouse(EInputEvent InputEvent, float Value)
{
        const bool bPatrolConsumed = PatrolComponent && PatrolComponent->HandleLeftClick(InputEvent);

        if (bPatrolConsumed)
        {
                return;
        }

        if (!Player || !MouseProjectionIsGrounded)
        {
                return;
        }

        switch (InputEvent)
        {
                case IE_Pressed:
                {
                        if (UUnitSelectionComponent* Selection = GetSelectionComponentChecked())
                        {
                                Selection->Handle_Selection(nullptr);
                                BoxSelect = false;
                                LeftMouseHitLocation = Selection->GetMousePositionOnTerrain().Location;
                                CommandStart();
                        }
                        break;
                }

                case IE_Released:
                        MouseProjectionIsGrounded = false;

                        if (BoxSelect && SelectionBox)
                        {
                                SelectionBox->End();
                                BoxSelect = false;
                        }
                        else if (UUnitSelectionComponent* Selection = GetSelectionComponentChecked())
                        {
                                Selection->Handle_Selection(GetSelectedObject());
                        }
                        break;

                case IE_Repeat:
                        if (Value == 0.f)
                        {
                                if (SelectionBox)
                                {
                                        SelectionBox->End();
                                }
                                return;
                        }

                        if (Player->GetInputKeyTimeDown(EKeys::LeftMouseButton) >= LeftMouseHoldThreshold && SelectionBox)
                        {
                                if (!BoxSelect)
                                {
                                        SelectionBox->Start(LeftMouseHitLocation, TargetRotation);
                                        BoxSelect = true;
                                }
                        }
                        break;

                default:
                        break;
        }
}

void APlayerCamera::Input_SelectAllUnitType()
{
        TArray<AActor*> SelectedInCameraBound = GetAllActorsOfClassInCameraBound<AActor>(GetWorld(), ASoldierRts::StaticClass());
        TArray<AActor*> ActorsToSelect;
        if (!SelectedInCameraBound.IsEmpty())
        {
                if (UUnitSelectionComponent* Selection = GetSelectionComponentChecked())
                {
                        const TArray<AActor*> CurrentSelection = Selection->GetSelectedActors();
                        if (Player && !CurrentSelection.IsEmpty())
                        {
                                const ETeams Team = ISelectable::Execute_GetCurrentTeam(CurrentSelection[0]);
                                for (AActor* Solider : SelectedInCameraBound)
                                {
                                        if (Solider && Team == ISelectable::Execute_GetCurrentTeam(Solider))
                                        {
                                                ActorsToSelect.Add(Solider);
                                        }
                                }

                                if (!ActorsToSelect.IsEmpty())
                                {
                                        Selection->Handle_Selection(ActorsToSelect);
                                }
                        }
                }
        }
}

AActor* APlayerCamera::GetSelectedObject()
{
        if (UWorld* World = GetWorld())
        {
                FVector WorldLocation, WorldDirection;

                Player->DeprojectMousePositionToWorld(WorldLocation, WorldDirection);
                const FVector End = WorldDirection * 1000000.0f + WorldLocation;

                FCollisionQueryParams CollisionParams;
                CollisionParams.AddIgnoredActor(this);

                FHitResult Hit;
                if (World->LineTraceSingleByChannel(Hit, WorldLocation, End, ECC_Visibility, CollisionParams))
                {
                        if (AActor* HitActor = Hit.GetActor())
                        {
                                return HitActor;
                        }
                }
        }

        return nullptr;
}

void APlayerCamera::Input_AltFunction()
{
        if (!MouseProjectionIsGrounded)
        {
                return;
        }

        HandleAltRightMouse(EInputEvent::IE_Pressed, 1.f);
        CommandStart();
}

void APlayerCamera::Input_AltFunctionRelease()
{
        HandleAltRightMouse(EInputEvent::IE_Released, 0.f);
}

void APlayerCamera::Input_AltFunctionHold(const FInputActionValue& ActionValue)
{
        if (!MouseProjectionIsGrounded)
        {
                return;
        }

        const float Value = ActionValue.Get<float>();
        HandleAltRightMouse(EInputEvent::IE_Repeat, Value);
}

void APlayerCamera::HandleAltRightMouse(EInputEvent InputEvent, float Value)
{
        if (!Player)
        {
                return;
        }

        FHitResult Result;

        switch (InputEvent)
        {
                case IE_Pressed:
                {
                        if (!bAltIsPressed)
                        {
                                return;
                        }

                        if (UUnitSelectionComponent* Selection = GetSelectionComponentChecked())
                        {
                                Result = Selection->GetMousePositionOnTerrain();
                                LeftMouseHitLocation = Result.Location;
                        }
                        break;
                }
                case IE_Released:
                {
                        if (SphereRadiusEnable && SphereRadius)
                        {
                                SphereRadius->End();
                                SphereRadiusEnable = false;
                                if (OrderComponent)
                                {
                                        OrderComponent->IssueOrder(CreateCommandData(ECommandType::CommandPatrol, nullptr, SphereRadius->GetRadius()));
                                }
                        }
                        break;
                }
                case IE_Repeat:
                {
                        if (!bAltIsPressed)
                        {
                                if (SphereRadius)
                                {
                                        SphereRadius->End();
                                }
                                SphereRadiusEnable = false;
                                return;
                        }

                        if (Value == 0.f)
                        {
                                if (SphereRadius)
                                {
                                        SphereRadius->End();
                                }
                                return;
                        }

                        if (Player->GetInputKeyTimeDown(EKeys::RightMouseButton) >= LeftMouseHoldThreshold && SphereRadius)
                        {
                                if (!SphereRadiusEnable)
                                {
                                        SphereRadius->Start(LeftMouseHitLocation, TargetRotation);
                                        SphereRadiusEnable = true;
                                }
                        }
                        break;
                }
                default:
                        break;
        }
}

void APlayerCamera::CreateSelectionBox()
{
        if (SelectionBox)
        {
                return;
        }

        if (UWorld* World = GetWorld())
        {
                FActorSpawnParameters SpawnParams;
                SpawnParams.Instigator = this;
                SpawnParams.Owner = this;
                SelectionBox = World->SpawnActor<ASelectionBox>(SelectionBoxClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
                if (SelectionBox)
                {
                        SelectionBox->SetOwner(this);
                }
        }
}

void APlayerCamera::CreateSphereRadius()
{
        if (UWorld* World = GetWorld())
        {
                FActorSpawnParameters SpawnParams;
                SpawnParams.Instigator = this;
                SpawnParams.Owner = this;
                SphereRadius = World->SpawnActor<ASphereRadius>(SphereRadiusClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
                if (SphereRadius)
                {
                        SphereRadius->SetOwner(this);
                }
        }
}

