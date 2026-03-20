// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestGlobalShader.h"

IMPLEMENT_GLOBAL_SHADER(FTestShaderVS, "/StellarShader/Private/TestShader.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FTestShaderPS, "/StellarShader/Private/TestShader.usf", "MainPS", SF_Pixel);
