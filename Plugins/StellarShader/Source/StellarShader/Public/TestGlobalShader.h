// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

/**
 * FTestShaderVS — Fullscreen triangle vertex shader.
 * Generates 3 vertices from SV_VertexID to cover the entire screen.
 */
class FTestShaderVS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FTestShaderVS);
	SHADER_USE_PARAMETER_STRUCT(FTestShaderVS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	END_SHADER_PARAMETER_STRUCT()
};

/**
 * FTestShaderPS — Animated plasma/voronoi pixel shader.
 * Renders a colorful animated effect with time and resolution parameters.
 */
class FTestShaderPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FTestShaderPS);
	SHADER_USE_PARAMETER_STRUCT(FTestShaderPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(float, TestShaderTime)
		SHADER_PARAMETER(FVector2f, TestShaderResolution)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()
};
