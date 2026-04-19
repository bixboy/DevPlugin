#include "UI/Editor/JupiterPageBase.h"


void UJupiterPageBase::InitPage(UCameraPlacementSystem* PlacementSys, UUnitPatrolComponent* PatrolComp, UUnitSelectionComponent* SelComp)
{
	PlacementSystem = TWeakObjectPtr(PlacementSys);
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
