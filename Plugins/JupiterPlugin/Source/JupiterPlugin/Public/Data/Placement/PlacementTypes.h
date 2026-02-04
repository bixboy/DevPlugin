#pragma once
#include "CoreMinimal.h"
#include "PlacementTypes.generated.h"


UENUM(BlueprintType)
enum class ESpawnFormation : uint8
{
	Square      UMETA(DisplayName = "Square"),
	Line        UMETA(DisplayName = "Line"),
	Column      UMETA(DisplayName = "Column"),
	Wedge       UMETA(DisplayName = "Wedge"),
	Custom      UMETA(DisplayName = "Custom")
};
