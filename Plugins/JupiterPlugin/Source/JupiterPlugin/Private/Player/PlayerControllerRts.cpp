#include "Player/PlayerControllerRts.h"
#include "EnhancedInputSubsystems.h"


APlayerControllerRts::APlayerControllerRts(const FObjectInitializer& ObjectInitializer)
{
	//SelectionComponent = CreateDefaultSubobject<USelectionComponent>(TEXT("SelectionComponent"));
}

void APlayerControllerRts::BeginPlay()
{
	Super::BeginPlay();
	
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
	
	//verify((SelectionComponent->AssetManager = UAssetManager::GetIfInitialized()) != nullptr);
	
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;

	// if (SelectionComponent)
	// {
	// 	SelectionComponent->CreateFormationData();
	// 	SelectionComponent->CreateHud();	
	// }
}
