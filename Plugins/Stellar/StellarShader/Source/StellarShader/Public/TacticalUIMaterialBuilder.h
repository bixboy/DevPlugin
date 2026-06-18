// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TacticalUIMaterialBuilder.generated.h"

class UMaterial;

/**
 * UTacticalUIMaterialBuilder
 * 
 * Utility to programmatically create the Tactical UI Shader material.
 * This ensures the material is always synchronized with the C++ shader code.
 * 
 * Usage: Console command "StellarShader.CreateTacticalUIMaterial"
 */
UCLASS()
class STELLARSHADER_API UTacticalUIMaterialBuilder : public UObject
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	/**
	 * Creates and saves the high-fidelity tactical UI material.
	 * Returns the created material, or nullptr on failure.
	 */
	UFUNCTION()
	static UMaterial* CreateTacticalMaterial();
#endif
};
