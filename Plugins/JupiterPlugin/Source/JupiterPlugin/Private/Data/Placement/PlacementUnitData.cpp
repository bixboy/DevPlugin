#include "Data/Placement/PlacementUnitData.h"


int32 UPlacementUnitData::GetDefaultUnitCount_Implementation() const
{
    return DefaultUnitCount;
}

float UPlacementUnitData::GetFormationSpacing_Implementation() const
{
    return FormationSpacing;
}

uint8 UPlacementUnitData::GetDefaultFormation_Implementation() const
{
    return static_cast<uint8>(DefaultFormation);
}
