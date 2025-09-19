#include "Container.h"
#include "Component/GridPlacementComponent.h"
#include "Component/ObjectPreviewComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"


AContainer::AContainer()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);

	ContainerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ContainerMesh"));
	ContainerMesh->SetupAttachment(CollisionBox);
	
}

void AContainer::BeginPlay()
{
	Super::BeginPlay();
	
	PC = Cast<APlayerController>( UGameplayStatics::GetPlayerController(GetWorld(), 0));
	
	if (PC)
		PreviewComp = PC->FindComponentByClass<UObjectPreviewComponent>();
}

void AContainer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!PC || !ContainerMesh || !PreviewComp)
		return;

	ENetMode Mode = PC->GetNetMode();
	if (Mode == NM_DedicatedServer)
		return;
	
	if (ActiveGrids.Num() == 0)
	{
		PreviewComp->ChangePreviewMesh(nullptr);
		PreviousCell = {-1,-1,-1};
		
		return;
	}

	UGridPlacementComponent* BestGrid = nullptr;
	float BestD2 = TNumericLimits<float>::Max();
	FGridCell BestCell = {};

	const FVector MyLoc = GetActorLocation();
	for (UGridPlacementComponent* Grid : ActiveGrids)
	{
		FGridCell Cell = Grid->WorldPosToCell(MyLoc);
		FVector2D Count = Grid->GetGridCount();
		
		if (Cell.X < 0 || Cell.X >= Count.X || Cell.Y < 0 || Cell.Y >= Count.Y)
			continue;
		
		FVector Center = Grid->CellToWorldPos(Cell.X, Cell.Y, 0);
		Center.Z = Grid->GetGridOrigin().Z + Grid->GetGridHeight();
		
		float D2 = FVector::DistSquared(MyLoc, Center);
		if (D2 < BestD2)
		{
			BestD2 = D2;
			BestGrid = Grid;
			BestCell = Cell;
		}
	}

	if (!BestGrid)
	{
		PreviewComp->ChangePreviewMesh(nullptr);
		PreviousCell = {-1,-1,-1};
		
		return;
	}

	if (BestCell.X == PreviousCell.X && BestCell.Y == PreviousCell.Y)
		return;

	PreviousCell = BestCell;

	FVector CellCenter = BestGrid->CellToWorldPos(BestCell.X, BestCell.Y, 0);
	CellCenter.Z = BestGrid->GetGridOrigin().Z + BestGrid->GetGridHeight();
	
	FTransform NewTransform;
	NewTransform.SetLocation(CellCenter);
	NewTransform.SetRotation({});
	NewTransform.SetScale3D(GetActorScale3D());
	
	PreviewComp->SetPreviewTransform(NewTransform);
	PreviewComp->ChangePreviewMesh(ContainerMesh->GetStaticMesh());
}

void AContainer::NotifyEnterGrid(UGridPlacementComponent* Grid)
{
	ActiveGrids.Add(Grid);
}

void AContainer::NotifyExitGrid(UGridPlacementComponent* Grid)
{
	ActiveGrids.Remove(Grid);
}

