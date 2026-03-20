// Copyright Epic Games, Inc. All Rights Reserved.

#include "StellarShaderSubsystem.h"
#include "TestShaderViewExtension.h"

void UStellarShaderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Create and register the view extension (starts disabled)
	ViewExtension = FSceneViewExtensions::NewExtension<FTestShaderViewExtension>();
	ViewExtension->SetEnabled(false);

	UE_LOG(LogTemp, Log, TEXT("[StellarShader] Subsystem initialized. Use ToggleTestShader() to enable the plasma effect."));
}

void UStellarShaderSubsystem::Deinitialize()
{
	ViewExtension.Reset();
	Super::Deinitialize();
}

void UStellarShaderSubsystem::ToggleTestShader()
{
	if (ViewExtension.IsValid())
	{
		const bool bNewState = !ViewExtension->IsEnabled();
		ViewExtension->SetEnabled(bNewState);
		UE_LOG(LogTemp, Log, TEXT("[StellarShader] Test shader %s"), bNewState ? TEXT("ENABLED") : TEXT("DISABLED"));
	}
}

void UStellarShaderSubsystem::SetTestShaderEnabled(bool bEnabled)
{
	if (ViewExtension.IsValid())
	{
		ViewExtension->SetEnabled(bEnabled);
		UE_LOG(LogTemp, Log, TEXT("[StellarShader] Test shader %s"), bEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));
	}
}

bool UStellarShaderSubsystem::IsTestShaderEnabled() const
{
	return ViewExtension.IsValid() && ViewExtension->IsEnabled();
}
