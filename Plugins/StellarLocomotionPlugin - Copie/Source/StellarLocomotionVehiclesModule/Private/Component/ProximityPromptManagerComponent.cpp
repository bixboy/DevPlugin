#include "Component/ProximityPromptManagerComponent.h"
#include "Component/ProximityPromptComponent.h"


UProximityPromptManagerComponent::UProximityPromptManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UProximityPromptManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	
	Player = Cast<APawn>(GetOwner());
	if (!Player)
		Player = Cast<AActor>(GetOwner());
	
	if (Player)
		PC = Cast<APlayerController>(Player->GetInstigatorController());
}

void UProximityPromptManagerComponent::RegisterPrompt(UProximityPromptComponent* Prompt)
{
	if (!Prompt || AllPrompts.Contains(Prompt) || !PC)
		return;

	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, "Proximity prompt 2");
	
	AllPrompts.Add(Prompt);
	Server_RegisterPrompt(Prompt);

	UInputAction* IA = Prompt->InteractInputAction;
	if (!IA || BoundActions.Contains(IA))
		return;

	if (UEnhancedInputComponent* EI = Cast<UEnhancedInputComponent>(PC->InputComponent))
	{
		EI->BindAction(IA, ETriggerEvent::Started, this, &UProximityPromptManagerComponent::OnAnyPromptAction);
		BoundActions.Add(IA);
	}
}

void UProximityPromptManagerComponent::Server_RegisterPrompt_Implementation(UProximityPromptComponent* Prompt)
{
	if (!Prompt || AllPrompts.Contains(Prompt))
		return;

	AllPrompts.Add(Prompt);
}

void UProximityPromptManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ENetMode Mode = GetOwner()->GetNetMode();
	if (Mode == NM_DedicatedServer || !Player || !PC)
		return;
	
	const FVector PlayerPos  = Player->GetActorLocation();

	static FVector2D ScreenCenter = [](){
		FVector2D VS; GEngine->GameViewport->GetViewportSize(VS);
		return VS * 0.5f;
	}();

	UProximityPromptComponent* BestPrompt = nullptr;
	float BestDist2  = FLT_MAX;

	for (UProximityPromptComponent* P : AllPrompts)
	{
		if (!P || !P->IsPromptEnabled()) 
			continue;

		float D2 = FVector::DistSquared(PlayerPos, P->GetPromptLocation());
		float R2 = FMath::Square(P->GetActivationDistance());
		if (D2 > R2) 
			continue;

		FVector2D ScreenPos;
		if (!PC->ProjectWorldLocationToScreen(P->GetPromptLocation(), ScreenPos))
			continue;

		float dx = ScreenPos.X - ScreenCenter.X;
		float dy = ScreenPos.Y - ScreenCenter.Y;
		float Dist2 = dx*dx + dy*dy;

		if (Dist2 < BestDist2)
		{
			BestDist2  = Dist2;
			BestPrompt = P;
		}
	}

	int32 PromptIndex = -1;
	AActor* PromptOwner = nullptr;
	
	if (BestPrompt)
	{
		PromptOwner = BestPrompt->GetOwner();
		TArray<UProximityPromptComponent*> Prompts;
		
		PromptOwner->GetComponents<UProximityPromptComponent>(Prompts);
		PromptIndex = Prompts.IndexOfByKey(BestPrompt);
	}
	
	Server_NotifyBestPrompt(PromptOwner, PromptIndex);
}

void UProximityPromptManagerComponent::Server_NotifyBestPrompt_Implementation(AActor* PromptOwner, int32 PromptIndex)
{

	UProximityPromptComponent* ValidPrompt = nullptr;
	if (PromptOwner && PromptIndex >= 0)
	{
		TArray<UProximityPromptComponent*> Prompts;
		PromptOwner->GetComponents<UProximityPromptComponent>(Prompts);

		if (Prompts.IsValidIndex(PromptIndex))
		{
			UProximityPromptComponent* Candidate = Prompts[PromptIndex];

			const float Dist2 = FVector::DistSquared(Player->GetActorLocation(), Candidate->GetPromptLocation());
			
			if (Candidate->IsPromptEnabled() && Dist2 <= FMath::Square(Candidate->GetActivationDistance()))
			{
				ValidPrompt = Candidate;
			}
		}
	}

	if (ValidPrompt)
	{
		AActor* Owner = ValidPrompt->GetOwner();
		TArray<UProximityPromptComponent*> Prompts;
		
		Owner->GetComponents<UProximityPromptComponent>(Prompts);
		int32 ValidIndex = Prompts.IndexOfByKey(ValidPrompt);

		if (ValidIndex != INDEX_NONE)
		{
			Client_SetPrompt(Owner, ValidIndex);
		}
	}
	else
	{
		Client_SetPrompt(nullptr, -1);
	}
	
}

void UProximityPromptManagerComponent::Client_SetPrompt_Implementation(AActor* PromptOwner, int32 PromptIndex)
{
	UProximityPromptComponent* NewPrompt = nullptr;
	
	if (PromptOwner && PromptIndex >= 0)
	{
		TArray<UProximityPromptComponent*> Prompts;
		PromptOwner->GetComponents<UProximityPromptComponent>(Prompts);

		if (Prompts.IsValidIndex(PromptIndex))
		{
			NewPrompt = Prompts[PromptIndex];
		}
	}

	if (CurrentPrompt.Get() == NewPrompt)
		return;

	if (CurrentPrompt.IsValid())
		CurrentPrompt->SetPromptVisible(false);

	if (NewPrompt)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, "Proximity prompt is show");
		NewPrompt->SetPromptVisible(true);
		CurrentPrompt = NewPrompt;
	}
	else
	{
		CurrentPrompt = nullptr;
	}
}

void UProximityPromptManagerComponent::OnAnyPromptAction(const FInputActionInstance& Instance)
{
	
	const UInputAction* Fired = Instance.GetSourceAction();
	if (!Fired)
		return;

	if (CurrentPrompt.IsValid() && CurrentPrompt->InteractInputAction == Fired)
	{
		if (CurrentPrompt->IsPromptEnabled() && CurrentPrompt->IsPromptVisible())
			OnPromptInteracted.Broadcast( CurrentPrompt.Get(), PC, CurrentPrompt->GetOwner() );
	}
}

void UProximityPromptManagerComponent::SetAllPromptsEnabled(bool bNewEnabled)
{
	for (const TWeakObjectPtr<UProximityPromptComponent>& Prompt : AllPrompts)
	{
		if (Prompt.IsValid() && Prompt->IsPromptEnabled() != bNewEnabled)
		{
			Prompt->SetPromptEnabled(bNewEnabled);
		}
	}
}

