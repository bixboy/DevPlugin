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
// STRUCTURE IMPLEMENTATION
// ------------------------------------------------------------

void FPatrolInstanceTracker::SetComponent(UInstancedStaticMeshComponent* InComp)
{
    if (ISMComponent == InComp) 
        return;

    ISMComponent = InComp;
    RouteToIndices.Empty();
    IndexToRoute.Empty();
}

void FPatrolInstanceTracker::Clear()
{
    if (ISMComponent) 
        ISMComponent->ClearInstances();
    
    RouteToIndices.Empty();
    IndexToRoute.Empty();
}

void FPatrolInstanceTracker::AddInstances(const FGuid& RouteID, const TArray<FTransform>& Transforms, const TArray<float>& CustomData1, const TArray<float>& CustomData2, const TArray<float>& CustomData3)
{
    if (!ISMComponent) 
        return;

    TArray<int32>& Indices = RouteToIndices.FindOrAdd(RouteID);
    
    for (int32 i = 0; i < Transforms.Num(); ++i)
    {
        int32 NewIndex = ISMComponent->AddInstance(Transforms[i], true);
        
        if (CustomData1.IsValidIndex(i))
            ISMComponent->SetCustomDataValue(NewIndex, 0, CustomData1[i], true);

        if (CustomData2.IsValidIndex(i)) 
            ISMComponent->SetCustomDataValue(NewIndex, 1, CustomData2[i], true);

        if (CustomData3.IsValidIndex(i)) 
            ISMComponent->SetCustomDataValue(NewIndex, 2, CustomData3[i], true);

        Indices.Add(NewIndex);
        IndexToRoute.Add(NewIndex, RouteID);
    }
}

void FPatrolInstanceTracker::RemoveInstances(const FGuid& RouteID)
{
    if (!ISMComponent || !RouteToIndices.Contains(RouteID)) 
        return;

    TArray<int32> IndicesToRemove = RouteToIndices[RouteID];
    IndicesToRemove.Sort(TGreater<int32>());

    for (int32 IndexToRemove : IndicesToRemove)
    {
        int32 LastIndex = ISMComponent->GetInstanceCount() - 1;
        
        if (IndexToRemove == LastIndex)
        {
            ISMComponent->RemoveInstance(IndexToRemove);
            
            if (IndexToRoute.Contains(IndexToRemove))
                IndexToRoute.Remove(IndexToRemove);
        }
        else
        {
            if (IndexToRoute.Contains(LastIndex))
            {
                 FGuid MoverID = IndexToRoute[LastIndex]; 
                 
                 ISMComponent->RemoveInstance(IndexToRemove);
                 
                 IndexToRoute.Remove(LastIndex);
                 IndexToRoute.Add(IndexToRemove, MoverID);
                 
                 TArray<int32>& MoverIndices = RouteToIndices[MoverID];
                 int32 ParamIndex = MoverIndices.Find(LastIndex);
                 if (ParamIndex != INDEX_NONE)
                 {
                     MoverIndices[ParamIndex] = IndexToRemove;
                 }
            }
            else
            {
                ISMComponent->RemoveInstance(IndexToRemove);
            }
        }
    }
    
    RouteToIndices.Remove(RouteID);
}

bool FPatrolInstanceTracker::FindRouteAndIndex(int32 InstanceIndex, FGuid& OutRouteID, int32& OutPointIndex) const
{
    if (!IndexToRoute.Contains(InstanceIndex))
        return false;

    OutRouteID = IndexToRoute[InstanceIndex];
    if (const TArray<int32>* Indices = RouteToIndices.Find(OutRouteID))
    {
        // The index in the array corresponds to the Point Index
        // because we AddInstances in order of the Points array!
        OutPointIndex = Indices->Find(InstanceIndex);
        return (OutPointIndex != INDEX_NONE);
    }
    return false;
}

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
    if (Routes.IsEmpty() && !CurrentRoutes.IsEmpty())
    {
        CurrentRoutes.Empty();
        
        WaypointTracker.Clear();
        ArrowTracker.Clear();
        PathLineTracker.Clear();
        VisualizationCache.Empty();
        
        for (UTextRenderComponent* TextComp : TextLabelPool)
        {
             if (TextComp) 
                TextComp->SetVisibility(false);
        }
        
        SetVisibility(false);
        return;
    }

    CurrentRoutes = Routes;
    SetVisibility(true);
    CurrentRoutes = Routes;
    SetVisibility(true);
    RebuildGeometry();
}

bool UPatrolVisualizerComponent::GetPatrolPointFromHitIndex(int32 HitIndex, FGuid& OutPatrolID, int32& OutPointIndex) const
{
    return WaypointTracker.FindRouteAndIndex(HitIndex, OutPatrolID, OutPointIndex);
}

void UPatrolVisualizerComponent::UpdatePointPosition(FGuid PatrolID, int32 PointIndex, FVector NewLocation)
{
    for (FPatrolRouteExtended& Route : CurrentRoutes)
    {
        if (Route.PatrolID == PatrolID)
        {
            if (Route.PatrolPoints.IsValidIndex(PointIndex))
            {
                Route.PatrolPoints[PointIndex] = NewLocation;
                RebuildGeometry();
            }
            break;
        }
    }
}



void UPatrolVisualizerComponent::SetVisibility(bool bVisible)
{
    if (WaypointISM)
    	WaypointISM->SetVisibility(bVisible);
	
    if (ArrowISM)
    	ArrowISM->SetVisibility(bVisible);
    
    if (PathLineISM)
        PathLineISM->SetVisibility(bVisible);

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

    WaypointTracker.SetComponent(WaypointISM);
    ArrowTracker.SetComponent(ArrowISM);
    PathLineTracker.SetComponent(PathLineISM);

    TSet<FGuid> IncomingIDs;
    IncomingIDs.Reserve(CurrentRoutes.Num());
    
    for (const auto& R : CurrentRoutes) 
    {
        if (R.PatrolID.IsValid())
            IncomingIDs.Add(R.PatrolID);
    }
    
    TArray<FGuid> IDsToRemove;
    for (auto& Pair : VisualizationCache)
    {
        if (!IncomingIDs.Contains(Pair.Key)) 
        {
            IDsToRemove.Add(Pair.Key);
        }
    }
    
    for (const FGuid& ID : IDsToRemove)
    {
        WaypointTracker.RemoveInstances(ID);
        ArrowTracker.RemoveInstances(ID);
        PathLineTracker.RemoveInstances(ID);
        VisualizationCache.Remove(ID);
    }
    
    for (UTextRenderComponent* TextComp : TextLabelPool)
    {
        if (TextComp)
        	TextComp->SetVisibility(false);
    }

    ActiveTextLabels = 0;

    const int32 MaxRoutes = FMath::Min(CurrentRoutes.Num(), Settings->MaxVisibleRoutes);
    for (int32 i = 0; i < MaxRoutes; ++i)
    {
        RenderRouteCached(CurrentRoutes[i], Settings);
    }
}


void UPatrolVisualizerComponent::RenderRouteCached(const FPatrolRouteExtended& Route, const UPatrolSystemSettings* Settings)
{
    if (Route.PatrolPoints.Num() < 2 || !Route.PatrolID.IsValid())
        return;

    FPatrolVisualizationCache& Cache = VisualizationCache.FindOrAdd(Route.PatrolID);

    const uint32 NewHash = HashGeometry(Route.PatrolPoints, Route.PatrolType);
    bool bGeometryChanged = (NewHash != Cache.GeometryHash);

    if (bGeometryChanged)
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
        
        WaypointTracker.RemoveInstances(Route.PatrolID);
        ArrowTracker.RemoveInstances(Route.PatrolID);
        PathLineTracker.RemoveInstances(Route.PatrolID);
    }

    if (Settings->bEnableFrustumCulling && ShouldCullRoute(Cache.RouteBounds, Settings))
        return;

    FLinearColor RouteColor = GetRouteColor(Route, Cache.State, Settings);
    const bool bIsPreview = (Cache.State == EPatrolVisualizationState::Preview);

    if (bGeometryChanged)
    {
        float Thickness = Settings->LineThickness;
        DrawSplinePolyline(Route.PatrolID, Cache.SplineSamples, RouteColor, Thickness, bIsPreview);

        const bool bRenderWaypoints = (CurrentLODLevel <= 1);
        const bool bRenderArrows = (CurrentLODLevel == 0) && Route.bShowDirectionArrows;
                
        if (bRenderWaypoints)
        {
            DrawWaypoints(Route.PatrolID, Cache.ElevatedPoints, RouteColor, Route, Settings);
        }

        if (bRenderArrows && Settings->ArrowSpacing > 0.f && !bIsPreview)
        {
            DrawDirectionArrows(Route.PatrolID, Cache.SplineSamples, RouteColor, Settings);
        }
    }
    
    const bool bRenderNumbers = (CurrentLODLevel == 0) && Route.bShowWaypointNumbers;
    if (bRenderNumbers)
    {
        for (int32 i = 0; i < Cache.ElevatedPoints.Num(); ++i)
        {
             FString LabelText = FString::FromInt(i + 1);
             FVector Point = Cache.ElevatedPoints[i];
             
            if (ActiveTextLabels < TextLabelPool.Num())
            {
               UTextRenderComponent* TextComp = TextLabelPool[ActiveTextLabels];
               if (TextComp)
               {
                   TextComp->SetVisibility(true);
                   TextComp->SetText(FText::FromString(LabelText));
                   TextComp->SetWorldLocation(Point + FVector(0,0, Settings->WaypointRadius * 2.5f));
                   TextComp->SetTextRenderColor(RouteColor.ToFColor(true));
                   ActiveTextLabels++;
               }
            }
        }
    }

    Cache.LastAccessTime = GlobalAnimationTime;
}

// ------------------------------------------------------------
// DRAWING IMPLEMENTATION
// ------------------------------------------------------------

void UPatrolVisualizerComponent::DrawSplinePolyline(const FGuid& RouteID, const TArray<FVector>& Samples, const FLinearColor& Color, float Thickness, bool bDashed)
{
    if (Samples.Num() < 2) 
        return;

    TArray<FTransform> Transforms;
    TArray<float> R, G, B;
    
    Transforms.Reserve(Samples.Num());
    R.Reserve(Samples.Num());
    G.Reserve(Samples.Num());
    B.Reserve(Samples.Num());

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

        FTransform& T = Transforms.AddDefaulted_GetRef();
        T.SetLocation(MidPoint);
        T.SetRotation(Rotation.Quaternion());
        T.SetScale3D(Scale);
        
        R.Add(Color.R);
        G.Add(Color.G);
        B.Add(Color.B);
    }
    
    PathLineTracker.AddInstances(RouteID, Transforms, R, G, B);
}

void UPatrolVisualizerComponent::DrawWaypoints(const FGuid& RouteID, const TArray<FVector>& Points, const FLinearColor& Color, const FPatrolRouteExtended& Route, const UPatrolSystemSettings* Settings)
{
    if (Points.IsEmpty()) 
        return;

    TArray<FTransform> Transforms;
    TArray<float> R, G, B;
    Transforms.Reserve(Points.Num());
    R.Reserve(Points.Num());
    G.Reserve(Points.Num());
    B.Reserve(Points.Num());

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

        FTransform& T = Transforms.AddDefaulted_GetRef();
        T.SetLocation(Point);
        T.SetScale3D(FVector(1.0f));
        
        R.Add(PointColor.R);
        G.Add(PointColor.G);
        B.Add(PointColor.B);
    }

    WaypointTracker.AddInstances(RouteID, Transforms, R, G, B);
}

void UPatrolVisualizerComponent::DrawDirectionArrows(const FGuid& RouteID, const TArray<FVector>& Samples, const FLinearColor& Color, const UPatrolSystemSettings* Settings)
{
    if (Samples.Num() < 2) 
        return;

    const float ArrowSpacing = Settings->ArrowSpacing;
    float AccumulatedDistance = 0.f;

    TArray<FTransform> Transforms;
    TArray<float> R, G, B;

    for (int32 i = 1; i < Samples.Num(); ++i)
    {
        const FVector& SegStart = Samples[i - 1];
        const FVector& SegEnd = Samples[i];
        float SegLen = FVector::Distance(SegStart, SegEnd);

        AccumulatedDistance += SegLen;
        if (AccumulatedDistance >= ArrowSpacing)
        {
             AccumulatedDistance = 0.f;
             const FVector Dir = (SegEnd - SegStart).GetSafeNormal();
             FRotator Rot = Dir.Rotation() + FRotator(-90.f, 0.f, 0.f);
             
             FTransform& T = Transforms.AddDefaulted_GetRef();
             T.SetLocation(SegEnd);
             T.SetRotation(Rot.Quaternion());
             T.SetScale3D(FVector(0.5f));
             
             R.Add(Color.R);
             G.Add(Color.G);
             B.Add(Color.B);
        }
    }
    
    ArrowTracker.AddInstances(RouteID, Transforms, R, G, B);
}

// ------------------------------------------------------------
// HELPERS
// ------------------------------------------------------------

void UPatrolVisualizerComponent::EnsureComponents(const UPatrolSystemSettings* Settings)
{
    AActor* Owner = GetOwner();
    if (!Owner || !Settings) 
        return;

    auto CreateISM = [&](TObjectPtr<UInstancedStaticMeshComponent>& Comp, FName Name, TSoftObjectPtr<UStaticMesh> MeshAsset, TSoftObjectPtr<UMaterialInterface> MatAsset, bool bIsLine = false)
    {
        if (Comp) 
            return;
        
        Comp = NewObject<UInstancedStaticMeshComponent>(Owner, Name);
        if (!Comp) 
            return;

        Comp->SetupAttachment(Owner->GetRootComponent());
        Comp->SetUsingAbsoluteLocation(true);
        Comp->RegisterComponent();
        if (Name == "PatrolWaypointISM")
        {
            Comp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            Comp->SetCollisionResponseToAllChannels(ECR_Ignore);
            Comp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        }
        else
        {
            Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
        
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
    if (Points.IsEmpty()) 
        return 0u;
    
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
    {
        Bounds += Point;
    }
	
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

    if (Route.RouteColor != FLinearColor(0.f, 1.f, 1.f)) 
        return Route.RouteColor;

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