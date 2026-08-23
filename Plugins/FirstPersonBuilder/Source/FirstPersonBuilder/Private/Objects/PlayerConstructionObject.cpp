// Copyright 2026

#include "Objects/PlayerConstructionObject.h"
#include "Objects/ConstructionSystemSettings.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Engine/StaticMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Interfaces/BuilderResourceInterface.h"
#include "Actors/JupiterFOBCore.h"
#include "Blueprint/UserWidget.h"
#include "UI/BuilderCatalogMenu.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "System/StellarAudioManager.h"
#include "Engine/StaticMesh.h"
#include "Interaction/Deployment/JupiterInstancedPropManager.h"
#include "Interaction/Deployment/JupiterPropSnapping.h"

UPlayerConstructionObject::UPlayerConstructionObject()
{
	bIsBuildModeActive = false;
	bIsValidPlacement = false;
	bLastValidState = false;

	bIsSnappingEnabled = true;
	CurrentRotationStep = 0;
	bHasActiveSweetSpot = false;
}

void UPlayerConstructionObject::InitializeConstruction(AActor* InOwner)
{
	if (!InOwner)
		return;

	if (!GhostComponent)
	{
		GhostComponent = NewObject<UStaticMeshComponent>(InOwner, NAME_None, RF_Transient);
		if (GhostComponent)
		{
			GhostComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			GhostComponent->SetCastShadow(false);
			GhostComponent->SetVisibility(false);
			GhostComponent->RegisterComponent();
		}
	}

	if (!SweetSpotDecal)
	{
		SweetSpotDecal = NewObject<UDecalComponent>(InOwner, NAME_None, RF_Transient);
		if (SweetSpotDecal)
		{
			SweetSpotDecal->SetVisibility(false);
			if (Settings && Settings->SweetSpotMaterial)
			{
				SweetSpotDecal->SetDecalMaterial(Settings->SweetSpotMaterial);
			}
			SweetSpotDecal->RegisterComponent();
		}
	}
}

void UPlayerConstructionObject::RegisterForReplication(AActor* OwnerActor)
{
	if (OwnerActor && OwnerActor->HasAuthority())
	{
		OwnerActor->AddReplicatedSubObject(this, COND_OwnerOnly);
	}
}

void UPlayerConstructionObject::CleanupConstruction()
{
	if (GhostComponent)
	{
		GhostComponent->DestroyComponent();
		GhostComponent = nullptr;
	}

	if (SweetSpotDecal)
	{
		SweetSpotDecal->DestroyComponent();
		SweetSpotDecal = nullptr;
	}

	if (ActiveUI)
	{
		ActiveUI->RemoveFromParent();
		ActiveUI = nullptr;
	}
}

int32 UPlayerConstructionObject::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	if (HasAnyFlags(RF_ClassDefaultObject) || !IsSupportedForNetworking())
	{
		return GEngine->GetGlobalFunctionCallspace(Function, this, Stack);
	}
	
	if (AActor* OuterActor = GetTypedOuter<AActor>())
	{
		return OuterActor->GetFunctionCallspace(Function, Stack);
	}
	return FunctionCallspace::Local;
}

bool UPlayerConstructionObject::CallRemoteFunction(UFunction* Function, void* Parms, struct FOutParmRec* OutParms, FFrame* Stack)
{
	if (AActor* OuterActor = GetTypedOuter<AActor>())
	{
		UNetDriver* NetDriver = OuterActor->GetNetDriver();
		if (NetDriver)
		{
			NetDriver->ProcessRemoteFunction(OuterActor, Function, Parms, OutParms, Stack, this);
			return true;
		}
	}
	return false;
}

void UPlayerConstructionObject::TickConstruction(float DeltaTime)
{
	if (bIsBuildModeActive && SelectedRecipe)
	{
		FOBUpdateTimer -= DeltaTime;
		UpdateGhostTransform();
		CheckPlacementValidity();
	}

	if (bHasActiveSweetSpot)
	{
		bool bIsCompleted = false;
		if (SweetSpotManager.IsValid())
		{
			if (SweetSpotManager->GetInstanceState(SweetSpotInstanceID) == EBuildState::Completed)
			{
				bIsCompleted = true;
			}
		}
		else
		{
			bIsCompleted = true;
		}

		if (bIsCompleted)
		{
			bHasActiveSweetSpot = false;
			if (SweetSpotDecal)
			{
				SweetSpotDecal->SetVisibility(false);
			}
		}
		else if (SweetSpotDecal)
		{
			SweetSpotDecal->SetWorldLocation(CurrentSweetSpotWorldLocation);
		}
	}
}

UCameraComponent* UPlayerConstructionObject::GetPlayerCamera() const
{
	AActor* OuterActor = GetTypedOuter<AActor>();
	if (ACharacter* Char = Cast<ACharacter>(OuterActor))
	{
		return Char->FindComponentByClass<UCameraComponent>();
	}
	return OuterActor ? OuterActor->FindComponentByClass<UCameraComponent>() : nullptr;
}

void UPlayerConstructionObject::EnterBuildMode(UPlacementPropData* InRecipe)
{
	UE_LOG(LogTemp, Display, TEXT("[ConstructionObject] EnterBuildMode called with recipe: %s"), InRecipe ? *InRecipe->GetName() : TEXT("NULL"));
	
	if (!InRecipe)
	{
		UE_LOG(LogTemp, Error, TEXT("[ConstructionObject] EnterBuildMode failed: InRecipe is null"));
		return;
	}
	if (!GhostComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[ConstructionObject] EnterBuildMode failed: GhostComponent is null"));
		return;
	}

	SelectedRecipe = InRecipe;
	bIsBuildModeActive = true;
	CurrentRotationStep = 0;
	bHasActiveSweetSpot = false;

	if (SweetSpotDecal)
		SweetSpotDecal->SetVisibility(false);

	if (InRecipe->BuildingMesh)
	{
		UE_LOG(LogTemp, Display, TEXT("[ConstructionObject] StaticMesh loaded successfully: %s"), *InRecipe->BuildingMesh->GetName());
		GhostComponent->SetStaticMesh(InRecipe->BuildingMesh);
		GhostComponent->SetWorldScale3D(InRecipe->SpawnScale);
		GhostComponent->SetVisibility(true);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[ConstructionObject] Failed to get StaticMesh from BuildingMesh property!"));
		GhostComponent->SetStaticMesh(nullptr);
		GhostComponent->SetVisibility(false);
	}

	for (int32 i = 0; i < GhostComponent->GetNumMaterials(); ++i)
	{
		GhostComponent->SetMaterial(i, InRecipe->HologramMaterial);
	}

	bLastValidState = true;
	
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, FString::Printf(TEXT("Entered Build Mode: %s"), *InRecipe->EntityID.ToString()));
}

void UPlayerConstructionObject::ExitBuildMode()
{
	bIsBuildModeActive = false;
	SelectedRecipe = nullptr;
	bHasActiveSweetSpot = false;

	if (GhostComponent)
	{
		GhostComponent->SetVisibility(false);
	}
	if (SweetSpotDecal)
	{
		SweetSpotDecal->SetVisibility(false);
	}
	
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Exited Build Mode"));
}

void UPlayerConstructionObject::RotateGhost(int32 StepDirection)
{
	if (!bIsBuildModeActive)
		return;
	CurrentRotationStep += StepDirection;
}

void UPlayerConstructionObject::ToggleSnapping()
{
	bIsSnappingEnabled = !bIsSnappingEnabled;
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, FString::Printf(TEXT("Snapping: %s"), bIsSnappingEnabled ? TEXT("ON") : TEXT("OFF")));
}

bool UPlayerConstructionObject::PerformPlacementTrace(FHitResult& OutHit) const
{
	UCameraComponent* Cam = GetPlayerCamera();
	if (!Cam) return false;

	FVector StartLoc = Cam->GetComponentLocation();
	float TraceDist = Settings ? Settings->MaxPlacementDistance : 1000.f;
	FVector EndLoc = StartLoc + (Cam->GetForwardVector() * TraceDist);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetTypedOuter<AActor>());

	return GetWorld()->LineTraceSingleByChannel(OutHit, StartLoc, EndLoc, ECC_Visibility, Params);
}

bool UPlayerConstructionObject::TrySnapGhost(const FHitResult& Hit, FVector& OutLoc, FRotator& OutRot)
{
	if (!bIsSnappingEnabled || !SelectedRecipe || !Hit.GetActor()) return false;

	if (!bHasCachedSnappableClasses)
	{
		bHasCachedSnappableClasses = UJupiterPropSnapLibrary::BuildSnappableTargetClasses(CachedSnappableClasses);
	}

	FJupiterPropSnapSettings SnapSettings = SelectedRecipe->SnapSettings;
	SnapSettings.bRequirePropMarker = true;

	FJupiterPropSnapQuery SnapQuery;
	SnapQuery.CursorLocation = Hit.ImpactPoint;
	SnapQuery.DesiredRotation = FRotator(0.f, CurrentRotationStep * 45.0f, 0.f);
	
	if (GhostComponent->GetStaticMesh())
	{
		SnapQuery.PropExtent = GhostComponent->GetStaticMesh()->GetBoundingBox().GetExtent() * SelectedRecipe->SpawnScale;
	}

	SnapQuery.PointedActor = Hit.GetActor();
	SnapQuery.PointedComponent = Hit.GetComponent();
	SnapQuery.PointedItem = Hit.Item;
	SnapQuery.PointedNormal = Hit.ImpactNormal;
	SnapQuery.IgnoredActors.Add(GetTypedOuter<AActor>());
	SnapQuery.AllowedTargetClasses = &CachedSnappableClasses;

	FJupiterPropSnapOutcome SnapOutcome = UJupiterPropSnapLibrary::ResolveSnap(SnapSettings, SnapQuery);

	if (SnapOutcome.Result != EJupiterPropSnapResult::None)
	{
		OutLoc = SnapOutcome.Location;
		OutRot = SnapOutcome.Rotation;
		return true;
	}
	return false;
}

void UPlayerConstructionObject::CalculateFreePlacement(const FHitResult& Hit, float BottomOffset, FVector& OutLoc, FRotator& OutRot) const
{
	OutLoc = Hit.ImpactPoint;
	if (BottomOffset > 0.0f)
	{
		OutLoc += Hit.ImpactNormal * BottomOffset;
	}

	FRotator BaseRot = Hit.ImpactNormal.Rotation();
	BaseRot.Pitch -= 90.0f; // Align UP vector with normal
	
	FRotator UserRot(0.f, CurrentRotationStep * 45.0f, 0.f);
	OutRot = (FQuat(BaseRot) * FQuat(UserRot)).Rotator();
}

void UPlayerConstructionObject::UpdateGhostTransform()
{
	FHitResult HitResult;
	if (PerformPlacementTrace(HitResult))
	{
		bIsAimingAtSky = false;
		AimedActor = HitResult.GetActor();
		GhostTargetLocation = HitResult.ImpactPoint;

		float BottomOffset = 0.0f;
		if (SelectedRecipe && SelectedRecipe->bAutoGround && GhostComponent->GetStaticMesh())
		{
			FBox LocalBox = GhostComponent->GetStaticMesh()->GetBoundingBox();
			BottomOffset = -LocalBox.Min.Z * SelectedRecipe->SpawnScale.Z;
		}

		if (TrySnapGhost(HitResult, GhostTargetLocation, GhostTargetRotation))
		{
			if (BottomOffset > 0.0f)
			{
				GhostTargetLocation.Z += BottomOffset;
			}
		}
		else
		{
			CalculateFreePlacement(HitResult, BottomOffset, GhostTargetLocation, GhostTargetRotation);
		}
	}
	else
	{
		bIsAimingAtSky = true;
		UCameraComponent* Cam = GetPlayerCamera();
		if (Cam)
		{
			GhostTargetLocation = Cam->GetComponentLocation() + (Cam->GetForwardVector() * (Settings ? Settings->MaxPlacementDistance : 1000.f));
		}
		GhostTargetRotation = FRotator(0.f, CurrentRotationStep * 45.0f, 0.f);
	}

	if (FVector::DistSquared(GhostTargetLocation, GhostComponent->GetComponentLocation()) > KINDA_SMALL_NUMBER ||
		!GhostTargetRotation.Equals(GhostComponent->GetComponentRotation()))
	{
		float DeltaTime = GetWorld()->GetDeltaSeconds();
		FVector NewLoc = FMath::VInterpTo(GhostComponent->GetComponentLocation(), GhostTargetLocation, DeltaTime, 15.f);
		FRotator NewRot = FMath::RInterpTo(GhostComponent->GetComponentRotation(), GhostTargetRotation, DeltaTime, 15.f);

		GhostComponent->SetWorldLocationAndRotation(NewLoc, NewRot);
	}
}

void UPlayerConstructionObject::CheckPlacementValidity()
{
	bIsValidPlacement = true;

	if (bIsAimingAtSky)
	{
		bIsValidPlacement = false;
	}

	if (SelectedRecipe && SelectedRecipe->bIsFOB)
	{
		if (FindNearbyFOB() != nullptr)
		{
			bIsValidPlacement = false;
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, TEXT("Cannot place FOB inside another FOB radius!"));
		}
	}
	else
	{
		if (FindNearbyFOB() == nullptr)
		{
			bIsValidPlacement = false;
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, TEXT("Must be built near a FOB!"));
		}
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams OverlapParams;
	OverlapParams.AddIgnoredActor(GetTypedOuter<AActor>());
	if (AimedActor.IsValid())
	{
		OverlapParams.AddIgnoredActor(AimedActor.Get());
	}

	FBox LocalBox = GhostComponent->GetStaticMesh()->GetBoundingBox();
	FVector LocalExtent = LocalBox.GetExtent() * SelectedRecipe->SpawnScale;
	FVector LocalCenter = LocalBox.GetCenter() * SelectedRecipe->SpawnScale;
	FVector WorldCenter = GhostTargetLocation + GhostTargetRotation.RotateVector(LocalCenter);

	FVector Extent = LocalExtent * 0.9f; 

	if (GetWorld()->OverlapMultiByChannel(Overlaps, WorldCenter, GhostTargetRotation.Quaternion(), ECC_Visibility, FCollisionShape::MakeBox(Extent), OverlapParams))
	{
		bIsValidPlacement = false;
	}

	if (bIsValidPlacement != bLastValidState)
	{
		UMaterialInterface* Mat = bIsValidPlacement ? SelectedRecipe->HologramMaterial : SelectedRecipe->InvalidHologramMaterial;
		for (int32 i = 0; i < GhostComponent->GetNumMaterials(); ++i)
		{
			GhostComponent->SetMaterial(i, Mat);
		}
		bLastValidState = bIsValidPlacement;
	}
}

void UPlayerConstructionObject::RequestPlacement()
{
	if (!SelectedRecipe || !bIsValidPlacement)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Invalid Placement!"));
		return;
	}

	if (!CheckAndConsumeResources(SelectedRecipe, false, false))
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Not enough resources!"));
		return; // Deny placement locally
	}

	FTransform SpawnTransform(GhostTargetRotation, GhostTargetLocation, SelectedRecipe->SpawnScale);
	Server_RequestPlacement(SelectedRecipe, SpawnTransform);

	ExitBuildMode();
}

void UPlayerConstructionObject::Server_RequestPlacement_Implementation(UPlacementPropData* Recipe, FTransform SpawnTransform)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Server: RPC Received!"));

	if (!Recipe)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Server: Recipe is NULL!"));
		return;
	}

	if (!CheckAndConsumeResources(Recipe, false, false))
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Server: Not enough resources for placement!"));
		return; 
	}

	if (AJupiterInstancedPropManager* Manager = GetPropManager())
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Server: Calling AddBuilding!"));
		Manager->AddBuilding(Recipe, SpawnTransform, true);
	}
	else
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Server: Manager is NULL!"));
	}
}

void UPlayerConstructionObject::RequestHammerHit()
{
	if (bIsBuildModeActive)
		return;

	UCameraComponent* Cam = GetPlayerCamera();
	if (!Cam) return;

	FVector StartLoc = Cam->GetComponentLocation();
	FVector EndLoc = StartLoc + (Cam->GetForwardVector() * 250.f);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetTypedOuter<AActor>());

	if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, ECC_Visibility, Params))
	{
		if (HitResult.GetComponent() && HitResult.GetComponent()->IsA<UHierarchicalInstancedStaticMeshComponent>())
		{
			UHierarchicalInstancedStaticMeshComponent* HitHISM = Cast<UHierarchicalInstancedStaticMeshComponent>(HitResult.GetComponent());
			int32 HISMIndex = HitResult.Item;

			if (HISMIndex != INDEX_NONE)
			{
				if (AJupiterInstancedPropManager* Manager = Cast<AJupiterInstancedPropManager>(HitHISM->GetOwner()))
				{
					int32 InstanceID = Manager->FindInstanceID(HitHISM, HISMIndex);
					if (InstanceID != INDEX_NONE)
					{
						EBuildState State = Manager->GetInstanceState(InstanceID);
						if (State == EBuildState::Completed)
						{
							return;
						}

						bool bIsPerfectHit = false;
						if (bHasActiveSweetSpot)
						{
							float Dist = FVector::Dist(HitResult.ImpactPoint, CurrentSweetSpotWorldLocation);
							if (Dist < 40.0f) // 40 units radius for perfect hit
							{
								bIsPerfectHit = true;
								if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("PERFECT HIT!"));
							}
						}
						
						// --- Play Feedback Locally ---
						UPlacementPropData* TargetRecipe = Manager->GetRecipeForInstance(InstanceID);
						if (TargetRecipe)
						{
							PlayConstructionFeedback(TargetRecipe, bIsPerfectHit, HitResult.ImpactPoint);
						}
						// -----------------------------

						Server_RequestHammerHit(InstanceID, nullptr, bIsPerfectHit);
						UpdateSweetSpot(HitHISM, HISMIndex, Manager, InstanceID);
					}
				}
			}
		}
	}
}

void UPlayerConstructionObject::UpdateSweetSpot(UHierarchicalInstancedStaticMeshComponent* HISM, int32 InstanceIndex, class AJupiterInstancedPropManager* Manager, int32 InstanceID)
{
	if (!HISM || !HISM->GetStaticMesh())
		return;

	SweetSpotManager = Manager;
	SweetSpotInstanceID = InstanceID;

	FTransform InstanceTransform;
	HISM->GetInstanceTransform(InstanceIndex, InstanceTransform, true);
	
	FBox Bounds = HISM->GetStaticMesh()->GetBoundingBox();
	FVector Center = InstanceTransform.GetLocation();
	float Radius = Bounds.GetExtent().Size() * InstanceTransform.GetScale3D().GetMax();
	
	bool bFoundSurface = false;
	
	for (int32 i = 0; i < 10; ++i)
	{
		FVector RandomDir = FMath::VRand();
		RandomDir.Z = FMath::Abs(RandomDir.Z); 
		RandomDir.Z = FMath::Max(0.2f, RandomDir.Z);
		RandomDir.Normalize();
		
		FVector TraceStart = Center + RandomDir * (Radius * 2.0f);
		
		FHitResult Hit;
		FCollisionQueryParams Params;
		if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, Center, ECC_Visibility, Params))
		{
			if (Hit.GetComponent() == HISM && Hit.Item == InstanceIndex)
			{
				CurrentSweetSpotWorldLocation = Hit.ImpactPoint;
				bHasActiveSweetSpot = true;
				bFoundSurface = true;
				
				if (SweetSpotDecal)
				{
					SweetSpotDecal->SetWorldLocation(CurrentSweetSpotWorldLocation);
					FRotator DecalRot = (-Hit.ImpactNormal).Rotation();
					DecalRot.Roll = FMath::RandRange(0.0f, 360.0f); // Random spin
					SweetSpotDecal->SetWorldRotation(DecalRot);
					SweetSpotDecal->DecalSize = FVector(40.f, 20.f, 20.f);
					SweetSpotDecal->SetVisibility(true);
				}
				break;
			}
		}
	}
	
	if (!bFoundSurface)
	{
		// Fallback to bounding box edge
		FVector RandomOffset = FMath::RandPointInBox(Bounds);
		CurrentSweetSpotWorldLocation = InstanceTransform.TransformPosition(RandomOffset);
		bHasActiveSweetSpot = true;
		
		if (SweetSpotDecal)
		{
			SweetSpotDecal->SetWorldLocation(CurrentSweetSpotWorldLocation);
			FVector Dir = (CurrentSweetSpotWorldLocation - Center).GetSafeNormal();
			SweetSpotDecal->SetWorldRotation((-Dir).Rotation());
			SweetSpotDecal->DecalSize = FVector(40.f, 20.f, 20.f);
			SweetSpotDecal->SetVisibility(true);
		}
	}
}

void UPlayerConstructionObject::Server_RequestHammerHit_Implementation(int32 TargetInstanceID, UPlacementPropData* TargetRecipe, bool bIsPerfectHit)
{
	if (!TargetRecipe)
	{
		if (AJupiterInstancedPropManager* Manager = GetPropManager())
		{
			TargetRecipe = Manager->GetRecipeForInstance(TargetInstanceID);
		}
	}

	if (TargetRecipe)
	{
		if (!CheckAndConsumeResources(TargetRecipe, true, true))
		{
			return; // Can't afford hit
		}
	}

	int32 FinalProgress = Settings ? Settings->ProgressPerHit : 10;
	if (bIsPerfectHit)
	{
		FinalProgress *= 3; // Triple speed on perfect hit!
	}

	if (AJupiterInstancedPropManager* Manager = GetPropManager())
	{
		Manager->ProcessHammerHit(TargetInstanceID, TargetRecipe, FinalProgress);
	}
}

void UPlayerConstructionObject::RequestDismantle()
{
	UCameraComponent* Cam = GetPlayerCamera();
	if (!Cam) return;

	FVector StartLoc = Cam->GetComponentLocation();
	FVector EndLoc = StartLoc + (Cam->GetForwardVector() * 250.f);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetTypedOuter<AActor>());

	if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, ECC_Visibility, Params))
	{
		if (HitResult.GetComponent() && HitResult.GetComponent()->IsA<UHierarchicalInstancedStaticMeshComponent>())
		{
			UHierarchicalInstancedStaticMeshComponent* HitHISM = Cast<UHierarchicalInstancedStaticMeshComponent>(HitResult.GetComponent());
			int32 HISMIndex = HitResult.Item;
			if (HISMIndex != INDEX_NONE)
			{
				if (AJupiterInstancedPropManager* Manager = GetPropManager())
				{
					int32 InstanceID = Manager->FindInstanceID(HitHISM, HISMIndex);
					if (InstanceID != INDEX_NONE)
					{
						Server_RequestDismantle(InstanceID);
					}
				}
			}
		}
	}
}

void UPlayerConstructionObject::Server_RequestDismantle_Implementation(int32 TargetInstanceID)
{
	if (AJupiterInstancedPropManager* Manager = GetPropManager())
	{
		Manager->DeleteInstance(TargetInstanceID);
	}
}

AJupiterFOBCore* UPlayerConstructionObject::FindNearbyFOB()
{
	if (CachedFOB.IsValid() && FOBUpdateTimer > 0.0f)
	{
		// Still valid timer, just verify distance
		FVector MyLocation = GetTypedOuter<AActor>()->GetActorLocation();
		float DistSq = FVector::DistSquared(MyLocation, CachedFOB->GetActorLocation());
		if (DistSq <= (CachedFOB->GetBuildRadius() * CachedFOB->GetBuildRadius()))
		{
			return CachedFOB.Get();
		}
	}

	FOBUpdateTimer = 1.0f; // Update only every 1 second
	CachedFOB = nullptr;

	TArray<AActor*> FoundFOBs;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AJupiterFOBCore::StaticClass(), FoundFOBs);

	FVector MyLocation = GetTypedOuter<AActor>()->GetActorLocation();
	AJupiterFOBCore* ClosestFOB = nullptr;
	float ClosestDistSq = MAX_flt;

	for (AActor* Actor : FoundFOBs)
	{
		if (AJupiterFOBCore* FOB = Cast<AJupiterFOBCore>(Actor))
		{
			float DistSq = FVector::DistSquared(MyLocation, FOB->GetActorLocation());
			float Radius = FOB->GetBuildRadius();
			if (DistSq <= (Radius * Radius) && DistSq < ClosestDistSq)
			{
				ClosestDistSq = DistSq;
				ClosestFOB = FOB;
			}
		}
	}
	
	if (ClosestFOB)
	{
		CachedFOB = ClosestFOB;
	}

	return ClosestFOB;
}

UObject* UPlayerConstructionObject::GetResourceProvider()
{
	if (AJupiterFOBCore* FOB = FindNearbyFOB())
	{
		return FOB;
	}
	if (AActor* OuterActor = GetTypedOuter<AActor>())
	{
		if (OuterActor->Implements<UBuilderResourceInterface>())
		{
			return OuterActor;
		}
	}
	return nullptr;
}

AJupiterInstancedPropManager* UPlayerConstructionObject::GetPropManager()
{
	if (CachedPropManager.IsValid())
	{
		return CachedPropManager.Get();
	}

	if (AJupiterInstancedPropManager* ExistingManager = Cast<AJupiterInstancedPropManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AJupiterInstancedPropManager::StaticClass())))
	{
		CachedPropManager = ExistingManager;
		return ExistingManager;
	}

	if (AActor* OuterActor = GetTypedOuter<AActor>())
	{
		if (OuterActor->HasAuthority())
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AJupiterInstancedPropManager* NewManager = GetWorld()->SpawnActor<AJupiterInstancedPropManager>(AJupiterInstancedPropManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
			CachedPropManager = NewManager;
			return NewManager;
		}
	}

	return nullptr;
}

bool UPlayerConstructionObject::CheckAndConsumeResources(UPlacementPropData* Recipe, bool bIsHit, bool bConsume)
{
	if (!Recipe) return false;
	
	if (Recipe->ConstructionCosts.IsEmpty())
	{
		UE_LOG(LogTemp, Display, TEXT("[ConstructionObject] Recipe '%s' has no costs, auto-passing."), *Recipe->GetName());
		return true;
	}

	UObject* ResourceProvider = GetResourceProvider();
	if (!ResourceProvider)
	{
		UE_LOG(LogTemp, Error, TEXT("[ConstructionObject] No ResourceProvider found! Cannot afford anything."));
		return false;
	}
	
	UE_LOG(LogTemp, Display, TEXT("[ConstructionObject] ResourceProvider: %s (Class: %s)"), *ResourceProvider->GetName(), *ResourceProvider->GetClass()->GetName());

	for (const FJupiterResourceCost& CostItem : Recipe->ConstructionCosts)
	{
		int32 AmountToCheck = CostItem.Amount;
		if (bIsHit)
		{
			AmountToCheck = FMath::Max(1, CostItem.Amount / FMath::Max(1, Recipe->HitsRequired));
		}
		
		FString ResourceName = CostItem.ResourceItem.IsNull() ? TEXT("NULL") : CostItem.ResourceItem.GetAssetName();
		
		if (bConsume)
		{
			if (!IBuilderResourceInterface::Execute_ConsumeResource(ResourceProvider, CostItem.ResourceItem, AmountToCheck))
			{
				UE_LOG(LogTemp, Warning, TEXT("[ConstructionObject] ConsumeResource FAILED for '%s' x%d"), *ResourceName, AmountToCheck);
				return false;
			}
		}
		else
		{
			if (!IBuilderResourceInterface::Execute_CanAffordResource(ResourceProvider, CostItem.ResourceItem, AmountToCheck))
			{
				UE_LOG(LogTemp, Warning, TEXT("[ConstructionObject] CanAffordResource FAILED for '%s' x%d"), *ResourceName, AmountToCheck);
				return false;
			}
		}
	}
	return true;
}

void UPlayerConstructionObject::PlayConstructionFeedback(UPlacementPropData* Recipe, bool bIsPerfectHit, const FVector& Location) const
{
	if (!Recipe) return;

	USoundBase* SoundToPlay = bIsPerfectHit ? 
		(Recipe->PerfectHitSound ? Recipe->PerfectHitSound : (Settings ? Settings->DefaultPerfectHitSound.Get() : nullptr)) :
		(Recipe->HitSound ? Recipe->HitSound : (Settings ? Settings->DefaultHitSound.Get() : nullptr));
	
	if (SoundToPlay)
	{
		if (UStellarAudioManager* AudioMgr = GetWorld()->GetSubsystem<UStellarAudioManager>())
		{
			AudioMgr->PlayDirectSoundAtLocation(SoundToPlay, Location);
		}
		else
		{
			UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, Location);
		}
	}

	UNiagaraSystem* VFXToPlay = Recipe->HitParticle ? Recipe->HitParticle : (Settings ? Settings->DefaultHitParticle.Get() : nullptr);
	if (VFXToPlay)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), VFXToPlay, Location, FRotator::ZeroRotator);
	}
}

void UPlayerConstructionObject::OpenBuilderMenu()
{
	if (AvailableRecipes.IsEmpty())
		return;

	if (!MenuTheme && Settings && Settings->MenuTheme)
	{
		MenuTheme = Settings->MenuTheme;
	}

	if (!ActiveUI)
	{
		ActiveUI = CreateWidget<UBuilderCatalogMenu>(GetWorld(), UBuilderCatalogMenu::StaticClass());
		if (ActiveUI)
		{
			ActiveUI->MenuTheme = MenuTheme;
		}
	}

	if (ActiveUI)
	{
		ActiveUI->MenuTheme = MenuTheme;
		if (!ActiveUI->IsInViewport())
		{
			ActiveUI->AddToViewport(100);
		}
		ActiveUI->InitializeMenu(this, AvailableRecipes);
		ActiveUI->OpenBuilderMenu();
	}
	else
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to spawn BuilderCatalogMenu widget!"));
	}
}

void UPlayerConstructionObject::CloseBuilderMenu()
{
	if (ActiveUI)
	{
		ActiveUI->CloseBuilderMenu();
	}
}

void UPlayerConstructionObject::HandleRadialMenuSelection(FName SegmentID)
{
	if (SegmentID.IsNone())
		return;

	bool bFound = false;
	for (UPlacementPropData* Recipe : AvailableRecipes)
	{
		if (Recipe && Recipe->EntityID == SegmentID)
		{
			bFound = true;
			EnterBuildMode(Recipe);
			break;
		}
	}
}
