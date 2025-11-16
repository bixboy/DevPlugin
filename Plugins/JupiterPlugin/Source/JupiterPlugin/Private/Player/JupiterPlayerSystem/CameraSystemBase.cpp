#include "Player/JupiterPlayerSystem/CameraSystemBase.h"
#include "Player/PlayerCamera.h"


void UCameraSystemBase::Init(APlayerCamera* InOwner)
{
	Owner = InOwner;
}

void UCameraSystemBase::Tick(float DeltaTime)
{
	// Default: do nothing
}


// -----------------------------------------------------------
//	Getters
// -----------------------------------------------------------

APlayerCamera* UCameraSystemBase::GetOwner() const
{
	return Owner.IsValid() ? Owner.Get() : nullptr;
}

UWorld* UCameraSystemBase::GetWorldSafe() const
{
	return Owner.IsValid() ? Owner->GetWorld() : nullptr;
}

UUnitSelectionComponent* UCameraSystemBase::GetSelectionComponent() const
{
	return Owner.IsValid() ? Owner->GetSelectionComponent() : nullptr;
}

UUnitOrderComponent* UCameraSystemBase::GetOrderComponent() const
{
	return Owner.IsValid() ? Owner->GetOrderComponent() : nullptr;
}

UUnitFormationComponent* UCameraSystemBase::GetFormationComponent() const
{
	return Owner.IsValid() ? Owner->GetFormationComponent() : nullptr;
}

UUnitSpawnComponent* UCameraSystemBase::GetSpawnComponent() const
{
	return Owner.IsValid() ? Owner->GetSpawnComponent() : nullptr;
}

UUnitPatrolComponent* UCameraSystemBase::GetPatrolComponent() const
{
	return Owner.IsValid() ? Owner->GetPatrolComponent() : nullptr;
}