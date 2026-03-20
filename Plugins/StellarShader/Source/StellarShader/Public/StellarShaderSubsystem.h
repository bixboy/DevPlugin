// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Subsystems/EngineSubsystem.h"
#include "StellarShaderSubsystem.generated.h"

class FTestShaderViewExtension;

/**
 * UStellarShaderSubsystem
 * Engine subsystem that manages the test shader view extension.
 * Use ToggleTestShader() to enable/disable the fullscreen plasma effect.
 */
UCLASS()
class STELLARSHADER_API UStellarShaderSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Toggle the fullscreen test shader effect on/off */
	UFUNCTION(BlueprintCallable, Category = "StellarShader")
	void ToggleTestShader();

	/** Enable or disable the test shader explicitly */
	UFUNCTION(BlueprintCallable, Category = "StellarShader")
	void SetTestShaderEnabled(bool bEnabled);

	/** Returns true if the test shader effect is currently active */
	UFUNCTION(BlueprintPure, Category = "StellarShader")
	bool IsTestShaderEnabled() const;

private:
	TSharedPtr<FTestShaderViewExtension, ESPMode::ThreadSafe> ViewExtension;
};
