#include "JupiterGameState.h"
#include "Components/SoldierManagerComponent.h"


AJupiterGameState::AJupiterGameState()
{
	SoldierManager = CreateDefaultSubobject<USoldierManagerComponent>(TEXT("SoldierManager"));
}
