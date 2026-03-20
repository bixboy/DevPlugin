// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SceneViewExtension.h"

class FTestShaderViewExtension : public FSceneViewExtensionBase
{
public:
	FTestShaderViewExtension(const FAutoRegister& AutoRegister);

	// FSceneViewExtensionBase interface
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}

	/**
	 * Draws the fullscreen plasma effect after the entire view family has been rendered.
	 * This is the simplest and most robust injection point for a test shader.
	 */
	virtual void PostRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override;

	virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override;

	void SetEnabled(bool bInEnabled) { bEnabled = bInEnabled; }
	bool IsEnabled() const { return bEnabled; }

private:
	bool bEnabled = true;
	double StartTime = 0.0;
};
