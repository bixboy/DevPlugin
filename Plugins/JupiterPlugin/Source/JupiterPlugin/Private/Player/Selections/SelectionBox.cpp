#include "Player/Selections/SelectionBox.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Interfaces/Selectable.h"
#include "Kismet/KismetMathLibrary.h"

ASelectionBox::ASelectionBox()
{
	PrimaryActorTick.bCanEverTick = false;

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	BoxComponent->SetBoxExtent(FVector(1.0f, 1.0f, 1.0f));
	BoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BoxComponent->SetCollisionResponseToAllChannels(ECR_Overlap);
	BoxComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RootComponent = BoxComponent;

	BoxComponent->OnComponentBeginOverlap.AddDynamic(this, &ASelectionBox::OnBoxCollisionBeginOverlap);

	Decal = CreateDefaultSubobject<UDecalComponent>(TEXT("Decal"));
	Decal->SetupAttachment(RootComponent);
	
	BoxSelect = false;
}

void ASelectionBox::BeginPlay()
{
	Super::BeginPlay();

	SetActorEnableCollision(false);
	
	if(Decal)
		Decal->SetVisibility(false);
}



void ASelectionBox::Start(FVector Position, const FRotator Rotation)
{
	if(!Decal) return;

	StartLocation = FVector(Position.X, Position.Y, 0.0f);
	StartRotation = FRotator(0.f, Rotation.Yaw, 0.0f);

	SetActorLocation(StartLocation);
	SetActorRotation(StartRotation);
	SetActorEnableCollision(true);
	
	Decal->SetVisibility(true);
	InBox.Empty();
	CenterInBox.Empty();
	BoxSelect = true;
}

void ASelectionBox::UpdateEndLocation(FVector CurrentMouseLocation)
{
	if (!BoxComponent || !Decal)
		return;

	CurrentMouseLocation.Z = 0.0f;
	FVector StartPosFlat = FVector(StartLocation.X, StartLocation.Y, 0.0f);

	FVector NewLocation = UKismetMathLibrary::VLerp(StartPosFlat, CurrentMouseLocation, 0.5f);
	BoxComponent->SetWorldLocation(NewLocation);

	FVector DistVector = StartPosFlat - CurrentMouseLocation;
	DistVector = GetActorRotation().GetInverse().RotateVector(DistVector);
	
	FVector NewExtent = DistVector.GetAbs();
	NewExtent.Z += 100000.f; 
	NewExtent *= 0.5f; 
    
	BoxComponent->SetBoxExtent(NewExtent);
	Decal->DecalSize = FVector(NewExtent.Z, NewExtent.Y, NewExtent.X); 
    
	Manage();
}

TArray<AActor*> ASelectionBox::End()
{
	BoxSelect = false;
	SetActorEnableCollision(false);
	Decal->SetVisibility(false);

    TArray<AActor*> Selected = CenterInBox;

	InBox.Empty();
	CenterInBox.Empty();
    
    return Selected;
}



void ASelectionBox::Manage()
{
	if (!BoxComponent)
		return;

	const FVector BoxExtent = BoxComponent->GetScaledBoxExtent();
	const FVector BoxCenter = BoxComponent->GetComponentLocation(); 

	for (int i = 0; i < InBox.Num(); ++i)
	{
		FVector ActorCenter = InBox[i]->GetActorLocation();

		FVector LocalActorCenter = BoxComponent->GetComponentTransform().InverseTransformPosition(ActorCenter);
		bool bInsideBox = FMath::Abs(LocalActorCenter.X) <= BoxExtent.X && FMath::Abs(LocalActorCenter.Y) <= BoxExtent.Y;
		
		if (bInsideBox)
		{

			if (!CenterInBox.Contains(InBox[i]))
			{
				CenterInBox.Add(InBox[i]);
				HandleHighlight(InBox[i], true);
			}

		}
		else
		{
			CenterInBox.Remove(InBox[i]);
			HandleHighlight(InBox[i], false);
		}
	}
	
}

void ASelectionBox::HandleHighlight(AActor* ActorInBox, const bool Highlight) const
{
	if(ISelectable* Selectable = Cast<ISelectable>(ActorInBox))
		Selectable->Highlight(Highlight);
}

void ASelectionBox::OnBoxCollisionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(!OtherActor || OtherActor == this) return;

	if(ISelectable* Selectable = Cast<ISelectable>(OtherActor))
	{
		InBox.AddUnique(OtherActor);
	}
}

