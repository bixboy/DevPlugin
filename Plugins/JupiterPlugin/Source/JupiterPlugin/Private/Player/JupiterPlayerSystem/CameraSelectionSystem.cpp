#include "Player/JupiterPlayerSystem/CameraSelectionSystem.h"

#include "Components/Unit/UnitSelectionComponent.h"
#include "Player/PlayerCamera.h"
#include "Player/Selections/SelectionBox.h"
#include "Interfaces/Selectable.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Player/JupiterPlayerSystem/CameraCommandSystem.h"
#include "Player/JupiterPlayerSystem/CameraSpawnSystem.h"


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
    {
        return;
    }
    
    // Raycast Terrain
    FHitResult Hit;
    if (!GetMouseHitOnTerrain(Hit))
    {
        bMouseGrounded = false;
        return;
    }

    bMouseGrounded = true;
    ClickStartLocation = Hit.Location;

    if (SpawnSystem)
    	SpawnSystem->ResetSpawnState();
	
}

// --------------------------------------------------
// INPUT : Mouse Released
// --------------------------------------------------
void UCameraSelectionSystem::HandleSelectionReleased()
{
    if (!bMouseGrounded)
    	return;

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
        StartBoxSelection();
        bBoxSelect = true;
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
	
    TArray<AActor*> VisibleActors = GetOwner()->GetAllActorsOfClassInCameraBound<AActor>(GetWorldSafe(), AActor::StaticClass());
    
    TArray<AActor*> ToSelect;
    ToSelect.Reserve(VisibleActors.Num());

    for (AActor* Actor : VisibleActors)
    {
        if (Actor && Actor->Implements<USelectable>())
        {
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

void UCameraSelectionSystem::HandleControlGroupInput(const FInputActionValue& Value, int32 GroupIndex)
{
    APlayerController* PC = GetOwner() ? GetOwner()->GetPlayerController() : nullptr;
    if (!PC)
    {
        return;
    }

    // Check Modifiers
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