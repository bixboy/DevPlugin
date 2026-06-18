// Copyright Epic Games, Inc. All Rights Reserved.

#include "StellarWaterActor.h"
#include "StellarWaterComponent.h"
#include "ProceduralMeshComponent.h" // Added for UProceduralMeshComponent

AStellarWaterActor::AStellarWaterActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Tessellated procedural mesh for water
	WaterMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WaterMesh"));
	RootComponent = WaterMesh;
	WaterMesh->CastShadow = false;

	// Water component with all shader parameters
	WaterComponent = CreateDefaultSubobject<UStellarWaterComponent>(TEXT("WaterComponent"));
	WaterComponent->TargetMesh = WaterMesh;
}

void AStellarWaterActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!WaterMesh) return;

	// Delete old mesh
	WaterMesh->ClearAllMeshSections();

	// Calculate grid dimensions based on user parameters
	int32 GridSizeX = FMath::Max(1, FMath::RoundToInt(WaterSizeX / FMath::Max(10.0f, VertexDistance)));
	int32 GridSizeY = FMath::Max(1, FMath::RoundToInt(WaterSizeY / FMath::Max(10.0f, VertexDistance)));

	// Safety cap to prevent freezing the Editor with accidental millions of polygons
	const int32 MaxGridSize = 500; // 500x500 = 250,000 vertices max
	if (GridSizeX > MaxGridSize) GridSizeX = MaxGridSize;
	if (GridSizeY > MaxGridSize) GridSizeY = MaxGridSize;

	const float RealSizeX = GridSizeX * VertexDistance;
	const float RealSizeY = GridSizeY * VertexDistance;

	const float StartX = -RealSizeX / 2.0f;
	const float StartY = -RealSizeY / 2.0f;

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	// Generate Vertices
	for (int32 Y = 0; Y <= GridSizeY; Y++)
	{
		for (int32 X = 0; X <= GridSizeX; X++)
		{
			Vertices.Add(FVector(StartX + (X * VertexDistance), StartY + (Y * VertexDistance), 0.0f));
			Normals.Add(FVector(0, 0, 1));
			UV0.Add(FVector2D(X / (float)GridSizeX, Y / (float)GridSizeY));
			VertexColors.Add(FLinearColor::White);
			Tangents.Add(FProcMeshTangent(1, 0, 0));
		}
	}

	// Generate Triangles
	for (int32 Y = 0; Y < GridSizeY; Y++)
	{
		for (int32 X = 0; X < GridSizeX; X++)
		{
			int32 BottomLeft = (Y * (GridSizeX + 1)) + X;
			int32 BottomRight = BottomLeft + 1;
			int32 TopLeft = ((Y + 1) * (GridSizeX + 1)) + X;
			int32 TopRight = TopLeft + 1;

			// Triangle 1
			Triangles.Add(BottomLeft);
			Triangles.Add(TopLeft);
			Triangles.Add(BottomRight);

			// Triangle 2
			Triangles.Add(BottomRight);
			Triangles.Add(TopLeft);
			Triangles.Add(TopRight);
		}
	}

	WaterMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, true);
	
	// Apply the material if it exists
	if (IsValid(WaterComponent) && IsValid(WaterComponent->BaseMaterial))
	{
		WaterComponent->InitializeMaterial();
	}
}
