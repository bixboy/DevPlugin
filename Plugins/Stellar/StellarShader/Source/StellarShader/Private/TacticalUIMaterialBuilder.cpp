// Copyright Epic Games, Inc. All Rights Reserved.

#include "TacticalUIMaterialBuilder.h"

#if WITH_EDITOR

#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "HAL/IConsoleManager.h"
#include "FileHelpers.h"

// Register Console Command
static FAutoConsoleCommand CreateTacticalUIMaterialCmd(
	TEXT("StellarShader.CreateTacticalUIMaterial"),
	TEXT("Creates the High-Fidelity Tactical UI base material in /StellarShader/ Content"),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		UMaterial* Mat = UTacticalUIMaterialBuilder::CreateTacticalMaterial();
		if (Mat)
		{
			UE_LOG(LogTemp, Log, TEXT("[StellarShader] Tactical UI material created: %s"), *Mat->GetPathName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[StellarShader] Failed to create tactical UI material."));
		}
	})
);

namespace TacticalUIMat
{
	static int32 ParamX = -800;
	static int32 CustomX = 0;
	static int32 ParamYStart = -200;
	static int32 ParamYSpacing = 100;

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

UMaterial* UTacticalUIMaterialBuilder::CreateTacticalMaterial()
{
	const FString PackageName = TEXT("/StellarShader/M_Tactical_Frame");
	const FString AssetName = TEXT("M_Tactical_Frame");

	UMaterial* Mat = LoadObject<UMaterial>(nullptr, *PackageName);
	UPackage* Package = nullptr;

	if (Mat)
	{
		Package = Mat->GetOutermost();
		Mat->PreEditChange(nullptr);
		Mat->GetExpressionCollection().Empty();
	}
	else
	{
		Package = CreatePackage(*PackageName);
		if (!Package) return nullptr;
		Mat = NewObject<UMaterial>(Package, UMaterial::StaticClass(), FName(*AssetName), RF_Public | RF_Standalone);
	}

	// Material settings for high-fidelity UI
	Mat->MaterialDomain = MD_UI;
	Mat->BlendMode = BLEND_Translucent;

	int32 YPos = TacticalUIMat::ParamYStart;

	// 1. Create Parameters
	auto* PBgColor       = TacticalUIMat::AddVector(Mat, FName("BgColor"), FLinearColor(0.04f, 0.05f, 0.06f, 0.98f), YPos);
	auto* PAccentColor   = TacticalUIMat::AddVector(Mat, FName("AccentColor"), FLinearColor(0.0f, 0.75f, 0.95f, 1.0f), YPos);
	auto* PScanSpeed     = TacticalUIMat::AddScalar(Mat, FName("ScanSpeed"), 2.5f, YPos);
	auto* PGlowIntensity = TacticalUIMat::AddScalar(Mat, FName("GlowIntensity"), 1.2f, YPos);
	auto* PHoverState    = TacticalUIMat::AddScalar(Mat, FName("HoverState"), 0.0f, YPos);
	auto* PCornerSize    = TacticalUIMat::AddScalar(Mat, FName("CornerSize"), 14.0f, YPos);
	auto* PCornerThick   = TacticalUIMat::AddScalar(Mat, FName("CornerThickness"), 1.5f, YPos);

	// 2. Create Global Inputs
	auto* TimeExpr = TacticalUIMat::CreateExpression<UMaterialExpressionTime>(Mat);
	TimeExpr->MaterialExpressionEditorX = TacticalUIMat::ParamX;
	TimeExpr->MaterialExpressionEditorY = YPos; YPos += 80;

	auto* UVExpr = TacticalUIMat::CreateExpression<UMaterialExpressionTextureCoordinate>(Mat);
	UVExpr->MaterialExpressionEditorX = TacticalUIMat::ParamX;
	UVExpr->MaterialExpressionEditorY = YPos;

	// 3. Create Main Custom Node
	auto* CustomExpr = TacticalUIMat::CreateExpression<UMaterialExpressionCustom>(Mat);
	CustomExpr->MaterialExpressionEditorX = TacticalUIMat::CustomX;
	CustomExpr->MaterialExpressionEditorY = TacticalUIMat::ParamYStart;
	CustomExpr->OutputType = CMOT_Float4; // We need both Color and Opacity
	CustomExpr->Description = TEXT("StellarTacticalNode");
	CustomExpr->IncludeFilePaths.Add(TEXT("/StellarShader/Private/TacticalUI.ush"));
	
	// Add Inputs to Custom Node
	auto SetInput = [&](const FName& Name, UMaterialExpression* Expr) {
		FCustomInput Input; Input.InputName = Name; Input.Input.Connect(0, Expr);
		CustomExpr->Inputs.Add(Input);
	};

	SetInput(FName("UV"), UVExpr);
	SetInput(FName("Time"), TimeExpr);
	SetInput(FName("BgColor"), PBgColor);
	SetInput(FName("AccentColor"), PAccentColor);
	SetInput(FName("ScanSpeed"), PScanSpeed);
	SetInput(FName("GlowIntensity"), PGlowIntensity);
	SetInput(FName("HoverState"), PHoverState);
	SetInput(FName("CornerSize"), PCornerSize);
	SetInput(FName("CornerThickness"), PCornerThick);

	// HLSL Code to call the main entry point from USH
	CustomExpr->Code = TEXT(
		"float3 outC; float outO;\n"
		"StellarTactical_Main(UV, Time, BgColor.rgb, AccentColor.rgb, ScanSpeed, GlowIntensity, HoverState, CornerSize, CornerThickness, outC, outO);\n"
		"return float4(outC, outO);"
	);

	// 4. Connect to Final Material Slots
	if (UMaterialEditorOnlyData* EditorData = Mat->GetEditorOnlyData())
	{
		// Mask for RGB (Emissive)
		auto* MaskRGB = TacticalUIMat::CreateExpression<UMaterialExpressionComponentMask>(Mat);
		MaskRGB->R = MaskRGB->G = MaskRGB->B = true;
		MaskRGB->A = false;
		MaskRGB->Input.Connect(0, CustomExpr);
		MaskRGB->MaterialExpressionEditorX = TacticalUIMat::CustomX + 200;
		MaskRGB->MaterialExpressionEditorY = TacticalUIMat::ParamYStart;

		// Mask for Alpha (Opacity)
		auto* MaskA = TacticalUIMat::CreateExpression<UMaterialExpressionComponentMask>(Mat);
		MaskA->R = MaskA->G = MaskA->B = false;
		MaskA->A = true;
		MaskA->Input.Connect(0, CustomExpr);
		MaskA->MaterialExpressionEditorX = TacticalUIMat::CustomX + 200;
		MaskA->MaterialExpressionEditorY = TacticalUIMat::ParamYStart + 100;

		EditorData->EmissiveColor.Connect(0, MaskRGB);
		EditorData->Opacity.Connect(0, MaskA);
	}

	Mat->PostEditChange();
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Mat);

	const FString PackageFilename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	UPackage::SavePackage(Package, Mat, *PackageFilename, FSavePackageArgs());

	return Mat;
}

#endif // WITH_EDITOR
