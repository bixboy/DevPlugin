#include "Component/HoverPointComponent.h"


UHoverPointComponent::UHoverPointComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UHoverPointComponent::BeginPlay()
{
	Super::BeginPlay();

	HoverPoint.LocalOffset = GetComponentLocation();
}

