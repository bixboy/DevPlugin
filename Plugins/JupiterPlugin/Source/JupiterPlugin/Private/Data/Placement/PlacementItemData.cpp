#include "Data/Placement/PlacementItemData.h"


UObject* UPlacementItemData::GetPreviewAsset_Implementation() const
{
    if (PreviewMesh.IsNull())
		return nullptr;
	
    if (PreviewMesh.IsValid())
		return PreviewMesh.Get();
    
    return nullptr;
}

bool UPlacementItemData::SupportsFormations_Implementation() const
{
    return false;
}

bool UPlacementItemData::IsGroupPlacement_Implementation() const
{
    return false;
}

bool UPlacementItemData::IsPlacementValid_Implementation(const FVector& Location, const FHitResult& Hit) const
{
    return true;
}

int32 UPlacementItemData::GetDefaultUnitCount_Implementation() const
{
    return 1;
}

float UPlacementItemData::GetFormationSpacing_Implementation() const
{
    return 100.0f;
}

uint8 UPlacementItemData::GetDefaultFormation_Implementation() const
{
    return 0;
}
