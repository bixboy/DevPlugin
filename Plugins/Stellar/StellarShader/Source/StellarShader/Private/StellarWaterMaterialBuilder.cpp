// Copyright Epic Games, Inc. All Rights Reserved.

#include "StellarWaterMaterialBuilder.h"

#if WITH_EDITOR

#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "HAL/IConsoleManager.h"
#include "FileHelpers.h"

// ---- Console Command Registration ----
static FAutoConsoleCommand CreateWaterMaterialCmd(
	TEXT("StellarShader.CreateWaterMaterial"),
	TEXT("Creates the StellarWater base material in /StellarShader/Content/"),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		UMaterial* Mat = UStellarWaterMaterialBuilder::CreateWaterMaterial();
		if (Mat)
		{
			UE_LOG(LogTemp, Log, TEXT("[StellarShader] Water material created: %s"), *Mat->GetPathName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[StellarShader] Failed to create water material."));
		}
	})
);

// ---- Helpers ----

namespace StellarWaterMat
{
	// Layout positions for Material Editor graph readability
	static int32 ParamX = -800;
	static int32 CustomX = 0;
	static int32 ParamYStart = -600;
	static int32 ParamYSpacing = 80;

	template<typename T>
	T* CreateExpression(UMaterial* Mat)
	{
		T* Expr = NewObject<T>(Mat);
		Mat->GetExpressionCollection().AddExpression(Expr);
		return Expr;
	}

	UMaterialExpressionScalarParameter* AddScalar(UMaterial* Mat, const FName& Name, float Default, int32& YPos)
	{
		auto* Param = CreateExpression<UMaterialExpressionScalarParameter>(Mat);
		Param->ParameterName = Name;
		Param->DefaultValue = Default;
		Param->MaterialExpressionEditorX = ParamX;
		Param->MaterialExpressionEditorY = YPos;
		YPos += ParamYSpacing;
		return Param;
	}

	UMaterialExpressionVectorParameter* AddVector(UMaterial* Mat, const FName& Name, const FLinearColor& Default, int32& YPos)
	{
		auto* Param = CreateExpression<UMaterialExpressionVectorParameter>(Mat);
		Param->ParameterName = Name;
		Param->DefaultValue = Default;
		Param->MaterialExpressionEditorX = ParamX;
		Param->MaterialExpressionEditorY = YPos;
		YPos += ParamYSpacing;
		return Param;
	}
}

UMaterial* UStellarWaterMaterialBuilder::CreateWaterMaterial()
{
	const FString PackageName = TEXT("/StellarShader/M_StellarWater");
	const FString AssetName = TEXT("M_StellarWater");

	// Try to load existing material to ensure package is fully loaded
	UMaterial* Mat = LoadObject<UMaterial>(nullptr, *PackageName);
	UPackage* Package = nullptr;

	if (Mat)
	{
		Package = Mat->GetOutermost();
		Mat->PreEditChange(nullptr);

		// Clear existing expressions
		Mat->GetExpressionCollection().Empty();
		if (Mat->GetEditorOnlyData())
		{
			Mat->GetEditorOnlyData()->ExpressionCollection.Empty();
		}
	}
	else
	{
		Package = CreatePackage(*PackageName);
		if (!Package)
		{
			UE_LOG(LogTemp, Error, TEXT("[StellarShader] Failed to create package: %s"), *PackageName);
			return nullptr;
		}

		Mat = NewObject<UMaterial>(Package, UMaterial::StaticClass(), FName(*AssetName), RF_Public | RF_Standalone);
	}

	// Unlink all inputs before rebuilding
	if (UMaterialEditorOnlyData* EditorData = Mat->GetEditorOnlyData())
	{
		EditorData->BaseColor.Expression = nullptr;
		EditorData->WorldPositionOffset.Expression = nullptr;
		EditorData->Normal.Expression = nullptr;
		EditorData->Opacity.Expression = nullptr;
		EditorData->Roughness.Expression = nullptr;
		EditorData->EmissiveColor.Expression = nullptr;
	}

	// Material settings for water
	Mat->MaterialDomain = MD_Surface;
	Mat->BlendMode = BLEND_Translucent;
	Mat->SetShadingModel(MSM_DefaultLit);
	Mat->TwoSided = true;
	Mat->TranslucencyLightingMode = TLM_Surface;
	Mat->bTangentSpaceNormal = false; // We output world-space normals directly

	int32 YPos = StellarWaterMat::ParamYStart;

	// ========================================
	// Create parameter expressions
	// ========================================

	// Wave params
	auto* PDeformation      = StellarWaterMat::AddScalar(Mat, FName("DeformationIntensity"), 1.0f, YPos);
	auto* PAmplitude        = StellarWaterMat::AddScalar(Mat, FName("WaveAmplitude"), 100.0f, YPos);
	auto* PWaveLength       = StellarWaterMat::AddScalar(Mat, FName("WaveLength"), 1500.0f, YPos);
	auto* PSpeed            = StellarWaterMat::AddScalar(Mat, FName("WaveSpeed"), 1.0f, YPos);
	auto* PSteepness        = StellarWaterMat::AddScalar(Mat, FName("WaveSteepness"), 0.5f, YPos);
	auto* PDirection        = StellarWaterMat::AddVector(Mat, FName("WaveDirection"), FLinearColor(1, 0, 0, 0), YPos);
	auto* PDirectionality   = StellarWaterMat::AddScalar(Mat, FName("WaveDirectionality"), 0.3f, YPos);
	auto* PNumWaves         = StellarWaterMat::AddScalar(Mat, FName("NumWaves"), 5.0f, YPos);
	auto* PDetailScale      = StellarWaterMat::AddScalar(Mat, FName("DetailWaveScale"), 0.3f, YPos);

	// Color params
	auto* PShallowColor     = StellarWaterMat::AddVector(Mat, FName("ShallowColor"), FLinearColor(0.1f, 0.6f, 0.7f, 1.0f), YPos);
	auto* PDeepColor        = StellarWaterMat::AddVector(Mat, FName("DeepColor"), FLinearColor(0.02f, 0.1f, 0.2f, 1.0f), YPos);
	auto* PDepthFalloff     = StellarWaterMat::AddScalar(Mat, FName("DepthFalloff"), 2.0f, YPos);
	auto* PWaterOpacity     = StellarWaterMat::AddScalar(Mat, FName("WaterOpacity"), 0.85f, YPos);
	auto* PFresnelPower     = StellarWaterMat::AddScalar(Mat, FName("FresnelPower"), 3.0f, YPos);

	// Surface params
	auto* PRoughness        = StellarWaterMat::AddScalar(Mat, FName("Roughness"), 0.02f, YPos);
	auto* PNormalStrength   = StellarWaterMat::AddScalar(Mat, FName("NormalStrength"), 1.0f, YPos);

	// Foam params
	auto* PFoamColor        = StellarWaterMat::AddVector(Mat, FName("FoamColor"), FLinearColor::White, YPos);
	auto* PFoamAmount       = StellarWaterMat::AddScalar(Mat, FName("FoamAmount"), 0.3f, YPos);
	auto* PFoamScale        = StellarWaterMat::AddScalar(Mat, FName("FoamScale"), 2.0f, YPos);
	auto* PFoamSpeed        = StellarWaterMat::AddScalar(Mat, FName("FoamSpeed"), 0.5f, YPos);

	// Caustics params
	auto* PCausticsIntensity = StellarWaterMat::AddScalar(Mat, FName("CausticsIntensity"), 0.3f, YPos);
	auto* PCausticsScale    = StellarWaterMat::AddScalar(Mat, FName("CausticsScale"), 3.0f, YPos);
	auto* PCausticsSpeed    = StellarWaterMat::AddScalar(Mat, FName("CausticsSpeed"), 0.8f, YPos);

	// ========================================
	// World Position (Absolute) node
	// ========================================
	auto* WorldPosExpr = StellarWaterMat::CreateExpression<UMaterialExpressionWorldPosition>(Mat);
	WorldPosExpr->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
	WorldPosExpr->MaterialExpressionEditorX = StellarWaterMat::ParamX - 400;
	WorldPosExpr->MaterialExpressionEditorY = StellarWaterMat::ParamYStart;

	// Component mask for XY
	auto* MaskXY = StellarWaterMat::CreateExpression<UMaterialExpressionComponentMask>(Mat);
	MaskXY->R = true;
	MaskXY->G = true;
	MaskXY->B = false;
	MaskXY->A = false;
	MaskXY->Input.Connect(0, WorldPosExpr);
	MaskXY->MaterialExpressionEditorX = StellarWaterMat::ParamX - 200;
	MaskXY->MaterialExpressionEditorY = StellarWaterMat::ParamYStart;

	// ========================================
	// Time node
	// ========================================
	auto* TimeExpr = StellarWaterMat::CreateExpression<UMaterialExpressionTime>(Mat);
	TimeExpr->MaterialExpressionEditorX = StellarWaterMat::ParamX - 200;
	TimeExpr->MaterialExpressionEditorY = StellarWaterMat::ParamYStart + 80;

	// ========================================
	// Main Custom Expressions (One per property to bypass AdditionalOutputs API bug)
	// ========================================

	auto CreateCustomNode = [&](const FString& Name, ECustomMaterialOutputType OutType, const FString& Code) -> UMaterialExpressionCustom*
	{
		auto* Expr = StellarWaterMat::CreateExpression<UMaterialExpressionCustom>(Mat);
		Expr->MaterialExpressionEditorX = StellarWaterMat::CustomX;
		Expr->MaterialExpressionEditorY = StellarWaterMat::ParamYStart;
		Expr->OutputType = OutType;
		Expr->Description = Name;
		Expr->IncludeFilePaths.Add(TEXT("/StellarShader/Private/StellarWater.ush"));
		
		auto AddInput = [&](const FName& InName, UMaterialExpression* Source) {
			FCustomInput Input; Input.InputName = InName; Input.Input.Connect(0, Source);
			Expr->Inputs.Add(Input);
		};

		AddInput(FName("WorldPosXY"), MaskXY);
		AddInput(FName("Time"), TimeExpr);
		AddInput(FName("DeformationIntensity"), PDeformation);
		AddInput(FName("WaveAmplitude"), PAmplitude);
		AddInput(FName("WaveLength"), PWaveLength);
		AddInput(FName("WaveSpeed"), PSpeed);
		AddInput(FName("WaveSteepness"), PSteepness);
		AddInput(FName("WaveDirection"), PDirection);
		AddInput(FName("WaveDirectionality"), PDirectionality);
		AddInput(FName("NumWaves"), PNumWaves);
		AddInput(FName("DetailWaveScale"), PDetailScale);
		AddInput(FName("ShallowColor"), PShallowColor);
		AddInput(FName("DeepColor"), PDeepColor);
		AddInput(FName("DepthFalloff"), PDepthFalloff);
		AddInput(FName("WaterOpacity"), PWaterOpacity);
		AddInput(FName("FresnelPower"), PFresnelPower);
		AddInput(FName("InRoughness"), PRoughness);
		AddInput(FName("NormalStrength"), PNormalStrength);
		AddInput(FName("FoamColor"), PFoamColor);
		AddInput(FName("FoamAmount"), PFoamAmount);
		AddInput(FName("FoamScale"), PFoamScale);
		AddInput(FName("FoamSpeed"), PFoamSpeed);
		AddInput(FName("CausticsIntensity"), PCausticsIntensity);
		AddInput(FName("CausticsScale"), PCausticsScale);
		AddInput(FName("CausticsSpeed"), PCausticsSpeed);

		Expr->Code = Code;
		return Expr;
	};

	// 1. WPO
	FString CodeWPO = TEXT(
		"float BaseWl = max(WaveLength, 10.0);\n"
		"int Nw = clamp((int)NumWaves, 1, 12);\n"
		"return MultiGerstnerWaves(WorldPosXY, Time, WaveAmplitude, BaseWl, WaveSpeed, WaveSteepness, WaveDirection.rg, WaveDirectionality, Nw, DetailWaveScale) * DeformationIntensity;\n"
	);
	auto* NodeWPO = CreateCustomNode(TEXT("Water WPO"), CMOT_Float3, CodeWPO);

	// 2. Normal
	FString CodeNormal = TEXT(
		"float BaseWl = max(WaveLength, 10.0);\n"
		"int Nw = clamp((int)NumWaves, 1, 12);\n"
		"return MultiGerstnerNormal(WorldPosXY, Time, WaveAmplitude, BaseWl, WaveSpeed, WaveSteepness, WaveDirection.rg, WaveDirectionality, Nw, NormalStrength);\n"
	);
	auto* NodeNormal = CreateCustomNode(TEXT("Water Normal"), CMOT_Float3, CodeNormal);

	// 3. BaseColor
	FString CodeColor = TEXT(
		"float BaseWl = max(WaveLength, 10.0);\n"
		"int Nw = clamp((int)NumWaves, 1, 12);\n"
		"float3 wOffset = MultiGerstnerWaves(WorldPosXY, Time, WaveAmplitude, BaseWl, WaveSpeed, WaveSteepness, WaveDirection.rg, WaveDirectionality, Nw, DetailWaveScale);\n"
		"float foam = CalculateFoam(WorldPosXY, Time, wOffset.z, WaveAmplitude, FoamAmount, FoamScale, FoamSpeed);\n"
		"float depthF = saturate((wOffset.z / max(WaveAmplitude, 0.01) + 1.0) * 0.5);\n"
		"depthF = pow(depthF, DepthFalloff);\n"
		"float3 c = lerp(DeepColor.rgb, ShallowColor.rgb, depthF);\n"
		"return lerp(c, FoamColor.rgb, foam);\n"
	);
	auto* NodeColor = CreateCustomNode(TEXT("Water Color"), CMOT_Float3, CodeColor);

	// 4. Opacity
	FString CodeOpacity = TEXT(
		"float BaseWl = max(WaveLength, 10.0);\n"
		"int Nw = clamp((int)NumWaves, 1, 12);\n"
		"float3 wOffset = MultiGerstnerWaves(WorldPosXY, Time, WaveAmplitude, BaseWl, WaveSpeed, WaveSteepness, WaveDirection.rg, WaveDirectionality, Nw, DetailWaveScale);\n"
		"float foam = CalculateFoam(WorldPosXY, Time, wOffset.z, WaveAmplitude, FoamAmount, FoamScale, FoamSpeed);\n"
		"return lerp(WaterOpacity, 1.0, foam * 0.7);\n"
	);
	auto* NodeOpacity = CreateCustomNode(TEXT("Water Opacity"), CMOT_Float1, CodeOpacity);

	// 5. Roughness
	FString CodeRoughness = TEXT(
		"float BaseWl = max(WaveLength, 10.0);\n"
		"int Nw = clamp((int)NumWaves, 1, 12);\n"
		"float3 wOffset = MultiGerstnerWaves(WorldPosXY, Time, WaveAmplitude, BaseWl, WaveSpeed, WaveSteepness, WaveDirection.rg, WaveDirectionality, Nw, DetailWaveScale);\n"
		"float foam = CalculateFoam(WorldPosXY, Time, wOffset.z, WaveAmplitude, FoamAmount, FoamScale, FoamSpeed);\n"
		"return lerp(InRoughness, 0.6, foam);\n"
	);
	auto* NodeRoughness = CreateCustomNode(TEXT("Water Roughness"), CMOT_Float1, CodeRoughness);

	// 6. Emissive / Caustics
	FString CodeEmissive = TEXT(
		"float BaseWl = max(WaveLength, 10.0);\n"
		"int Nw = clamp((int)NumWaves, 1, 12);\n"
		"float3 wOffset = MultiGerstnerWaves(WorldPosXY, Time, WaveAmplitude, BaseWl, WaveSpeed, WaveSteepness, WaveDirection.rg, WaveDirectionality, Nw, DetailWaveScale);\n"
		"float foam = CalculateFoam(WorldPosXY, Time, wOffset.z, WaveAmplitude, FoamAmount, FoamScale, FoamSpeed);\n"
		"float depthF = saturate((wOffset.z / max(WaveAmplitude, 0.01) + 1.0) * 0.5);\n"
		"float3 c = lerp(DeepColor.rgb, ShallowColor.rgb, pow(depthF, DepthFalloff));\n"
		"float caustics = CalculateCaustics(WorldPosXY, Time, CausticsScale, CausticsSpeed, CausticsIntensity);\n"
		"return lerp(c, FoamColor.rgb, foam) * caustics * 0.5;\n"
	);
	auto* NodeEmissive = CreateCustomNode(TEXT("Water Emissive"), CMOT_Float3, CodeEmissive);

	// ========================================
	// Connect Custom Expression outputs to Material pins
	// ========================================
	if (UMaterialEditorOnlyData* EditorData = Mat->GetEditorOnlyData())
	{
		EditorData->BaseColor.Connect(0, NodeColor);
		EditorData->WorldPositionOffset.Connect(0, NodeWPO);
		EditorData->Normal.Connect(0, NodeNormal);
		EditorData->Opacity.Connect(0, NodeOpacity);
		EditorData->Roughness.Connect(0, NodeRoughness);
		EditorData->EmissiveColor.Connect(0, NodeEmissive);
	}

	// Metallic = 0 (water is dielectric)
	// Specular is handled by Fresnel in the shader

	// ========================================
	// Compile and save
	// ========================================
	Mat->PreEditChange(nullptr);
	Mat->PostEditChange();

	// Mark package dirty and save
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Mat);

	const FString PackageFilename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	const bool bSaved = UPackage::SavePackage(Package, Mat, *PackageFilename, FSavePackageArgs());

	if (bSaved)
	{
		UE_LOG(LogTemp, Log, TEXT("[StellarShader] Material saved to: %s"), *PackageFilename);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[StellarShader] Material created but could not be saved to disk. Save it manually."));
	}

	return Mat;
}

#endif // WITH_EDITOR
