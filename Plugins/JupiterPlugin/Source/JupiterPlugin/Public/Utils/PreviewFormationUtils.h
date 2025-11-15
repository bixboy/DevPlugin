#pragma once

#include "CoreMinimal.h"
#include "Data/AiData.h"

class UUnitSpawnComponent;
class UUnitFormationComponent;

namespace PreviewFormationUtils
{
    /** Calculates how many units should be represented based on spawn settings. */
    JUPITERPLUGIN_API int32 GetEffectiveSpawnCount(const UUnitSpawnComponent* SpawnComponent);

    /** Produces offset positions for spawn previews based on the requested formation. */
    JUPITERPLUGIN_API void BuildSpawnFormationOffsets(const UUnitSpawnComponent* SpawnComponent, int32 SpawnCount, float Spacing, TArray<FVector>& OutOffsets);

    /** Builds transforms for command previews matching the current formation. */
    JUPITERPLUGIN_API void BuildCommandPreviewTransforms(const UUnitFormationComponent* FormationComponent, const FCommandData& BaseCommand, const TArray<AActor*>& SelectedUnits, TArray<FTransform>& OutTransforms);
}

