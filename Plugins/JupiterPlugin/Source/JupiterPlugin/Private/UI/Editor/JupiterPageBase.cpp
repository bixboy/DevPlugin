#include "UI/Editor/JupiterPageBase.h"


void UJupiterPageBase::InitPage(UUnitSpawnComponent* SpawnComp, UUnitPatrolComponent* PatrolComp, UUnitSelectionComponent* SelComp)
{
	SpawnComponent = TWeakObjectPtr(SpawnComp);
	PatrolComponent = TWeakObjectPtr(PatrolComp);
	SelectionComponent = TWeakObjectPtr(SelComp);
}

void UJupiterPageBase::OnPageOpened()
{
	// Override in child classes
}

void UJupiterPageBase::OnPageClosed()
{
	// Override in child classes
}
