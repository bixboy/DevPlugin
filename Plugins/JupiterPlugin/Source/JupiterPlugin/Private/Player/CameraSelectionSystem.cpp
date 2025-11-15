#include "Player/CameraSelectionSystem.h"

#include "Components/UnitSelectionComponent.h"
#include "Components/UnitSpawnComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Interfaces/Selectable.h"
#include "Kismet/GameplayStatics.h"
#include "Player/CameraCommandSystem.h"
#include "Player/CameraPatrolSystem.h"
#include "Player/CameraSpawnSystem.h"
#include "Player/PlayerCamera.h"
#include "Player/Selections/SelectionBox.h"
#include "Units/SoldierRts.h"
#include "Utils/JupiterInputHelpers.h"
#include "Utils/JupiterTraceUtils.h"

void UCameraSelectionSystem::Init(APlayerCamera* InOwner)
{
    Owner = InOwner;
    if (!Owner.IsValid())
    {
        return;
    }

    Player = Owner->GetPlayerController();
    SelectionComponent = Owner->GetSelectionComponent();
    SpawnComponent = Owner->GetSpawnComponent();

    EnsureSelectionBox();

    if (SelectionComponent)
    {
        SelectionComponent->CreateHud();
    }
}

void UCameraSelectionSystem::Tick(float DeltaTime)
{
    // Selection logic is event-driven; no recurring work required each tick currently.
}

void UCameraSelectionSystem::HandleSelectionPressed()
{
    if (!SelectionComponent)
    {
        return;
    }

    if (PatrolSystem.IsValid() && PatrolSystem->HandleLeftClick(EInputEvent::IE_Pressed))
    {
        return;
    }

    bBoxSelecting = false;

    if (SpawnComponent)
    {
        SpawnComponent->SetUnitToSpawn(nullptr);
    }

    if (SpawnSystem.IsValid())
    {
        SpawnSystem->OnSelectionInteractionStarted();
    }

    ClearCurrentSelection();
    UpdateMouseGroundLocation();

    if (CommandSystem.IsValid() && bMouseGrounded)
    {
        CommandSystem->NotifySelectionPressed(LastGroundLocation);
    }
}

void UCameraSelectionSystem::HandleSelectionReleased()
{
    if (!SelectionComponent)
    {
        return;
    }

    if (PatrolSystem.IsValid() && PatrolSystem->HandleLeftClick(EInputEvent::IE_Released))
    {
        return;
    }

    if (!bMouseGrounded)
    {
        return;
    }

    bMouseGrounded = false;

    if (bBoxSelecting)
    {
        EndBoxSelection();
        return;
    }

    if (AActor* HitActor = GetActorUnderCursor())
    {
        SelectionComponent->Handle_Selection(HitActor);
    }
    else
    {
        SelectionComponent->Handle_Selection(nullptr);
    }
}

void UCameraSelectionSystem::HandleSelectionHold(const FInputActionValue& Value)
{
    if (!SelectionComponent)
    {
        return;
    }

    if (PatrolSystem.IsValid() && PatrolSystem->HandleLeftClick(EInputEvent::IE_Repeat))
    {
        return;
    }

    const float InputValue = Value.Get<float>();
    if (FMath::IsNearlyZero(InputValue))
    {
        EndBoxSelection();
        return;
    }

    UpdateMouseGroundLocation();

    if (!Player.IsValid())
    {
        return;
    }

    const float HoldTime = JupiterInputHelpers::GetKeyTimeDown(Player.Get(), EKeys::LeftMouseButton);
    if (HoldTime >= LeftMouseHoldThreshold)
    {
        BeginBoxSelection();
    }
}

void UCameraSelectionSystem::HandleSelectAll()
{
    SelectActorsInCameraBounds();
}

void UCameraSelectionSystem::EnsureSelectionBox()
{
    if (SelectionBox || !SelectionBoxClass)
    {
        return;
    }

    if (UWorld* World = Owner.IsValid() ? Owner->GetWorld() : nullptr)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = Owner.Get();
        SpawnParams.Instigator = Owner.Get();
        SelectionBox = World->SpawnActor<ASelectionBox>(SelectionBoxClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
        if (SelectionBox)
        {
            SelectionBox->SetOwner(Owner.Get());
        }
    }
}

void UCameraSelectionSystem::UpdateMouseGroundLocation()
{
    if (!SelectionComponent)
    {
        bMouseGrounded = false;
        return;
    }

    const FHitResult Hit = SelectionComponent->GetMousePositionOnTerrain();
    bMouseGrounded = Hit.bBlockingHit;
    LastGroundLocation = bMouseGrounded ? Hit.Location : FVector::ZeroVector;
}

AActor* UCameraSelectionSystem::GetActorUnderCursor() const
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

void UCameraSelectionSystem::ClearCurrentSelection()
{
    if (SelectionComponent)
    {
        SelectionComponent->Handle_Selection(nullptr);
    }
}

void UCameraSelectionSystem::BeginBoxSelection()
{
    if (bBoxSelecting)
    {
        return;
    }

    if (!SelectionBox || !bMouseGrounded)
    {
        return;
    }

    const FRotator FacingRotation = Owner.IsValid() && Owner->GetSpringArm() ? Owner->GetSpringArm()->GetComponentRotation() : FRotator::ZeroRotator;
    SelectionBox->Start(LastGroundLocation, FacingRotation);
    bBoxSelecting = true;
}

void UCameraSelectionSystem::EndBoxSelection()
{
    if (!bBoxSelecting)
    {
        return;
    }

    if (SelectionBox)
    {
        SelectionBox->End();
    }

    bBoxSelecting = false;
}

void UCameraSelectionSystem::SelectActorsInCameraBounds()
{
    if (!SelectionComponent || !Owner.IsValid())
    {
        return;
    }

    TArray<ASoldierRts*> ActorsInView;
    GatherActorsInView<ASoldierRts>(ASoldierRts::StaticClass(), ActorsInView);
    if (ActorsInView.IsEmpty())
    {
        return;
    }

    const TArray<AActor*> CurrentlySelected = SelectionComponent->GetSelectedActors();
    if (CurrentlySelected.IsEmpty())
    {
        TArray<AActor*> Converted;
        Converted.Reserve(ActorsInView.Num());
        for (ASoldierRts* Soldier : ActorsInView)
        {
            Converted.Add(Soldier);
        }
        SelectionComponent->Handle_Selection(Converted);
        return;
    }

    const ETeams Team = ISelectable::Execute_GetCurrentTeam(CurrentlySelected[0]);
    TArray<AActor*> ActorsToSelect;
    for (ASoldierRts* Soldier : ActorsInView)
    {
        if (Soldier && ISelectable::Execute_GetCurrentTeam(Soldier) == Team)
        {
            ActorsToSelect.Add(Soldier);
        }
    }

    if (!ActorsToSelect.IsEmpty())
    {
        SelectionComponent->Handle_Selection(ActorsToSelect);
    }
}

template <typename T>
void UCameraSelectionSystem::GatherActorsInView(TSubclassOf<T> ActorClass, TArray<T*>& OutActors) const
{
    OutActors.Reset();

    if (!Owner.IsValid())
    {
        return;
    }

    UWorld* World = Owner->GetWorld();
    if (!World)
    {
        return;
    }

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, ActorClass, FoundActors);

    if (!Player.IsValid())
    {
        return;
    }

    int32 ViewportX = 0;
    int32 ViewportY = 0;
    Player->GetViewportSize(ViewportX, ViewportY);

    for (AActor* Actor : FoundActors)
    {
        if (!Actor)
        {
            continue;
        }

        FVector2D ScreenLocation;
        if (Player->ProjectWorldLocationToScreen(Actor->GetActorLocation(), ScreenLocation))
        {
            if (ScreenLocation.X >= 0.f && ScreenLocation.X <= ViewportX && ScreenLocation.Y >= 0.f && ScreenLocation.Y <= ViewportY)
            {
                if (T* CastedActor = Cast<T>(Actor))
                {
                    OutActors.Add(CastedActor);
                }
            }
        }
    }
}

