#include "Components/PatrolVisualizerComponent.h"
#include "Components/LineBatchComponent.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Settings/PatrolSystemSettings.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UPatrolVisualizerComponent::UPatrolVisualizerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = true;
	SetIsReplicatedByDefault(false); // Visualization is client-side only
}

void UPatrolVisualizerComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Load settings
	const UPatrolSystemSettings* Settings = UPatrolSystemSettings::Get();
	if (Settings)
	{
		CurrentQuality = Settings->DefaultQualityLevel;
	}
}

void UPatrolVisualizerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (LineBatchComponent)
	{
		LineBatchComponent->DestroyComponent();
		LineBatchComponent = nullptr;
	}
	
	Super::EndPlay(EndPlayReason);
}

void UPatrolVisualizerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bVisualizationEnabled)
		return;

	GlobalAnimationTime += DeltaTime;
	
	UpdateLOD();
	UpdateRouteVisuals(DeltaTime);
}

// ============================================================
// PUBLIC API
// ============================================================

void UPatrolVisualizerComponent::UpdateVisualization(const TArray<FPatrolRouteExtended>& Routes)
{
	CurrentRoutes = Routes;
	
	// Resize cache to match routes
	if (VisualizationCache.Num() != Routes.Num())
	{
		VisualizationCache.SetNum(Routes.Num());
	}

	bVisualsDirty = true;
}

void UPatrolVisualizerComponent::ClearVisualization()
{
	CurrentRoutes.Empty();
	VisualizationCache.Empty();
	
	if (LineBatchComponent)
	{
		LineBatchComponent->Flush();
	}
	
	bVisualsDirty = false;
}

void UPatrolVisualizerComponent::SetVisualizationQuality(EPatrolVisualQuality Quality)
{
	if (CurrentQuality != Quality)
	{
		CurrentQuality = Quality;
		bVisualsDirty = true;
	}
}

void UPatrolVisualizerComponent::SetVisualizationEnabled(bool bEnabled)
{
	if (bVisualizationEnabled != bEnabled)
	{
		bVisualizationEnabled = bEnabled;
		
		if (!bEnabled && LineBatchComponent)
		{
			LineBatchComponent->Flush();
		}
		else if (bEnabled)
		{
			bVisualsDirty = true;
		}
	}
}

void UPatrolVisualizerComponent::ForceRefresh()
{
	// Invalidate all caches
	for (FPatrolVisualizationCache& Cache : VisualizationCache)
	{
		Cache.GeometryHash = 0;
	}
	
	bVisualsDirty = true;
	RefreshTimer = 0.f;
}

// ============================================================
// CORE RENDERING
// ============================================================

void UPatrolVisualizerComponent::EnsureLineBatchComponent()
{
	if (LineBatchComponent && LineBatchComponent->IsRegistered())
		return;

	AActor* Owner = GetOwner();
	if (!Owner)
		return;

	LineBatchComponent = NewObject<ULineBatchComponent>(Owner, TEXT("PatrolLineRenderer"));
	if (LineBatchComponent)
	{
		LineBatchComponent->SetupAttachment(Owner->GetRootComponent());
		LineBatchComponent->SetAutoActivate(true);
		LineBatchComponent->RegisterComponent();
		LineBatchComponent->SetHiddenInGame(false);
		LineBatchComponent->bCalculateAccurateBounds = false;
	}
}

void UPatrolVisualizerComponent::UpdateRouteVisuals(float DeltaSeconds)
{
	EnsureLineBatchComponent();
	if (!LineBatchComponent)
		return;

	const UPatrolSystemSettings* Settings = UPatrolSystemSettings::Get();
	if (!Settings)
		return;

	RefreshTimer -= DeltaSeconds;
	
	// Check if we should skip this update
	if (!bVisualsDirty && RefreshTimer > 0.f)
		return;

	// Clear previous frame
	LineBatchComponent->Flush();

	// Render all routes
	RenderActiveRoutes();

	RefreshTimer = Settings->VisualRefreshInterval;
	bVisualsDirty = false;
}

void UPatrolVisualizerComponent::RenderActiveRoutes()
{
	if (!LineBatchComponent || CurrentRoutes.IsEmpty())
		return;

	const UPatrolSystemSettings* Settings = UPatrolSystemSettings::Get();
	if (!Settings)
		return;

	// Limit number of visible routes for performance
	const int32 MaxRoutes = FMath::Min(CurrentRoutes.Num(), Settings->MaxVisibleRoutes);

	for (int32 i = 0; i < MaxRoutes; ++i)
	{
		RenderRouteCached(CurrentRoutes[i], i);
	}
}

void UPatrolVisualizerComponent::RenderRouteCached(const FPatrolRouteExtended& Route, int32 CacheIndex)
{
	if (Route.PatrolPoints.Num() < 2)
		return;

	const UPatrolSystemSettings* Settings = UPatrolSystemSettings::Get();
	if (!Settings)
		return;

	// Validate cache index
	if (!VisualizationCache.IsValidIndex(CacheIndex))
		return;

	FPatrolVisualizationCache& Cache = VisualizationCache[CacheIndex];

	// Check if geometry needs rebuilding
	const uint32 NewHash = HashGeometry(Route.PatrolPoints, Route.bIsLoop);
	const bool bNeedsRebuild = (NewHash != Cache.GeometryHash);

	if (bNeedsRebuild)
	{
		// Rebuild elevated points
		Cache.ElevatedPoints.Reset(Route.PatrolPoints.Num());
		for (const FVector& Point : Route.PatrolPoints)
		{
			Cache.ElevatedPoints.Add(Point + FVector(0, 0, Settings->VisualZOffset));
		}

		// Build smooth spline
		BuildSplineSamples(Cache.ElevatedPoints, Route.bIsLoop, Cache.SplineSamples);

		// Calculate bounds
		Cache.RouteBounds = CalculateRouteBounds(Cache.ElevatedPoints);

		// Update cache metadata
		Cache.GeometryHash = NewHash;
		Cache.bLoopCached = Route.bIsLoop;
	}

	// Frustum culling
	if (Settings->bEnableFrustumCulling && ShouldCullRoute(Cache.RouteBounds))
		return;

	// Get color for this route
	FLinearColor RouteColor = GetRouteColor(Route, Cache.State);
	
	// Apply animations if enabled
	if (ShouldAnimate())
	{
		RouteColor = ApplyColorAnimation(RouteColor, GlobalAnimationTime);
	}

	// Determine thickness based on LOD
	float Thickness = Settings->LineThickness;
	if (ShouldAnimate())
	{
		Thickness += FMath::Sin(GlobalAnimationTime * Settings->LineWaveSpeed) * Settings->LineWaveAmplitude;
	}

	// Adjust rendering detail based on quality and LOD
	const bool bRenderWaypoints = (CurrentQuality >= EPatrolVisualQuality::Medium) && (CurrentLODLevel <= 1);
	const bool bRenderArrows = (CurrentQuality >= EPatrolVisualQuality::High) && (CurrentLODLevel == 0) && Route.bShowDirectionArrows;
	const bool bRenderNumbers = (CurrentQuality >= EPatrolVisualQuality::High) && (CurrentLODLevel == 0) && Route.bShowWaypointNumbers;

	// Draw spline
	DrawSplinePolyline(Cache.SplineSamples, RouteColor, Thickness, ShouldAnimate());

	// Draw waypoints
	if (bRenderWaypoints)
	{
		DrawWaypoints(Cache.ElevatedPoints, RouteColor, bRenderNumbers);
	}

	// Draw direction arrows
	if (bRenderArrows && Settings->ArrowSpacing > 0.f)
	{
		DrawDirectionArrows(Cache.SplineSamples, RouteColor.ToFColor(true));
	}

	Cache.LastAccessTime = GlobalAnimationTime;
}

void UPatrolVisualizerComponent::DrawSplinePolyline(const TArray<FVector>& Samples, const FLinearColor& Color, float Thickness, bool bAnimated)
{
	if (!LineBatchComponent || Samples.Num() < 2)
		return;

	const UPatrolSystemSettings* Settings = UPatrolSystemSettings::Get();
	if (!Settings)
		return;

	// Draw glow passes
	if (Settings->GlowPasses > 0 && CurrentQuality >= EPatrolVisualQuality::High)
	{
		const float TimeWobble = bAnimated ? (0.5f + 0.5f * FMath::Sin(GlobalAnimationTime * Settings->LineWaveSpeed)) : 0.f;

		for (int32 Pass = Settings->GlowPasses; Pass > 0; --Pass)
		{
			const float T = static_cast<float>(Pass) / static_cast<float>(Settings->GlowPasses);
			const float ExtraThickness = Settings->GlowSize * T * (0.6f + 0.4f * TimeWobble);
			
			FLinearColor GlowColor = Color;
			GlowColor.A = Settings->GlowAlpha * (T * T);
			
			const float GlowThickness = Thickness + ExtraThickness;

			for (int32 i = 0; i < Samples.Num() - 1; ++i)
			{
				LineBatchComponent->DrawLine(Samples[i], Samples[i + 1], GlowColor.ToFColor(true), SDPG_World, GlowThickness, 0.f);
			}
		}
	}

	// Draw core line with gradient
	for (int32 i = 0; i < Samples.Num() - 1; ++i)
	{
		const float U = static_cast<float>(i) / static_cast<float>(Samples.Num() - 1);
		FLinearColor SegmentColor = FLinearColor::LerpUsingHSV(Color, FLinearColor::White, 0.25f * U);
		
		LineBatchComponent->DrawLine(Samples[i], Samples[i + 1], SegmentColor.ToFColor(true), SDPG_World, Thickness, 0.f);
	}
}

void UPatrolVisualizerComponent::DrawWaypoints(const TArray<FVector>& Points, const FLinearColor& Color, bool bShowNumbers)
{
	if (!LineBatchComponent || Points.IsEmpty())
		return;

	const UPatrolSystemSettings* Settings = UPatrolSystemSettings::Get();
	if (!Settings)
		return;

	// Pulsing animation
	const float Pulse = ShouldAnimate() ? (FMath::Sin(GlobalAnimationTime * Settings->PointPulseSpeed) + 1.f) * 0.5f : 0.5f;
	const float ExtraRadius = 8.f * Pulse;

	for (int32 i = 0; i < Points.Num(); ++i)
	{
		const FVector& Point = Points[i];
		const float Radius = Settings->WaypointRadius + ExtraRadius;

		// Draw point sphere
		LineBatchComponent->DrawPoint(Point, Color.ToFColor(true), Radius, SDPG_World, 0.f);

		// Draw circle ring
		DrawDebugCircle(
			GetWorld(),
			Point,
			Radius * 1.4f,
			32,
			FColor(Color.ToFColor(true).R, Color.ToFColor(true).G, Color.ToFColor(true).B, 120),
			false,
			0.f,
			0,
			2.f,
			FVector(1, 0, 0),
			FVector(0, 1, 0),
			false
		);

		// Draw waypoint number
		if (bShowNumbers)
		{
			const FString NumberText = FString::FromInt(i + 1);
			const FVector TextLocation = Point + FVector(0, 0, Settings->WaypointRadius * 2.f);
			
			DrawDebugString(
				GetWorld(),
				TextLocation,
				NumberText,
				nullptr,
				Color.ToFColor(true),
				0.f,
				true,
				Settings->WaypointNumberScale
			);
		}
	}
}

void UPatrolVisualizerComponent::DrawDirectionArrows(const TArray<FVector>& Samples, const FLinearColor& Color)
{
	if (!LineBatchComponent || Samples.Num() < 2)
		return;

	const UPatrolSystemSettings* Settings = UPatrolSystemSettings::Get();
	if (!Settings)
		return;

	const float ArrowSpacing = Settings->ArrowSpacing;
	const float HeadLength = Settings->ArrowHeadSize;
	const float HeadAngleRad = FMath::DegreesToRadians(Settings->ArrowHeadAngle);

	float AccumulatedDistance = 0.f;

	for (int32 i = 1; i < Samples.Num(); ++i)
	{
		const FVector& A = Samples[i - 1];
		const FVector& B = Samples[i];
		const float SegmentLength = FVector::Distance(A, B);

		AccumulatedDistance += SegmentLength;

		if (AccumulatedDistance < ArrowSpacing)
			continue;

		AccumulatedDistance = 0.f;

		// Calculate arrow direction
		const FVector Direction = (B - A).GetSafeNormal();
		const FVector Right = FVector::CrossProduct(Direction, FVector::UpVector).GetSafeNormal();

		// Arrow tip at point B
		const FVector Tip = B;
		const FVector LeftWing = Tip - Direction * HeadLength * FMath::Cos(HeadAngleRad) + (-Right) * HeadLength * FMath::Sin(HeadAngleRad);
		const FVector RightWing = Tip - Direction * HeadLength * FMath::Cos(HeadAngleRad) + Right * HeadLength * FMath::Sin(HeadAngleRad);

		// Draw arrow
		LineBatchComponent->DrawLine(Tip, LeftWing, Color.ToFColor(true), SDPG_World, Settings->LineThickness, 0.f);
		LineBatchComponent->DrawLine(Tip, RightWing, Color.ToFColor(true), SDPG_World, Settings->LineThickness, 0.f);
	}
}

// ============================================================
// GEOMETRY GENERATION
// ============================================================

void UPatrolVisualizerComponent::BuildSplineSamples(const TArray<FVector>& Points, bool bLoop, TArray<FVector>& OutSamples)
{
	OutSamples.Reset();

	const int32 NumPoints = Points.Num();
	if (NumPoints < 2)
	{
		OutSamples = Points;
		return;
	}

	const UPatrolSystemSettings* Settings = UPatrolSystemSettings::Get();
	const int32 SamplesPerSegment = Settings ? Settings->SplineSamplesPerSegment : 8;

	const int32 NumSegments = bLoop ? NumPoints : NumPoints - 1;

	auto WrapIndex = [NumPoints, bLoop](int32 Index) -> int32
	{
		return bLoop ? (Index % NumPoints + NumPoints) % NumPoints : FMath::Clamp(Index, 0, NumPoints - 1);
	};

	OutSamples.Reserve(NumSegments * SamplesPerSegment + 1);

	for (int32 Segment = 0; Segment < NumSegments; ++Segment)
	{
		const FVector& P0 = Points[WrapIndex(Segment - 1)];
		const FVector& P1 = Points[WrapIndex(Segment)];
		const FVector& P2 = Points[WrapIndex(Segment + 1)];
		const FVector& P3 = Points[WrapIndex(Segment + 2)];

		for (int32 Sample = 0; Sample < SamplesPerSegment; ++Sample)
		{
			const float T = static_cast<float>(Sample) / static_cast<float>(SamplesPerSegment);
			FVector SamplePoint = EvaluateCatmullRom(P0, P1, P2, P3, T);

			// Smoothing
			if (!OutSamples.IsEmpty())
			{
				SamplePoint = FMath::Lerp(OutSamples.Last(), SamplePoint, 0.98f);
			}

			// Avoid duplicate points
			if (OutSamples.IsEmpty() || !SamplePoint.Equals(OutSamples.Last(), 0.5f))
			{
				OutSamples.Add(SamplePoint);
			}
		}
	}

	// Add final point
	OutSamples.Add(Points[bLoop ? 0 : NumPoints - 1]);
}

FVector UPatrolVisualizerComponent::EvaluateCatmullRom(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T)
{
	const float T2 = T * T;
	const float T3 = T2 * T;

	return 0.5f * (
		(2.f * P1) +
		(-P0 + P2) * T +
		(2.f * P0 - 5.f * P1 + 4.f * P2 - P3) * T2 +
		(-P0 + 3.f * P1 - 3.f * P2 + P3) * T3
	);
}

uint32 UPatrolVisualizerComponent::HashGeometry(const TArray<FVector>& Points, bool bLoop)
{
	if (Points.IsEmpty())
		return 0u;

	FVector Sum = FVector::ZeroVector;
	for (const FVector& Point : Points)
	{
		Sum += Point;
	}

	const FVector& First = Points[0];
	const FVector& Last = Points.Last();

	uint32 Hash = 2166136261u;

	auto MixFloat = [&Hash](float Value)
	{
		const uint32 Bits = *reinterpret_cast<const uint32*>(&Value);
		Hash ^= Bits;
		Hash *= 16777619u;
	};

	MixFloat(static_cast<float>(Points.Num()));
	MixFloat(First.X); MixFloat(Last.X); MixFloat(Sum.X);
	MixFloat(First.Y); MixFloat(Last.Y); MixFloat(Sum.Y);
	MixFloat(First.Z); MixFloat(Last.Z); MixFloat(Sum.Z);
	MixFloat(bLoop ? 1.f : 0.f);

	return Hash;
}

FBox UPatrolVisualizerComponent::CalculateRouteBounds(const TArray<FVector>& Points)
{
	FBox Bounds(ForceInit);

	for (const FVector& Point : Points)
	{
		Bounds += Point;
	}

	// Add some padding
	Bounds = Bounds.ExpandBy(100.f);

	return Bounds;
}

// ============================================================
// LOD & CULLING
// ============================================================

void UPatrolVisualizerComponent::UpdateLOD()
{
	const float CameraDistance = GetDistanceToCamera();
	LastCameraDistance = CameraDistance;

	const UPatrolSystemSettings* Settings = UPatrolSystemSettings::Get();
	if (!Settings)
		return;

	// Determine LOD level
	int32 NewLOD = 0;
	if (CameraDistance > Settings->LODDistance2)
	{
		NewLOD = 2; // Low detail
	}
	else if (CameraDistance > Settings->LODDistance1)
	{
		NewLOD = 1; // Medium detail
	}

	if (NewLOD != CurrentLODLevel)
	{
		CurrentLODLevel = NewLOD;
		bVisualsDirty = true;
	}
}

UCameraComponent* UPatrolVisualizerComponent::GetViewCamera() const
{
	UWorld* World = GetWorld();
	if (!World)
		return nullptr;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
		return nullptr;

	APawn* Pawn = PC->GetPawn();
	if (!Pawn)
		return nullptr;

	return Pawn->FindComponentByClass<UCameraComponent>();
}

bool UPatrolVisualizerComponent::ShouldCullRoute(const FBox& RouteBounds) const
{
	const UPatrolSystemSettings* Settings = UPatrolSystemSettings::Get();
	if (!Settings)
		return false;

	// Distance culling
	if (LastCameraDistance > Settings->MaxVisualizationDistance)
		return true;

	// Frustum culling would go here (requires camera frustum planes)
	// For now, just use distance culling

	return false;
}

float UPatrolVisualizerComponent::GetDistanceToCamera() const
{
	UCameraComponent* Camera = GetViewCamera();
	if (!Camera)
		return 0.f;

	const AActor* Owner = GetOwner();
	if (!Owner)
		return 0.f;

	return FVector::Distance(Camera->GetComponentLocation(), Owner->GetActorLocation());
}

// ============================================================
// HELPERS
// ============================================================

FLinearColor UPatrolVisualizerComponent::GetRouteColor(const FPatrolRouteExtended& Route, EPatrolVisualizationState State) const
{
	const UPatrolSystemSettings* Settings = UPatrolSystemSettings::Get();
	if (!Settings)
		return FLinearColor::White;

	// State-based color override
	switch (State)
	{
	case EPatrolVisualizationState::Preview:
		return Settings->PreviewRouteColor;
	case EPatrolVisualizationState::Selected:
		return Settings->SelectedRouteColor;
	case EPatrolVisualizationState::Hidden:
		return FLinearColor::Transparent;
	case EPatrolVisualizationState::Active:
	default:
		break;
	}

	// Use route's custom color if set, otherwise use default
	if (Route.RouteColor != FLinearColor(0.0f, 1.0f, 1.0f)) // If not default cyan
	{
		return Route.RouteColor;
	}

	return Settings->ActiveRouteColor;
}

FLinearColor UPatrolVisualizerComponent::ApplyColorAnimation(const FLinearColor& BaseColor, float Time) const
{
	const UPatrolSystemSettings* Settings = UPatrolSystemSettings::Get();
	if (!Settings)
		return BaseColor;

	const float Pulse = (FMath::Sin(Time * Settings->ColorPulseSpeed) + 1.f) * 0.5f;
	return FLinearColor::LerpUsingHSV(BaseColor, FLinearColor::White, Pulse * 0.15f);
}

bool UPatrolVisualizerComponent::ShouldAnimate() const
{
	const UPatrolSystemSettings* Settings = UPatrolSystemSettings::Get();
	return Settings && Settings->bEnableAnimations && (CurrentQuality >= EPatrolVisualQuality::High);
}
