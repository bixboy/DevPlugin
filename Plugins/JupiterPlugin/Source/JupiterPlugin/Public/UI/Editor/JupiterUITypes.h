#pragma once

#include "CoreMinimal.h"
#include "JupiterUITypes.generated.h"

UENUM(BlueprintType)
enum class EEditorPage : uint8
{
	Units		UMETA(DisplayName = "Units Spawn"),
	Patrols		UMETA(DisplayName = "Patrols"),
	Settings	UMETA(DisplayName = "Settings")
};
