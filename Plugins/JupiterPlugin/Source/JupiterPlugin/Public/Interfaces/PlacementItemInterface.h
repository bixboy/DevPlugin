#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PlacementItemInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UPlacementItemInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for any data asset that can be placed by the Placement System.
 * (Units, Props, Presets, Vehicles...)
 */
class JUPITERPLUGIN_API IPlacementItemInterface
{
	GENERATED_BODY()

public:
	/** Returns the asset to show in preview (StaticMesh, SkeletalMesh, or specialized logic) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Placement Interface")
	UObject* GetPreviewAsset() const;

	/** Does this item support formations (Line, Wedge...)? */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Placement Interface")
	bool SupportsFormations() const;

    /** Should this item create a group of actors? */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Placement Interface")
    bool IsGroupPlacement() const;
    
    /** Validates if the item can be placed (e.g. slope check, water check) */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Placement Interface")
    bool IsPlacementValid(const FVector& Location, const FHitResult& Hit) const;

    // --- Formation API ---
    
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Placement Interface")
    int32 GetDefaultUnitCount() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Placement Interface")
    float GetFormationSpacing() const;

    /** Returns Enum as int (or Byte) */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Placement Interface")
    uint8 GetDefaultFormation() const;
};
