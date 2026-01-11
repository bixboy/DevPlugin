#include "Core/JupiterGameState.h"
#include "Components/Unit/SoldierManagerComponent.h"


AJupiterGameState::AJupiterGameState()
{
	SoldierManager = CreateDefaultSubobject<USoldierManagerComponent>(TEXT("SoldierManager"));
}
