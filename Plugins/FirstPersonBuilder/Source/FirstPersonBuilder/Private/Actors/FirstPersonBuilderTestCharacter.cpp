// Copyright 2026

#include "Actors/FirstPersonBuilderTestCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "Objects/PlayerConstructionObject.h"
#include "Objects/ConstructionSystemSettings.h"
#include "UI/BuilderMenuTheme.h"

AFirstPersonBuilderTestCharacter::AFirstPersonBuilderTestCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// Create a CameraComponent
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(0.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));
}

void AFirstPersonBuilderTestCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AFirstPersonBuilderTestCharacter, ConstructionObject, COND_OwnerOnly);
}

bool AFirstPersonBuilderTestCharacter::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	if (ConstructionObject && RepFlags->bNetOwner)
	{
		WroteSomething |= Channel->ReplicateSubobject(ConstructionObject, *Bunch, *RepFlags);
	}

	return WroteSomething;
}

void AFirstPersonBuilderTestCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Instantiate the construction object on the server and local client
	if (HasAuthority())
	{
		ConstructionObject = NewObject<UPlayerConstructionObject>(this);
	}

	// Local initialization (Ghost, Decal, UI) is done inside the InitializeConstruction function
	// We call it here for both Client and Server just in case, though the Object usually handles its local nature
	if (ConstructionObject)
	{
		ConstructionObject->Settings = ConstructionSettings;
		ConstructionObject->MenuTheme = MenuTheme ? MenuTheme : (ConstructionSettings ? ConstructionSettings->MenuTheme : nullptr);
		ConstructionObject->InitializeConstruction(this);
	}
}

void AFirstPersonBuilderTestCharacter::OnRep_ConstructionObject()
{
	if (ConstructionObject)
	{
		ConstructionObject->Settings = ConstructionSettings;
		ConstructionObject->MenuTheme = MenuTheme ? MenuTheme : (ConstructionSettings ? ConstructionSettings->MenuTheme : nullptr);
		ConstructionObject->InitializeConstruction(this);
	}
}

void AFirstPersonBuilderTestCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Forward tick to the construction object
	if (ConstructionObject)
	{
		ConstructionObject->TickConstruction(DeltaTime);
	}
}

void AFirstPersonBuilderTestCharacter::LeftClick_Pressed()
{
	if (!ConstructionObject)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("LeftClick: ConstructionObject is NULL!"));
		return;
	}

	if (ConstructionObject->bIsBuildModeActive)
	{
		ConstructionObject->RequestPlacement();
	}
	else
	{
		ConstructionObject->RequestHammerHit();
	}
}

void AFirstPersonBuilderTestCharacter::OpenBuildMenu(const TArray<UPlacementPropData*>& Recipes, UBuilderMenuTheme* InMenuStyle)
{
	if (!ConstructionObject) return;
	
	ConstructionObject->AvailableRecipes = Recipes;
	if (InMenuStyle)
	{
		ConstructionObject->MenuTheme = InMenuStyle;
	}
	ConstructionObject->OpenBuilderMenu();
}

void AFirstPersonBuilderTestCharacter::CloseBuildMenu()
{
	if (!ConstructionObject) return;
	
	ConstructionObject->CloseBuilderMenu();
}

void AFirstPersonBuilderTestCharacter::RotateGhost(int32 Direction)
{
	if (!ConstructionObject || !ConstructionObject->bIsBuildModeActive) return;

	ConstructionObject->RotateGhost(Direction);
}
