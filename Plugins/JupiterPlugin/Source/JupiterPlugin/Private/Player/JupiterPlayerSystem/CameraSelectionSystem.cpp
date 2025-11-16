#include "Player/JupiterPlayerSystem/CameraSelectionSystem.h"
#include "Player/PlayerCamera.h"
#include "Components/UnitSelectionComponent.h"
#include "Player/Selections/SelectionBox.h"
#include "Interfaces/Selectable.h"
#include "Units/SoldierRts.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Player/JupiterPlayerSystem/CameraCommandSystem.h"
#include "Player/JupiterPlayerSystem/CameraSpawnSystem.h"


void UCameraSelectionSystem::Init(APlayerCamera* InOwner)
{
    Super::Init(InOwner);

    if (!GetWorldSafe() || !InOwner)
        return;

    // Spawn SelectionBox actor
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
        SelectionBox->SetOwner(InOwner);
}

void UCameraSelectionSystem::Tick(float DeltaTime)
{
    // Nothing to tick for now
}


// --------------------------------------------------
// INPUT : Mouse Down
// --------------------------------------------------
void UCameraSelectionSystem::HandleSelectionPressed()
{

	if (!Owner.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("GetSelectionComponent FAILED: Owner invalid"));
		return;
	}

	if (!Owner->GetSelectionComponent())
	{
		UE_LOG(LogTemp, Error, TEXT("GetSelectionComponent FAILED: Owner->SelectionComponent is NULL"));
	}
	
    if (!GetSelectionComponent())
        return;

	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, "Selection Started");
	
    FHitResult Hit;
    if (!GetMouseHit(Hit))
    {
        bMouseGrounded = false;
        return;
    }

    bMouseGrounded = true;

    if (SpawnSystem)
        SpawnSystem->ResetSpawnState();

    GetSelectionComponent()->Handle_Selection(nullptr);

    bBoxSelect = false;
    ClickStartLocation = Hit.Location;
}


// --------------------------------------------------
// INPUT : Mouse Released
// --------------------------------------------------
void UCameraSelectionSystem::HandleSelectionReleased()
{
    if (!bMouseGrounded)
        return;

    bMouseGrounded = false;

    if (bBoxSelect && SelectionBox)
    {
        SelectionBox->End();
        bBoxSelect = false;
        return;
    }

    FinalizeSelection();
}


// --------------------------------------------------
// INPUT : Mouse Hold
// --------------------------------------------------
void UCameraSelectionSystem::HandleSelectionHold(const FInputActionValue& Value)
{
    if (!bMouseGrounded)
        return;

    const float HeldValue = Value.Get<float>();

    if (HeldValue == 0.f)
    {
        if (SelectionBox)
            SelectionBox->End();
        return;
    }

    APlayerController* PC = GetOwner()->GetPlayerController();
    if (!PC || !SelectionBox)
        return;

    if (PC->GetInputKeyTimeDown(EKeys::LeftMouseButton) >= LeftMouseHoldThreshold)
    {
        if (!bBoxSelect)
        {
            StartBoxSelection();
            bBoxSelect = true;
        }
    }
}


// --------------------------------------------------
// FINAL SELECTION
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
}


// --------------------------------------------------
// Double Tap : Select all same type
// --------------------------------------------------
void UCameraSelectionSystem::HandleSelectAll()
{
    if (!GetSelectionComponent() || !GetOwner())
        return;

    TArray<AActor*> ToSelect;
    TArray<AActor*> VisibleUnits = GetOwner()->GetAllActorsOfClassInCameraBound<AActor>(GetWorldSafe(), ASoldierRts::StaticClass());

    if (VisibleUnits.IsEmpty())
        return;

    const TArray<AActor*> Current = GetSelectionComponent()->GetSelectedActors();
    if (Current.IsEmpty())
        return;

    const ETeams Team = ISelectable::Execute_GetCurrentTeam(Current[0]);

    for (AActor* Unit : VisibleUnits)
    {
        if (Unit && ISelectable::Execute_GetCurrentTeam(Unit) == Team)
            ToSelect.Add(Unit);
    }

    if (!ToSelect.IsEmpty())
        GetSelectionComponent()->Handle_Selection(ToSelect);
}

// --------------------------------------------------
// Helpers
// --------------------------------------------------

bool UCameraSelectionSystem::GetMouseHit(FHitResult& OutHit) const
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

    FVector Wl, Wd;
	
    APlayerController* PC = GetOwner()->GetPlayerController();
    if (!PC)
        return nullptr;

    PC->DeprojectMousePositionToWorld(Wl, Wd);
    const FVector End = Wl + Wd * 1000000.f;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());

    if (GetWorldSafe()->LineTraceSingleByChannel(Hit, Wl, End, ECC_Visibility, Params))
        return Hit.GetActor();

    return nullptr;
}

void UCameraSelectionSystem::StartBoxSelection()
{
    if (SelectionBox)
        SelectionBox->Start(ClickStartLocation, GetOwner()->TargetRotation);
}

void UCameraSelectionSystem::EndBoxSelection()
{
    if (SelectionBox)
        SelectionBox->End();
}

void UCameraSelectionSystem::ResetSelectionState()
{
    bMouseGrounded = false;
    bBoxSelect = false;
}
