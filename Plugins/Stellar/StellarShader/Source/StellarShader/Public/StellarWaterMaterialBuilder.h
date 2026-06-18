// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StellarWaterMaterialBuilder.generated.h"

class UMaterial;

/**
 * UStellarWaterMaterialBuilder
 *
 * Editor-only utility to programmatically create the base water material.
 * The material uses a UMaterialExpressionCustom that #includes StellarWater.ush
 * and exposes all parameters as ScalarParameter / VectorParameter expressions.
 *
 * Usage: Console command "StellarShader.CreateWaterMaterial"
 */
UCLASS()
class STELLARSHADER_API UStellarWaterMaterialBuilder : public UObject
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	/**
	 * Creates and saves the water material to the plugin's Content folder.
	 * Returns the created material, or nullptr on failure.
	 */
	UFUNCTION()
	static UMaterial* CreateWaterMaterial();
#endif
};
