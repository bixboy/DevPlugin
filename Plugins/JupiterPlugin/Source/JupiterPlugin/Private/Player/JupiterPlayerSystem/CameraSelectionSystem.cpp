#include "Player/JupiterPlayerSystem/CameraSelectionSystem.h"
#include "Player/JupiterPlayerSystem/CameraCommandSystem.h"
#include "Player/JupiterPlayerSystem/CameraPlacementSystem.h"
#include "Player/Selections/SelectionBox.h"
#include "Player/PlayerCamera.h"

#include "Components/Patrol/PatrolVisualizerComponent.h"
#include "Components/Unit/UnitSelectionComponent.h"
#include "Components/Patrol/UnitPatrolComponent.h"

#include "GameFramework/PlayerController.h"
#include "Interfaces/Selectable.h"
#include "Engine/World.h"
#include "Subsystems/UnitSpatialGridSubsystem.h"


void UCameraSelectionSystem::Init(APlayerCamera* InOwner)
{
    Super::Init(InOwner);

    if (!GetWorldSafe() || !InOwner)
    	return;

    if (InOwner->GetSelectionBoxClass())
    {
        FActorSpawnParameters Params;
        Params.Owner = InOwner;
        Params.Instigator = InOwner;

        SelectionBox = GetWorldSafe()->SpawnActor<ASelectionBox>(
            InOwner->GetSelectionBoxClass(),
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            Params
        );

        if (SelectionBox)
        {
            SelectionBox->SetOwner(InOwner);
            SelectionBox->SetActorHiddenInGame(true);
        }
    }
}

void UCameraSelectionSystem::Tick(float DeltaTime)
{
    if (bIsDraggingPatrol)
    {
        UpdatePatrolDrag();
    }

    if (bBoxSelect && SelectionBox)
    {
        UpdateBoxSelection();
    }
}

// --------------------------------------------------
// INPUT : Mouse Down
// --------------------------------------------------
void UCameraSelectionSystem::HandleSelectionPressed()
{
    if (!GetOwner() || !GetSelectionComponent())
    	return;

    if (CommandSystem && CommandSystem->IsBuildingPatrolPath())
        return;
    
    if (TryStartPatrolDrag())
        return;
    
    FHitResult Hit;
    if (!GetMouseHitOnTerrain(Hit))
    {
        bMouseGrounded = false;
        return;
    }
    
    bMouseGrounded = true;
    ClickStartLocation = Hit.Location;
    
    if (APlayerController* PC = GetOwner()->GetPlayerController())
    {
         double X, Y;
         if (PC->GetMousePosition(X, Y))
         {
             ClickScreenLocation = FVector2D(X, Y);
         }
    }

    if (PlacementSystem)
    	PlacementSystem->CancelPlacement();
	
}

// --------------------------------------------------
// INPUT : Mouse Released
// --------------------------------------------------
void UCameraSelectionSystem::HandleSelectionReleased()
{
    if (bIsDraggingPatrol)
    {
        EndPatrolDrag();
        bMouseGrounded = false;
        return;
    }

    if (!bMouseGrounded)
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleSelectionReleased: Skipped because !bMouseGrounded"));
    	return;
    }

    UE_LOG(LogTemp, Warning, TEXT("HandleSelectionReleased: Executing FinalizeSelection"));
    bMouseGrounded = false;

    if (bBoxSelect)
    {
        EndBoxSelection();
        bBoxSelect = false;
        return;
    }

    FinalizeSelection();
}

// --------------------------------------------------
// INPUT : Mouse Hold (Triggered)
// --------------------------------------------------
void UCameraSelectionSystem::HandleSelectionHold(const FInputActionValue& Value)
{
    if (!bMouseGrounded)
    	return;

    APlayerController* PC = GetOwner()->GetPlayerController();
    if (!PC)
    	return;

    float TimeDown = PC->GetInputKeyTimeDown(EKeys::LeftMouseButton);

    if (TimeDown >= LeftMouseHoldThreshold && !bBoxSelect)
    {
        double X, Y;
        if (PC->GetMousePosition(X, Y))
        {
             float Dist = FVector2D::Distance(FVector2D(X, Y), ClickScreenLocation);
             if (Dist > DragStartThreshold)
             {
                 StartBoxSelection();
                 bBoxSelect = true;
             }
        }
    }
}

// --------------------------------------------------
// BOX LOGIC
// --------------------------------------------------

void UCameraSelectionSystem::StartBoxSelection()
{
    if (!SelectionBox || !GetSelectionComponent())
    	return;

    GetSelectionComponent()->Handle_Selection(nullptr);
	
    SelectionBox->Start(ClickStartLocation, FRotator::ZeroRotator); 
}

void UCameraSelectionSystem::UpdateBoxSelection()
{
    if (!SelectionBox)
    	return;

    FHitResult Hit;
    if (GetMouseHitOnTerrain(Hit))
    {
    	SelectionBox->UpdateEndLocation(Hit.Location);
    }
}

void UCameraSelectionSystem::EndBoxSelection()
{
    if (SelectionBox && GetSelectionComponent())
    {
        TArray<AActor*> SelectedActors = SelectionBox->End();
        
        if (SelectedActors.Num() > 0)
        {
            GetSelectionComponent()->Handle_Selection(SelectedActors);
        }
        else
        {
            GetSelectionComponent()->Handle_Selection(nullptr); 
        }
    }
}


// --------------------------------------------------
// SINGLE CLICK LOGIC
// --------------------------------------------------
void UCameraSelectionSystem::FinalizeSelection()
{
    if (!GetSelectionComponent())
    	return;

    AActor* HitActor = GetHoveredActor();

    if (HitActor)
    {
        GetSelectionComponent()->Handle_Selection(HitActor);
    }
    else
    {
        GetSelectionComponent()->Handle_Selection(nullptr);
        
        if (UUnitPatrolComponent* PatrolComp = GetOwner()->FindComponentByClass<UUnitPatrolComponent>())
        {
            PatrolComp->SetUISelectedPatrol(FGuid());
        }
    }
}


// --------------------------------------------------
// DOUBLE TAP : Select All Visible of Type
// --------------------------------------------------
void UCameraSelectionSystem::HandleSelectAll()
{
    if (!GetSelectionComponent() || !GetOwner())
    	return;

    TArray<AActor*> CurrentSelection = GetSelectionComponent()->GetSelectedActors();
    if (CurrentSelection.IsEmpty())
    	return;

    AActor* ReferenceUnit = CurrentSelection[0];
    if (!ReferenceUnit->Implements<USelectable>())
    	return;

    const ETeams RefTeam = ISelectable::Execute_GetCurrentTeam(ReferenceUnit);
	
    TArray<AActor*> Candidates;
    bool bUseFallback = true;
    
    if (UGameInstance* GI = GetWorldSafe() ? GetWorldSafe()->GetGameInstance() : nullptr)
    {
        if (UUnitSpatialGridSubsystem* Spatial = GI->GetSubsystem<UUnitSpatialGridSubsystem>())
        {
            FVector CamLoc = GetOwner()->GetActorLocation();
            float Range = 6000.f;
            FVector2D Min(CamLoc.X - Range, CamLoc.Y - Range);
            FVector2D Max(CamLoc.X + Range, CamLoc.Y + Range);
            
            Candidates = Spatial->GetUnitsInBounds(Min, Max);
            bUseFallback = false;
        }
    }

    if (bUseFallback)
    {
         Candidates = GetOwner()->GetAllActorsOfClassInCameraBound<AActor>(GetWorldSafe(), AActor::StaticClass());
    }

    TArray<AActor*> ToSelect;
    ToSelect.Reserve(Candidates.Num());

    int32 ViewX = 0, ViewY = 0;
    GetOwner()->GetPlayerController()->GetViewportSize(ViewX, ViewY);

    for (AActor* Actor : Candidates)
    {
        if (Actor && Actor->Implements<USelectable>())
        {
            if (!bUseFallback)
            {
                 FVector2D ScreenPos;
                 if (GetOwner()->GetPlayerController()->ProjectWorldLocationToScreen(Actor->GetActorLocation(), ScreenPos))
                 {
                     if (ScreenPos.X < 0 || ScreenPos.X > ViewX || ScreenPos.Y < 0 || ScreenPos.Y > ViewY)
						continue;
                 }
            }

            bool bSameClass = Actor->GetClass() == ReferenceUnit->GetClass();
            bool bSameTeam = ISelectable::Execute_GetCurrentTeam(Actor) == RefTeam;

            if (bSameClass && bSameTeam)
            {
                ToSelect.Add(Actor);
            }
        }
    }

    if (ToSelect.Num() > 0)
    {
        GetSelectionComponent()->Handle_Selection(ToSelect);
    }
}

// --------------------------------------------------
// Control Groups
// --------------------------------------------------

void UCameraSelectionSystem::HandleControlGroupInput(const FInputActionValue& Value)
{
	APlayerController* PC = GetOwner() ? GetOwner()->GetPlayerController() : nullptr;
	if (!PC)
		return;
	
	FVector InputVector = Value.Get<FVector>(); 
	int32 RawValue = FMath::RoundToInt(InputVector.X);
	int32 GroupIndex = RawValue % 10;

	const bool bCtrl = PC->IsInputKeyDown(EKeys::LeftControl) || PC->IsInputKeyDown(EKeys::RightControl);
	const bool bAlt = PC->IsInputKeyDown(EKeys::LeftAlt) || PC->IsInputKeyDown(EKeys::RightAlt);

	if (bCtrl)
	{
		HandleSetGroup(GroupIndex);
	}
	else if (bAlt)
	{
		HandleClearGroup(GroupIndex);
	}
	else
	{
		HandleRecallGroup(GroupIndex);
	}
}

void UCameraSelectionSystem::HandleRecallGroup(int32 Index)
{
    if (GetSelectionComponent())
    {
        GetSelectionComponent()->RecallControlGroup(Index);
    }
}

void UCameraSelectionSystem::HandleSetGroup(int32 Index)
{
    if (GetSelectionComponent())
    {
        GetSelectionComponent()->SetControlGroup(Index);
    }
}

void UCameraSelectionSystem::HandleClearGroup(int32 Index)
{
    if (GetSelectionComponent())
    {
        GetSelectionComponent()->ClearControlGroup(Index);
    }
}

// --------------------------------------------------
// HELPERS
// --------------------------------------------------

bool UCameraSelectionSystem::GetMouseHitOnTerrain(FHitResult& OutHit) const
{
    if (!GetSelectionComponent())
    	return false;
	
    OutHit = GetSelectionComponent()->GetMousePositionOnTerrain();
    return OutHit.bBlockingHit;
}

AActor* UCameraSelectionSystem::GetHoveredActor() const
{
    if (!GetOwner() || !GetWorldSafe())
    	return nullptr;

    APlayerController* PC = GetOwner()->GetPlayerController();
    if (!PC)
    	return nullptr;

    FVector WLoc, WDir;
    if (PC->DeprojectMousePositionToWorld(WLoc, WDir))
    {
        FVector End = WLoc + WDir * 1000000.f;

        FHitResult Hit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(GetOwner());
    	
        if (SelectionBox)
        	Params.AddIgnoredActor(SelectionBox);

        if (GetWorldSafe()->LineTraceSingleByChannel(Hit, WLoc, End, ECC_Visibility, Params))
        {
            if (Hit.GetActor() && Hit.GetActor()->Implements<USelectable>())
            {
                return Hit.GetActor();
            }
        }
    }
    return nullptr;
}

// --------------------------------------------------
// PATROL DRAG LOGIC
// --------------------------------------------------

bool UCameraSelectionSystem::TryStartPatrolDrag()
{
    if (!GetOwner() || !GetWorldSafe())
        return false;

    APlayerController* PC = GetOwner()->GetPlayerController();
    if (!PC)
        return false;

    FHitResult Hit;
    if (PC->GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit))
    {
         if (AActor* HitActor = Hit.GetActor())
         {
             if (UPatrolVisualizerComponent* Visualizer = HitActor->FindComponentByClass<UPatrolVisualizerComponent>())
             {
                 if (Hit.Item != INDEX_NONE)
                 {
                     if (Visualizer->GetPatrolPointFromHitIndex(Hit.Item, DraggedPatrolID, DraggedPointIndex))
                     {
                         UE_LOG(LogTemp, Warning, TEXT("[CameraSelection] Drag Started: Patrol %s Point %d"), *DraggedPatrolID.ToString(), DraggedPointIndex);
                         bIsDraggingPatrol = true;
                         DraggingPlaneLocation = Hit.Location; 
                         return true;
                     }
                 }
             }
         }
    }
    
    return false;
}

void UCameraSelectionSystem::UpdatePatrolDrag()
{
    if (!bIsDraggingPatrol)
        return;

    APlayerController* PC = GetOwner()->GetPlayerController();
    if (!PC)
        return;

    FVector WorldLoc, WorldDir;
    if (PC->DeprojectMousePositionToWorld(WorldLoc, WorldDir))
    {
        FVector End = WorldLoc + WorldDir * 1000000.0f;
        FHitResult Hit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(GetOwner());

        if (GetWorldSafe()->LineTraceSingleByChannel(Hit, WorldLoc, End, ECC_Visibility, Params))
        {
             FVector NewLocation = Hit.Location;
             
             if (UPatrolVisualizerComponent* Visualizer = GetOwner()->FindComponentByClass<UPatrolVisualizerComponent>())
             {
                 Visualizer->UpdatePointPosition(DraggedPatrolID, DraggedPointIndex, NewLocation);
             }
             
             DraggingPlaneLocation = NewLocation;
        }
    }
}

void UCameraSelectionSystem::EndPatrolDrag()
{
    if (!bIsDraggingPatrol)
        return;
        
    UE_LOG(LogTemp, Warning, TEXT("[CameraSelection] Drag Ended"));

    if (UUnitPatrolComponent* PatrolComp = GetOwner()->FindComponentByClass<UUnitPatrolComponent>())
    {
        PatrolComp->Server_UpdatePatrolPoint(DraggedPatrolID, DraggedPointIndex, DraggingPlaneLocation);
    }

    bIsDraggingPatrol = false;
    DraggedPointIndex = -1;
    DraggedPatrolID.Invalidate();
}