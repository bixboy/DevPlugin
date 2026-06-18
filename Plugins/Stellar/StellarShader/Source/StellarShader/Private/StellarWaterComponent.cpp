// Copyright Epic Games, Inc. All Rights Reserved.

#include "StellarWaterComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/MeshComponent.h"

UStellarWaterComponent::UStellarWaterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStellarWaterComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeMaterial();
}

void UStellarWaterComponent::InitializeMaterial()
{
	if (!IsValid(BaseMaterial))
	{
		UE_LOG(LogTemp, Warning, TEXT("[StellarWater] No BaseMaterial set. Generate one via console: StellarShader.CreateWaterMaterial"));
		return;
	}

	// Auto-detect mesh if not set
	if (!IsValid(TargetMesh))
	{
		AActor* Owner = GetOwner();
		if (IsValid(Owner))
		{
			TargetMesh = Owner->FindComponentByClass<UMeshComponent>();
		}
	}

	if (!IsValid(TargetMesh))
	{
		UE_LOG(LogTemp, Warning, TEXT("[StellarWater] No target mesh found on owner actor."));
		return;
	}

	// Create MID
	WaterMID = UMaterialInstanceDynamic::Create(BaseMaterial, this, FName("StellarWaterMID"));
	if (!IsValid(WaterMID))
	{
		UE_LOG(LogTemp, Error, TEXT("[StellarWater] Failed to create MaterialInstanceDynamic."));
		return;
	}

	TargetMesh->SetMaterial(0, WaterMID);
	SyncAllParameters();

	UE_LOG(LogTemp, Log, TEXT("[StellarWater] Water material applied successfully."));
}

void UStellarWaterComponent::ApplyParameters()
{
	SyncAllParameters();
}

void UStellarWaterComponent::SyncAllParameters()
{
	if (!IsValid(WaterMID))
	{
		return;
	}

	// Wave parameters
	WaterMID->SetScalarParameterValue(FName("DeformationIntensity"), DeformationIntensity);
	WaterMID->SetScalarParameterValue(FName("WaveAmplitude"), WaveAmplitude);
	WaterMID->SetScalarParameterValue(FName("WaveLength"), WaveLength);
	WaterMID->SetScalarParameterValue(FName("WaveSpeed"), WaveSpeed);
	WaterMID->SetScalarParameterValue(FName("WaveSteepness"), WaveSteepness);
	WaterMID->SetVectorParameterValue(FName("WaveDirection"), FLinearColor(WaveDirection.X, WaveDirection.Y, 0.0f, 0.0f));
	WaterMID->SetScalarParameterValue(FName("WaveDirectionality"), WaveDirectionality);
	WaterMID->SetScalarParameterValue(FName("NumWaves"), static_cast<float>(NumWaves));
	WaterMID->SetScalarParameterValue(FName("DetailWaveScale"), DetailWaveScale);

	// Color parameters
	WaterMID->SetVectorParameterValue(FName("ShallowColor"), ShallowColor);
	WaterMID->SetVectorParameterValue(FName("DeepColor"), DeepColor);
	WaterMID->SetScalarParameterValue(FName("DepthFalloff"), DepthFalloff);
	WaterMID->SetScalarParameterValue(FName("WaterOpacity"), WaterOpacity);
	WaterMID->SetScalarParameterValue(FName("FresnelPower"), FresnelPower);

	// Surface parameters
	WaterMID->SetScalarParameterValue(FName("Roughness"), Roughness);
	WaterMID->SetScalarParameterValue(FName("NormalStrength"), NormalStrength);

	// Foam parameters
	WaterMID->SetVectorParameterValue(FName("FoamColor"), FoamColor);
	WaterMID->SetScalarParameterValue(FName("FoamAmount"), FoamAmount);
	WaterMID->SetScalarParameterValue(FName("FoamScale"), FoamScale);
	WaterMID->SetScalarParameterValue(FName("FoamSpeed"), FoamSpeed);

	// Caustics parameters
	WaterMID->SetScalarParameterValue(FName("CausticsIntensity"), CausticsIntensity);
	WaterMID->SetScalarParameterValue(FName("CausticsScale"), CausticsScale);
	WaterMID->SetScalarParameterValue(FName("CausticsSpeed"), CausticsSpeed);
}

#if WITH_EDITOR
void UStellarWaterComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Re-init material if base material changed
	const FName PropName = PropertyChangedEvent.GetMemberPropertyName();
	if (PropName == GET_MEMBER_NAME_CHECKED(UStellarWaterComponent, BaseMaterial)
		|| PropName == GET_MEMBER_NAME_CHECKED(UStellarWaterComponent, TargetMesh))
	{
		InitializeMaterial();
	}
	else
	{
		SyncAllParameters();
	}
}
#endif
