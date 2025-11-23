#include "Components/PatrolVisualizerComponent.h"
#include "Components/LineBatchComponent.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Settings/PatrolSystemSettings.h"
#include "GameFramework/PlayerController.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

// ------------------------------------------------------------
// LIFECYCLE
// ------------------------------------------------------------

UPatrolVisualizerComponent::UPatrolVisualizerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.05f; 
    
    bAutoActivate = true;
    SetIsReplicatedByDefault(false);
}

void UPatrolVisualizerComponent::BeginPlay()
{
    Super::BeginPlay();
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

    GlobalAnimationTime += DeltaTime;
    UpdateRouteVisuals(DeltaTime);
}

// ------------------------------------------------------------
// PUBLIC API
// ------------------------------------------------------------

void UPatrolVisualizerComponent::UpdateVisualization(const TArray<FPatrolRouteExtended>& Routes)
{
    CurrentRoutes = Routes;

    if (VisualizationCache.Num() != Routes.Num())
    {
        VisualizationCache.SetNum(Routes.Num());
    }

    bVisualsDirty = true;
    SetVisibility(true);
}

void UPatrolVisualizerComponent::SetVisibility(bool bVisible)
{
    if (LineBatchComponent)
    	LineBatchComponent->SetVisibility(bVisible);
	
    if (WaypointISM)
    	WaypointISM->SetVisibility(bVisible);
	
    if (ArrowISM)
    	ArrowISM->SetVisibility(bVisible);

    if (!bVisible)
    {
        for (UTextRenderComponent* TextComp : TextLabelPool)
        {
            if (TextComp)
            	TextComp->SetVisibility(false);
        }
    }
    else
    {
        bVisualsDirty = true;
    }
}

// ------------------------------------------------------------
// CORE LOGIC
// ------------------------------------------------------------

void UPatrolVisualizerComponent::UpdateRouteVisuals(float DeltaSeconds)
{
    const UPatrolSystemSettings* Settings = UPatrolSystemSettings::Get();
    if (!Settings)
    	return;

    EnsureComponents(Settings);
    if (!LineBatchComponent)
    	return;

    UpdateLOD(Settings);

    RefreshTimer -= DeltaSeconds;

    if (!bVisualsDirty && RefreshTimer > 0.f)
        return;

    LineBatchComponent->Flush();
    
    if (WaypointISM)
    	WaypointISM->ClearInstances();
	
    if (ArrowISM)
    	ArrowISM->ClearInstances();

    for (UTextRenderComponent* TextComp : TextLabelPool)
    {
        if (TextComp)
        	TextComp->SetVisibility(false);
    }
	
    ActiveTextLabels = 0;

    RenderActiveRoutes(Settings);

    RefreshTimer = Settings->VisualRefreshInterval;
    bVisualsDirty = false;
}

void UPatrolVisualizerComponent::RenderActiveRoutes(const UPatrolSystemSettings* Settings)
{
    if (CurrentRoutes.IsEmpty())
    	return;

    const int32 MaxRoutes = FMath::Min(CurrentRoutes.Num(), Settings->MaxVisibleRoutes);
    for (int32 i = 0; i < MaxRoutes; ++i)
    {
        RenderRouteCached(CurrentRoutes[i], i, Settings);
    }
}

void UPatrolVisualizerComponent::RenderRouteCached(const FPatrolRouteExtended& Route, int32 CacheIndex, const UPatrolSystemSettings* Settings)
{
    if (Route.PatrolPoints.Num() < 2 || !VisualizationCache.IsValidIndex(CacheIndex))
        return;

    FPatrolVisualizationCache& Cache = VisualizationCache[CacheIndex];

    const uint32 NewHash = HashGeometry(Route.PatrolPoints, Route.bIsLoop);
    if (NewHash != Cache.GeometryHash)
    {
        Cache.ElevatedPoints.Reset(Route.PatrolPoints.Num());
        for (const FVector& Point : Route.PatrolPoints)
        {
            Cache.ElevatedPoints.Add(Point + FVector(0, 0, Settings->VisualZOffset));
        }

        BuildSplineSamples(Cache.ElevatedPoints, Route.bIsLoop, Cache.SplineSamples, Settings);
        Cache.RouteBounds = CalculateRouteBounds(Cache.ElevatedPoints);
        Cache.GeometryHash = NewHash;
        Cache.bLoopCached = Route.bIsLoop;
    }

    if (Settings->bEnableFrustumCulling && ShouldCullRoute(Cache.RouteBounds, Settings))
        return;

    FLinearColor RouteColor = GetRouteColor(Route, Cache.State, Settings);
    if (ShouldAnimate(Settings))
    {
        RouteColor = ApplyColorAnimation(RouteColor, GlobalAnimationTime, Settings);
    }

    float Thickness = Settings->LineThickness;
    if (ShouldAnimate(Settings))
    {
        Thickness += FMath::Sin(GlobalAnimationTime * Settings->LineWaveSpeed) * Settings->LineWaveAmplitude;
    }

    const bool bIsPreview = Cache.State == EPatrolVisualizationState::Preview;
    if (bIsPreview)
    {
        DrawDashedPolyline(Cache.SplineSamples, RouteColor, Thickness);
    }
    else
    {
        DrawSplinePolyline(Cache.SplineSamples, RouteColor, Thickness);
    }


    const bool bRenderWaypoints = (CurrentLODLevel <= 1);
    const bool bRenderArrows = (CurrentLODLevel == 0) && Route.bShowDirectionArrows;
    const bool bRenderNumbers = (CurrentLODLevel == 0) && Route.bShowWaypointNumbers;

    if (bRenderWaypoints)
    {
        DrawWaypoints(Cache.ElevatedPoints, RouteColor, bRenderNumbers, Route, Settings);
    }

    if (bRenderArrows && Settings->ArrowSpacing > 0.f && !bIsPreview)
    {
        DrawDirectionArrows(Cache.SplineSamples, RouteColor.ToFColor(true), Settings);
    }

    Cache.LastAccessTime = GlobalAnimationTime;
}

// ------------------------------------------------------------
// DRAWING IMPLEMENTATION
// ------------------------------------------------------------

void UPatrolVisualizerComponent::DrawSplinePolyline(const TArray<FVector>& Samples, const FLinearColor& Color, float Thickness, bool bDashed)
{
    if (Samples.Num() < 2)
    	return;

    if (bDashed)
    {
        DrawDashedPolyline(Samples, Color, Thickness);
        return;
    }
	
    for (int32 i = 0; i < Samples.Num() - 1; ++i)
    {
        const float U = static_cast<float>(i) / static_cast<float>(Samples.Num() - 1);
        FLinearColor SegmentColor = FLinearColor::LerpUsingHSV(Color, FLinearColor::White, 0.25f * U);
        
        LineBatchComponent->DrawLine(Samples[i], Samples[i + 1], SegmentColor.ToFColor(true), SDPG_World, Thickness, 0.f);
    }
}

void UPatrolVisualizerComponent::DrawDashedPolyline(const TArray<FVector>& Samples, const FLinearColor& Color, float Thickness)
{
    if (Samples.Num() < 2)
    	return;

    const float DashSize = 50.f;
    const float GapSize = 30.f;
    float CurrentDist = 0.f;

    for (int32 i = 0; i < Samples.Num() - 1; ++i)
    {
        const FVector& P0 = Samples[i];
        const FVector& P1 = Samples[i + 1];
        const float SegmentLen = FVector::Distance(P0, P1);

        if (SegmentLen <= KINDA_SMALL_NUMBER)
        	continue;

        const FVector Dir = (P1 - P0) / SegmentLen;
        float DistAlongSegment = 0.f;

        while (DistAlongSegment < SegmentLen)
        {
            const float RemainingInDash = DashSize - FMath::Fmod(CurrentDist, DashSize + GapSize);
            if (RemainingInDash > 0)
            {
                const float DrawLen = FMath::Min(RemainingInDash, SegmentLen - DistAlongSegment);
                const FVector Start = P0 + Dir * DistAlongSegment;
                const FVector End = Start + Dir * DrawLen;
                LineBatchComponent->DrawLine(Start, End, Color.ToFColor(true), SDPG_World, Thickness, 0.f);
                DistAlongSegment += DrawLen;
                CurrentDist += DrawLen;
            }
            else
            {
                const float RemainingInGap = (DashSize + GapSize) - FMath::Fmod(CurrentDist, DashSize + GapSize);
                const float SkipLen = FMath::Min(RemainingInGap, SegmentLen - DistAlongSegment);
                DistAlongSegment += SkipLen;
                CurrentDist += SkipLen;
            }
        }
    }
}

void UPatrolVisualizerComponent::DrawWaypoints(const TArray<FVector>& Points, const FLinearColor& Color, bool bShowNumbers, const FPatrolRouteExtended& Route, const UPatrolSystemSettings* Settings)
{
    if (!WaypointISM || Points.IsEmpty())
    	return;

    const float Pulse = ShouldAnimate(Settings) ? (FMath::Sin(GlobalAnimationTime * Settings->PointPulseSpeed) + 1.f) * 0.5f : 0.5f;
    const float ScaleMult = 1.0f + (0.2f * Pulse);

    for (int32 i = 0; i < Points.Num(); ++i)
    {
        const FVector& Point = Points[i];
        
        FLinearColor PointColor = Color;
        if (i == 0 && !Route.bIsLoop)
        {
	        PointColor = FLinearColor::Green;
        }
        else if (i == Points.Num() - 1 && !Route.bIsLoop)
        {
	        PointColor = FLinearColor::Red;
        }

        FTransform InstanceTransform;
        InstanceTransform.SetLocation(Point);
        InstanceTransform.SetScale3D(FVector(ScaleMult));
        
        const int32 InstIdx = WaypointISM->AddInstance(InstanceTransform, true);
    	
        WaypointISM->SetCustomDataValue(InstIdx, 0, PointColor.R, true);
        WaypointISM->SetCustomDataValue(InstIdx, 1, PointColor.G, true);
        WaypointISM->SetCustomDataValue(InstIdx, 2, PointColor.B, true);

        if (bShowNumbers)
        {
            FString LabelText = FString::FromInt(i + 1);
            if (Route.WaitTimeAtWaypoints > 0.f)
            {
                LabelText += FString::Printf(TEXT("\n(%.1fs)"), Route.WaitTimeAtWaypoints);
            }

            const FVector TextLocation = Point + FVector(0, 0, Settings->WaypointRadius * 2.5f);
            
            // Pool management
            UTextRenderComponent* TextComp = nullptr;
            if (ActiveTextLabels < TextLabelPool.Num())
            {
                TextComp = TextLabelPool[ActiveTextLabels];
            }
            else
            {
                TextComp = NewObject<UTextRenderComponent>(GetOwner());
                TextComp->SetupAttachment(GetOwner()->GetRootComponent());
                TextComp->RegisterComponent();
                TextLabelPool.Add(TextComp);
            }
            
            if (TextComp)
            {
                TextComp->SetVisibility(true);
                TextComp->SetText(FText::FromString(LabelText));
                TextComp->SetTextRenderColor(PointColor.ToFColor(true));
                TextComp->SetWorldLocation(TextLocation);
                TextComp->SetWorldScale3D(FVector(Settings->WaypointNumberScale));
                TextComp->SetHorizontalAlignment(EHTA_Center);
                TextComp->SetVerticalAlignment(EVRTA_TextCenter);
                
                if (UCameraComponent* Cam = GetViewCamera())
                {
                    TextComp->SetWorldRotation(Cam->GetComponentRotation());
                }
                
                ActiveTextLabels++;
            }
        }
    }
}

void UPatrolVisualizerComponent::DrawDirectionArrows(const TArray<FVector>& Samples, const FLinearColor& Color, const UPatrolSystemSettings* Settings)
{
    if (!ArrowISM || Samples.Num() < 2)
    	return;

    const float ArrowSpacing = Settings->ArrowSpacing;
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

        const FVector Direction = (B - A).GetSafeNormal();
        const FRotator Rotation = Direction.Rotation() + FRotator(-90.f, 0.f, 0.f);

        FTransform InstanceTransform;
        InstanceTransform.SetLocation(B);
        InstanceTransform.SetRotation(Rotation.Quaternion());
        InstanceTransform.SetScale3D(FVector(0.5f)); 

        const int32 InstIdx = ArrowISM->AddInstance(InstanceTransform, true);
        ArrowISM->SetCustomDataValue(InstIdx, 0, Color.R, true);
        ArrowISM->SetCustomDataValue(InstIdx, 1, Color.G, true);
        ArrowISM->SetCustomDataValue(InstIdx, 2, Color.B, true);
    }
}

// ------------------------------------------------------------
// HELPERS
// ------------------------------------------------------------

void UPatrolVisualizerComponent::EnsureComponents(const UPatrolSystemSettings* Settings)
{
    AActor* Owner = GetOwner();
    if (!Owner || !Settings) return;

    // Line Batch
    if (!LineBatchComponent)
    {
        LineBatchComponent = NewObject<ULineBatchComponent>(Owner, TEXT("PatrolLineRenderer"));
        if (LineBatchComponent)
        {
            LineBatchComponent->SetupAttachment(Owner->GetRootComponent());
            LineBatchComponent->SetUsingAbsoluteLocation(true);
            LineBatchComponent->SetUsingAbsoluteRotation(true);
            LineBatchComponent->SetUsingAbsoluteScale(true);
            LineBatchComponent->RegisterComponent();
        }
    }

    // Waypoint ISM
    if (!WaypointISM)
    {
        WaypointISM = NewObject<UInstancedStaticMeshComponent>(Owner, TEXT("PatrolWaypointISM"));
        if (WaypointISM)
        {
            WaypointISM->SetupAttachment(Owner->GetRootComponent());
            WaypointISM->SetUsingAbsoluteLocation(true);
            WaypointISM->RegisterComponent();
            WaypointISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            WaypointISM->SetCastShadow(false);
            WaypointISM->SetNumCustomDataFloats(3);
            
            UStaticMesh* Mesh = Settings->WaypointMesh.IsNull() ?
        		LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere")) : Cast<UStaticMesh>(Settings->WaypointMesh.TryLoad());
        	
            if (Mesh)
            	WaypointISM->SetStaticMesh(Mesh);

            if (!Settings->WaypointMaterial.IsNull())
            {
	            if (UMaterialInterface* Mat = Cast<UMaterialInterface>(Settings->WaypointMaterial.TryLoad()))
	            {
	            	WaypointISM->SetMaterial(0, Mat);   
	            }
            }
        }
    }

    // Arrow ISM
    if (!ArrowISM)
    {
        ArrowISM = NewObject<UInstancedStaticMeshComponent>(Owner, TEXT("PatrolArrowISM"));
        if (ArrowISM)
        {
            ArrowISM->SetupAttachment(Owner->GetRootComponent());
            ArrowISM->SetUsingAbsoluteLocation(true);
            ArrowISM->RegisterComponent();
            ArrowISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            ArrowISM->SetCastShadow(false);
            ArrowISM->SetNumCustomDataFloats(3);

            UStaticMesh* Mesh = Settings->ArrowMesh.IsNull() ?
        		LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone")) : Cast<UStaticMesh>(Settings->ArrowMesh.TryLoad());
        	
            if (Mesh)
            	ArrowISM->SetStaticMesh(Mesh);

            if (!Settings->ArrowMaterial.IsNull())
            {
                UMaterialInterface* Mat = Cast<UMaterialInterface>(Settings->ArrowMaterial.TryLoad());
                if (Mat) ArrowISM->SetMaterial(0, Mat);
            }
        }
    }
}

void UPatrolVisualizerComponent::UpdateLOD(const UPatrolSystemSettings* Settings)
{
    const float CameraDistance = GetDistanceToCamera();
    LastCameraDistance = CameraDistance;

    int32 NewLOD = 0;
    if (CameraDistance > Settings->LODDistance2)
    {
    	NewLOD = 2;   
    }
    else if (CameraDistance > Settings->LODDistance1)
    {
    	NewLOD = 1;   
    }

    if (NewLOD != CurrentLODLevel)
    {
        CurrentLODLevel = NewLOD;
        bVisualsDirty = true;
    }
}

// ------------------------------------------------------------
// GEOMETRY
// ------------------------------------------------------------

void UPatrolVisualizerComponent::BuildSplineSamples(const TArray<FVector>& Points, bool bLoop, TArray<FVector>& OutSamples, const UPatrolSystemSettings* Settings)
{
    OutSamples.Reset();
    const int32 NumPoints = Points.Num();
    if (NumPoints < 2)
    {
        OutSamples = Points;
        return;
    }

    const int32 SamplesPerSegment = Settings->SplineSamplesPerSegment;
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
            
            if (!OutSamples.IsEmpty())
            {
                SamplePoint = FMath::Lerp(OutSamples.Last(), SamplePoint, 0.98f);
            }
        	
            OutSamples.Add(SamplePoint);
        }
    }
    OutSamples.Add(Points[bLoop ? 0 : NumPoints - 1]);
}

FVector UPatrolVisualizerComponent::EvaluateCatmullRom(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T)
{
    const float T2 = T * T;
    const float T3 = T2 * T;
    return 0.5f * ((2.f * P1) + (-P0 + P2) * T + (2.f * P0 - 5.f * P1 + 4.f * P2 - P3) * T2 + (-P0 + 3.f * P1 - 3.f * P2 + P3) * T3);
}

uint32 UPatrolVisualizerComponent::HashGeometry(const TArray<FVector>& Points, bool bLoop)
{
    if (Points.IsEmpty()) return 0u;
    
    uint32 Hash = 2166136261u;
    for (const FVector& P : Points)
    {
        Hash = Hash * 16777619u ^ static_cast<uint32>(P.X);
        Hash = Hash * 16777619u ^ static_cast<uint32>(P.Y);
    }
	
    Hash = Hash * 16777619u ^ (bLoop ? 1 : 0);
    return Hash;
}

FBox UPatrolVisualizerComponent::CalculateRouteBounds(const TArray<FVector>& Points)
{
    FBox Bounds(ForceInit);
    for (const FVector& Point : Points)
    	Bounds += Point;
	
    return Bounds.ExpandBy(100.f);
}

bool UPatrolVisualizerComponent::ShouldCullRoute(const FBox& RouteBounds, const UPatrolSystemSettings* Settings) const
{
    return LastCameraDistance > Settings->MaxVisualizationDistance;
}

float UPatrolVisualizerComponent::GetDistanceToCamera() const
{
    if (UCameraComponent* Camera = GetViewCamera())
    {
        return FVector::Distance(Camera->GetComponentLocation(), GetOwner()->GetActorLocation());
    }
	
    return 0.f;
}

UCameraComponent* UPatrolVisualizerComponent::GetViewCamera() const
{
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (APawn* Pawn = PC->GetPawn())
            {
                return Pawn->FindComponentByClass<UCameraComponent>();
            }
        }
    }
    return nullptr;
}

// ------------------------------------------------------------
// ANIMATION HELPERS
// ------------------------------------------------------------

FLinearColor UPatrolVisualizerComponent::GetRouteColor(const FPatrolRouteExtended& Route, EPatrolVisualizationState State, const UPatrolSystemSettings* Settings) const
{
    switch (State)
    {
        case EPatrolVisualizationState::Preview:
    	return Settings->PreviewRouteColor;
    	
        case EPatrolVisualizationState::Selected:
    	return Settings->SelectedRouteColor;
    	
        case EPatrolVisualizationState::Hidden:
    	return FLinearColor::Transparent;
    	
        default: break;
    }

    if (Route.RouteColor != FLinearColor(0.f, 1.f, 1.f)) return Route.RouteColor;
    return Settings->ActiveRouteColor;
}

FLinearColor UPatrolVisualizerComponent::ApplyColorAnimation(const FLinearColor& BaseColor, float Time, const UPatrolSystemSettings* Settings) const
{
    const float Pulse = (FMath::Sin(Time * Settings->ColorPulseSpeed) + 1.f) * 0.5f;
    return FLinearColor::LerpUsingHSV(BaseColor, FLinearColor::White, Pulse * 0.15f);
}

bool UPatrolVisualizerComponent::ShouldAnimate(const UPatrolSystemSettings* Settings) const
{
    return Settings->bEnableAnimations && (CurrentQuality >= EPatrolVisualQuality::High);
}