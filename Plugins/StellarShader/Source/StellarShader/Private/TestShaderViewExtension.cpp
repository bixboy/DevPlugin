// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestShaderViewExtension.h"
#include "TestGlobalShader.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "CommonRenderResources.h"
#include "HAL/PlatformTime.h"
#include "SceneView.h"
#include "RHIStaticStates.h"

FTestShaderViewExtension::FTestShaderViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
	, StartTime(FPlatformTime::Seconds())
{
}

bool FTestShaderViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
	return bEnabled;
}

void FTestShaderViewExtension::PostRenderViewFamily_RenderThread(
	FRDGBuilder& GraphBuilder,
	FSceneViewFamily& InViewFamily)
{
	if (InViewFamily.Views.Num() == 0)
	{
		return;
	}

	const FSceneView* View = InViewFamily.Views[0];
	if (!View)
	{
		return;
	}

	const FRenderTarget* RenderTarget = InViewFamily.RenderTarget;
	if (!RenderTarget)
	{
		return;
	}

	FRHITexture* RHITexture = RenderTarget->GetRenderTargetTexture();
	if (!RHITexture)
	{
		return;
	}

	const FIntRect ViewportRect = View->UnscaledViewRect;
	const FVector2f Resolution(
		static_cast<float>(ViewportRect.Width()),
		static_cast<float>(ViewportRect.Height())
	);
	const float ElapsedTime = static_cast<float>(FPlatformTime::Seconds() - StartTime);

	// Keep the pooled RT alive through RDG execution
	TRefCountPtr<IPooledRenderTarget> PooledBackBuffer = CreateRenderTarget(RHITexture, TEXT("StellarShaderBB"));
	FRDGTextureRef BackBuffer = GraphBuilder.RegisterExternalTexture(PooledBackBuffer);

	// Setup pixel shader parameters
	FTestShaderPS::FParameters* PSParams = GraphBuilder.AllocParameters<FTestShaderPS::FParameters>();
	PSParams->TestShaderTime = ElapsedTime;
	PSParams->TestShaderResolution = Resolution;
	PSParams->RenderTargets[0] = FRenderTargetBinding(BackBuffer, ERenderTargetLoadAction::ELoad);

	const FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	TShaderMapRef<FTestShaderVS> VertexShader(ShaderMap);
	TShaderMapRef<FTestShaderPS> PixelShader(ShaderMap);

	if (!VertexShader.IsValid() || !PixelShader.IsValid())
	{
		return;
	}

	// NeverCull prevents RDG from optimizing away this pass (backbuffer has no RDG consumers)
	GraphBuilder.AddPass(
		RDG_EVENT_NAME("StellarShader::TestPlasma"),
		PSParams,
		ERDGPassFlags::Raster | ERDGPassFlags::NeverCull,
		[VertexShader, PixelShader, PSParams, ViewportRect](FRHICommandList& RHICmdList)
		{
			RHICmdList.SetViewport(
				static_cast<float>(ViewportRect.Min.X),
				static_cast<float>(ViewportRect.Min.Y),
				0.0f,
				static_cast<float>(ViewportRect.Max.X),
				static_cast<float>(ViewportRect.Max.Y),
				1.0f
			);

			FGraphicsPipelineStateInitializer GraphicsPSOInit;
			RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

			GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
			GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
			GraphicsPSOInit.PrimitiveType = PT_TriangleList;

			GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GEmptyVertexDeclaration.VertexDeclarationRHI;
			GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();

			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

			SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PSParams);

			// Fullscreen triangle: 3 vertices from SV_VertexID, no vertex buffer
			RHICmdList.DrawPrimitive(0, 1, 1);
		}
	);
}
