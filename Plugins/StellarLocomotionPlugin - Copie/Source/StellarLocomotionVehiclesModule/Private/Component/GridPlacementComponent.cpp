#include "Component/GridPlacementComponent.h"

#include "Container.h"
#include "ProceduralMeshComponent.h"
#include "Components/BoxComponent.h"

// ==== Setup ====
#pragma region Setup

UGridPlacementComponent::UGridPlacementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	ProcMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GridProcMesh"));
	ProcMesh->SetupAttachment(this);
	ProcMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProcMesh->bUseAsyncCooking = true;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("GridTrigger"));
	TriggerVolume->SetupAttachment(this);
	TriggerVolume->SetCollisionProfileName("OverlapAllDynamic");
	TriggerVolume->SetGenerateOverlapEvents(true);
}

void UGridPlacementComponent::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerVolume)
	{
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &UGridPlacementComponent::OnTriggerBegin);
		TriggerVolume->OnComponentEndOverlap  .AddDynamic(this, &UGridPlacementComponent::OnTriggerEnd);
	}
	
	BuildGridMesh();
}

#if WITH_EDITOR
void UGridPlacementComponent::OnRegister()
{
	Super::OnRegister();
	
	BuildGridMesh();

	if (bEnableRuntimeTrigger && TriggerVolume)
	{
		const float HalfX = GridCountX * CellSize * 0.5f;
		const float HalfY = GridCountY * CellSize * 0.5f;
		const float HalfZ = 500.f;
        
		TriggerVolume->SetBoxExtent(FVector(HalfX, HalfY, HalfZ));
		TriggerVolume->SetRelativeLocation(FVector(0.f, 0.f, HalfZ));
	}
}

void UGridPlacementComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	BuildGridMesh();
}
#endif

#pragma endregion


void UGridPlacementComponent::BuildGridMesh()
{
	if (!bShowGrid || !ProcMesh)
	{
		ProcMesh->ClearAllMeshSections();
		return;
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	auto CreateLine = [&](const FVector& Start, const FVector& End)
	{
		FVector Dir = (End - Start).GetSafeNormal();
		FVector Right = FVector::CrossProduct(Dir, FVector::UpVector);

		FVector P0 = Start + Right * (LineThickness * 0.5f);
		FVector P1 = End   + Right * (LineThickness * 0.5f);
		FVector P2 = Start - Right * (LineThickness * 0.5f);
		FVector P3 = End   - Right * (LineThickness * 0.5f);

		int32 BaseIndex = Vertices.Num();
		Vertices.Append({P0, P1, P2, P3});
		Triangles.Append({
			BaseIndex + 0, BaseIndex + 1, BaseIndex + 2,
			BaseIndex + 2, BaseIndex + 1, BaseIndex + 3
		});
		
		for (int i=0; i<4; i++)
			Normals.Add(FVector::UpVector);
		
		for (int i=0; i<4; i++)
		{
			UV0.Add(FVector2D::ZeroVector);
			VertexColors.Add(FColor::White); Tangents.Add({Dir, false});
		}
	};

	const float TotalX = GridCountX * CellSize;
	const float TotalY = GridCountY * CellSize;
	
	const FTransform& T = GetComponentTransform();
	const float HalfX = TotalX * 0.5f;
	const float HalfY = TotalY * 0.5f;

	for (int32 ix = 0; ix <= GridCountX; ++ix)
	{
		float X = ix * CellSize - HalfX;
		
		FVector LocalStart = FVector(X, -HalfY, GridHeight);
		FVector LocalEnd   = FVector(X,  HalfY, GridHeight);
		
		CreateLine(LocalStart, LocalEnd);
	}
	for (int32 iy = 0; iy <= GridCountY; ++iy)
	{
		float Y = iy * CellSize - HalfY;
		
		FVector LocalStart = FVector(-HalfX, Y, GridHeight);
		FVector LocalEnd   = FVector( HalfX, Y, GridHeight);
		
		CreateLine(LocalStart, LocalEnd);
	}

	ProcMesh->ClearAllMeshSections();
	ProcMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, false);
	
	if (LineMaterial) ProcMesh->SetMaterial(0, LineMaterial);
}


FVector UGridPlacementComponent::GetGridOrigin() const
{
	const FVector WorldLoc = GetComponentLocation();
	return WorldLoc;
}

FVector UGridPlacementComponent::SnapWorldPosition(const FVector& InWorldPos) const
{
	FGridCell Cell = WorldPosToCell(InWorldPos);
	return CellToWorldPos(Cell.X, Cell.Y, Cell.Z);
}


FGridCell UGridPlacementComponent::WorldPosToCell(const FVector& InWorldPos) const
{
	FVector BL = GetBottomLeft();
	FVector Local = InWorldPos - BL;

	int32 X = FMath::FloorToInt(Local.X / CellSize);
	int32 Y = FMath::FloorToInt(Local.Y / CellSize);

	return FGridCell{ X, Y, 0 };
}

FVector UGridPlacementComponent::CellToWorldPos(int32 X, int32 Y, int32 Z) const
{
	FVector BL = GetBottomLeft();

	return BL + FVector( (X + 0.5f) * CellSize,
						 (Y + 0.5f) * CellSize,
						 GridHeight);
}

FVector2D UGridPlacementComponent::GetGridCount() const
{
	return FVector2D(GridCountX, GridCountY);
}

float UGridPlacementComponent::GetGridHeight() const
{
	return  GridHeight;
}

void UGridPlacementComponent::OnTriggerBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (AContainer* C = Cast<AContainer>(OtherActor))
	{
		C->NotifyEnterGrid(this);
	}
}

void UGridPlacementComponent::OnTriggerEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AContainer* C = Cast<AContainer>(OtherActor))
	{
		C->NotifyExitGrid(this);
	}
}
