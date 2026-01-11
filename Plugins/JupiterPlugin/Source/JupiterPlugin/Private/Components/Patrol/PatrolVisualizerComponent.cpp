#include "Components/Patrol/PatrolVisualizerComponent.h"
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
    PrimaryComponentTick.bCanEverTick = false;
    
    bAutoActivate = true;
    SetIsReplicatedByDefault(false);
}

void UPatrolVisualizerComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UPatrolVisualizerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
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

    SetVisibility(true);
    RebuildGeometry();
}

void UPatrolVisualizerComponent::SetVisibility(bool bVisible)
{
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

void UPatrolVisualizerComponent::RebuildGeometry()
{
    const UPatrolSystemSettings* Settings = UPatrolSystemSettings::Get();
    if (!Settings)
    	return;

    EnsureComponents(Settings);
    if (!PathLineISM)
    	return;
        
    UpdateLOD(Settings);

    if (PathLineISM)
        PathLineISM->ClearInstances();

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

    const uint32 NewHash = HashGeometry(Route.PatrolPoints, Route.PatrolType);
    if (NewHash != Cache.GeometryHash)
    {
        Cache.ElevatedPoints.Reset(Route.PatrolPoints.Num());
        for (const FVector& Point : Route.PatrolPoints)
        {
            Cache.ElevatedPoints.Add(Point + FVector(0, 0, Settings->VisualZOffset));
        }

        BuildSplineSamples(Cache.ElevatedPoints, Route.PatrolType, Cache.SplineSamples, Settings);
        Cache.RouteBounds = CalculateRouteBounds(Cache.ElevatedPoints);
        Cache.GeometryHash = NewHash;
        Cache.PatrolTypeCached = Route.PatrolType;
    }

    if (Settings->bEnableFrustumCulling && ShouldCullRoute(Cache.RouteBounds, Settings))
        return;

    FLinearColor RouteColor = GetRouteColor(Route, Cache.State, Settings);

    float Thickness = Settings->LineThickness;
	
    const bool bIsPreview = Cache.State == EPatrolVisualizationState::Preview;
    if (bIsPreview)
    {
        DrawSplinePolyline(Cache.SplineSamples, RouteColor, Thickness, true);
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
    if (!PathLineISM || Samples.Num() < 2)
    	return;

    for (int32 i = 0; i < Samples.Num() - 1; ++i)
    {
        const FVector& Start = Samples[i];
        const FVector& End = Samples[i + 1];
        const FVector Delta = End - Start;
        const float Length = Delta.Size();
        
        if (Length < KINDA_SMALL_NUMBER)
        	continue;

        const FVector MidPoint = Start + (Delta * 0.5f);
        const FRotator Rotation = FRotationMatrix::MakeFromX(Delta).Rotator();
        const FVector Scale(Length / 100.f, Thickness/100.f, Thickness/100.f);

        FTransform InstanceTransform;
        InstanceTransform.SetLocation(MidPoint);
        InstanceTransform.SetRotation(Rotation.Quaternion());
        InstanceTransform.SetScale3D(Scale);

        const int32 Idx = PathLineISM->AddInstance(InstanceTransform, true);
        
        PathLineISM->SetCustomDataValue(Idx, 0, Color.R, true);
        PathLineISM->SetCustomDataValue(Idx, 1, Color.G, true);
        PathLineISM->SetCustomDataValue(Idx, 2, Color.B, true);
    }
}

void UPatrolVisualizerComponent::DrawWaypoints(const TArray<FVector>& Points, const FLinearColor& Color, bool bShowNumbers, const FPatrolRouteExtended& Route, const UPatrolSystemSettings* Settings)
{
    if (!WaypointISM || Points.IsEmpty())
    	return;

    const float ScaleMult = 1.0f;

    for (int32 i = 0; i < Points.Num(); ++i)
    {
        const FVector& Point = Points[i];
        
        FLinearColor PointColor = Color;
        if (i == 0 && Route.PatrolType != EPatrolType::Loop)
        {
	        PointColor = FLinearColor::Green;
        }
        else if (i == Points.Num() - 1 && Route.PatrolType != EPatrolType::Loop)
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

    auto CreateISM = [&](TObjectPtr<UInstancedStaticMeshComponent>& Comp, FName Name, TSoftObjectPtr<UStaticMesh> MeshAsset, TSoftObjectPtr<UMaterialInterface> MatAsset, bool bIsLine = false)
    {
        if (Comp) return;
        
        Comp = NewObject<UInstancedStaticMeshComponent>(Owner, Name);
        if (!Comp) return;

        Comp->SetupAttachment(Owner->GetRootComponent());
        Comp->SetUsingAbsoluteLocation(true);
        Comp->RegisterComponent();
        Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Comp->SetCastShadow(false);
        Comp->SetNumCustomDataFloats(3);

        UStaticMesh* Mesh = MeshAsset.IsNull() ? 
            LoadObject<UStaticMesh>(nullptr, bIsLine ? TEXT("/Engine/BasicShapes/Cube") : TEXT("/Engine/BasicShapes/Sphere")) : 
            MeshAsset.LoadSynchronous();

        if (Mesh)
        	Comp->SetStaticMesh(Mesh);

        if (!MatAsset.IsNull())
        {
            if (UMaterialInterface* Mat = MatAsset.LoadSynchronous())
                Comp->SetMaterial(0, Mat);
        }
    };

    CreateISM(WaypointISM, TEXT("PatrolWaypointISM"), TSoftObjectPtr<UStaticMesh>(Settings->WaypointMesh), TSoftObjectPtr<UMaterialInterface>(Settings->WaypointMaterial));
    
    TSoftObjectPtr<UStaticMesh> ArrowMesh(Settings->ArrowMesh);
    if (ArrowMesh.IsNull())
    {
        ArrowMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cone")));
    }
	
    CreateISM(ArrowISM, TEXT("PatrolArrowISM"), ArrowMesh, TSoftObjectPtr<UMaterialInterface>(Settings->ArrowMaterial));

    CreateISM(PathLineISM, TEXT("PatrolPathLineISM"), TSoftObjectPtr<UStaticMesh>(), TSoftObjectPtr<UMaterialInterface>(Settings->ArrowMaterial), true);
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

void UPatrolVisualizerComponent::BuildSplineSamples(const TArray<FVector>& Points, EPatrolType Type, TArray<FVector>& OutSamples, const UPatrolSystemSettings* Settings)
{
    OutSamples.Reset();
    const int32 NumPoints = Points.Num();
    if (NumPoints < 2)
    {
        OutSamples = Points;
        return;
    }

    const int32 SamplesPerSegment = Settings->SplineSamplesPerSegment;
    const bool bIsLoop = (Type == EPatrolType::Loop);
    const int32 NumSegments = bIsLoop ? NumPoints : NumPoints - 1;

    auto WrapIndex = [NumPoints, bIsLoop](int32 Index) -> int32
    {
        return bIsLoop ? (Index % NumPoints + NumPoints) % NumPoints : FMath::Clamp(Index, 0, NumPoints - 1);
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
    OutSamples.Add(Points[bIsLoop ? 0 : NumPoints - 1]);
}

FVector UPatrolVisualizerComponent::EvaluateCatmullRom(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T)
{
    const float T2 = T * T;
    const float T3 = T2 * T;
    return 0.5f * ((2.f * P1) + (-P0 + P2) * T + (2.f * P0 - 5.f * P1 + 4.f * P2 - P3) * T2 + (-P0 + 3.f * P1 - 3.f * P2 + P3) * T3);
}

uint32 UPatrolVisualizerComponent::HashGeometry(const TArray<FVector>& Points, EPatrolType Type)
{
    if (Points.IsEmpty()) return 0u;
    
    uint32 Hash = 2166136261u;
    for (const FVector& P : Points)
    {
        Hash = Hash * 16777619u ^ static_cast<uint32>(P.X);
        Hash = Hash * 16777619u ^ static_cast<uint32>(P.Y);
    }
	
    Hash = Hash * 16777619u ^ static_cast<uint8>(Type);
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