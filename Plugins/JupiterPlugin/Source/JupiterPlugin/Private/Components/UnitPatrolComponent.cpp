#include "Components/UnitPatrolComponent.h"
#include "Components/LineBatchComponent.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"

UUnitPatrolComponent::UUnitPatrolComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    bAutoActivate = true;
    SetIsReplicatedByDefault(true);
}

void UUnitPatrolComponent::BeginPlay()
{
    Super::BeginPlay();
    RoutesVisCache.SetNum(ActivePatrolRoutes.Num());
}

void UUnitPatrolComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (LineBatchComponent)
    {
        LineBatchComponent->DestroyComponent();
        LineBatchComponent = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}

void UUnitPatrolComponent::TickComponent(float DeltaTime, ELevelTick, FActorComponentTickFunction*)
{
    if (!IsLocallyControlled()) return;

    GlobalVisualTime += DeltaTime;
    UpdateRouteVisuals(DeltaTime);
}

void UUnitPatrolComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UUnitPatrolComponent, ActivePatrolRoutes);
}

bool UUnitPatrolComponent::IsLocallyControlled() const
{
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    return OwnerPawn && OwnerPawn->IsLocallyControlled();
}

bool UUnitPatrolComponent::HasAuthority() const
{
    const AActor* Owner = GetOwner();
    return Owner && Owner->HasAuthority();
}

void UUnitPatrolComponent::EnsureLineBatchComponent()
{
    if (LineBatchComponent && LineBatchComponent->IsRegistered()) return;

    if (AActor* Owner = GetOwner())
    {
        LineBatchComponent = NewObject<ULineBatchComponent>(Owner, TEXT("PatrolVisualizer"));
        if (LineBatchComponent)
        {
            LineBatchComponent->SetupAttachment(Owner->GetRootComponent());
            LineBatchComponent->SetAutoActivate(true);
            LineBatchComponent->RegisterComponent();
            LineBatchComponent->SetHiddenInGame(false);
            LineBatchComponent->bCalculateAccurateBounds = false;
        }
    }
}

void UUnitPatrolComponent::UpdateRouteVisuals(float DeltaSeconds)
{
    EnsureLineBatchComponent();
    if (!LineBatchComponent) return;

    VisualRefreshTimer -= DeltaSeconds;
    if (!bVisualsDirty && VisualRefreshTimer > 0.f)
        return;

    LineBatchComponent->Flush();
    DrawActiveRouteVisualizations();

    VisualRefreshTimer = VisualRefreshInterval;
    bVisualsDirty = false;
}

void UUnitPatrolComponent::DrawActiveRouteVisualizations()
{
    if (!LineBatchComponent || ActivePatrolRoutes.IsEmpty())
        return;

    if (RoutesVisCache.Num() != ActivePatrolRoutes.Num())
        RoutesVisCache.SetNum(ActivePatrolRoutes.Num());

    for (int32 i = 0; i < ActivePatrolRoutes.Num(); ++i)
    {
        const FPatrolRoute& R = ActivePatrolRoutes[i];
        DrawRouteVisualizationCached(R.PatrolPoints, R.bIsLoop, ActiveRouteColor, bAnimateActiveRoutes, RoutesVisCache[i]);
    }
}

void UUnitPatrolComponent::DrawRouteVisualizationCached(const TArray<FVector>& Points, bool bLoop, const FColor& BaseColor, bool bAnimate, FPatrolRouteVisCache& Cache)
{
    if (!LineBatchComponent || Points.Num() < 2) return;

    const uint32 NewHash = HashPointsLight(Points, bLoop);
    const bool bRebuild = (NewHash != Cache.GeometryHash);

    if (bRebuild)
    {
        Cache.ElevatedPoints.Reset(Points.Num());
        for (const FVector& P : Points)
            Cache.ElevatedPoints.Add(P + FVector(0, 0, VisualZOffset));

        BuildSplineSamples(Cache.ElevatedPoints, bLoop, Cache.SplineSamples);
        Cache.GeometryHash = NewHash;
    }

    DrawPointsHalo(Cache.ElevatedPoints, BaseColor);

    const float Pulse = (FMath::Sin(GlobalVisualTime * ColorPulseSpeed) + 1.f) * 0.5f;
    const FLinearColor Core = FLinearColor::LerpUsingHSV(FLinearColor(BaseColor), FLinearColor::White, bAnimate ? Pulse * 0.15f : 0.1f);
    const float Thickness = DebugLineThickness + (bAnimate ? FMath::Sin(GlobalVisualTime * LineWaveSpeed) * LineWaveAmplitude : 0.f);

    DrawPolylineWithGlow(Cache.SplineSamples, Core, Thickness, bAnimate);

    if (ArrowEveryUU > 0.f)
        DrawDirectionArrows(Cache.SplineSamples, BaseColor);
}

void UUnitPatrolComponent::DrawPolylineWithGlow(const TArray<FVector>& Samples, const FLinearColor& CoreColor, float BaseThickness, bool bAnimate) const
{
    const float TimeWobble = bAnimate ? (0.5f + 0.5f * FMath::Sin(GlobalVisualTime * LineWaveSpeed)) : 0.f;

    for (int32 p = GlowPasses; p > 0; --p)
    {
        const float T = (float)p / (float)GlowPasses;
        const float Extra = GlowSize * T * (0.6f + 0.4f * TimeWobble);
        FLinearColor C = CoreColor; C.A = GlowAlpha * (T * T);
        const float Thickness = BaseThickness + Extra;

        for (int32 i = 0; i < Samples.Num() - 1; ++i)
            LineBatchComponent->DrawLine(Samples[i], Samples[i + 1], C.ToFColor(true), SDPG_World, Thickness, 0.f);
    }

    for (int32 i = 0; i < Samples.Num() - 1; ++i)
    {
        const float U = (float)i / (float)(Samples.Num() - 1);
        const FLinearColor C = FLinearColor::LerpUsingHSV(CoreColor, FLinearColor::White, 0.25f * U);
        LineBatchComponent->DrawLine(Samples[i], Samples[i + 1], C.ToFColor(true), SDPG_World, BaseThickness, 0.f);
    }
}

void UUnitPatrolComponent::DrawPointsHalo(const TArray<FVector>& Points, const FColor& BaseColor) const
{
    const float Pulse = (FMath::Sin(GlobalVisualTime * PointPulseSpeed) + 1.f) * 0.5f;
    const float Extra = 8.f * Pulse;

    for (const FVector& P : Points)
    {
        const float R = DebugPointRadius + Extra;
        LineBatchComponent->DrawPoint(P, BaseColor, R, SDPG_World, 0.f);

        DrawDebugCircle(GetWorld(), P, R * 1.4f, 32, FColor(BaseColor.R, BaseColor.G, BaseColor.B, 120),
                        false, 0.f, 0, 2.f, FVector(1,0,0), FVector(0,1,0), false);
    }
}

void UUnitPatrolComponent::DrawDirectionArrows(const TArray<FVector>& Samples, const FColor& Color) const
{
    float Accum = 0.f;
    const float HeadLen = 36.f;
    const float HeadAng = 22.5f * (PI / 180.f);

    for (int32 i = 1; i < Samples.Num(); ++i)
    {
        const FVector A = Samples[i - 1];
        const FVector B = Samples[i];
        const float SegLen = FVector::Distance(A, B);

        Accum += SegLen;
        if (Accum < ArrowEveryUU) continue;
        Accum = 0.f;

        const FVector Dir = (B - A).GetSafeNormal();
        const FVector Right = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal();
        const FVector Tip = B;
        const FVector L = Tip - Dir * HeadLen * FMath::Cos(HeadAng) + (-Right) * HeadLen * FMath::Sin(HeadAng);
        const FVector R = Tip - Dir * HeadLen * FMath::Cos(HeadAng) + Right * HeadLen * FMath::Sin(HeadAng);

        LineBatchComponent->DrawLine(Tip, L, Color, SDPG_World, DebugLineThickness, 0.f);
        LineBatchComponent->DrawLine(Tip, R, Color, SDPG_World, DebugLineThickness, 0.f);
    }
}

void UUnitPatrolComponent::BuildSplineSamples(const TArray<FVector>& Points, bool bLoop, TArray<FVector>& OutSamples) const
{
    OutSamples.Reset();
    const int32 N = Points.Num();
    if (N < 2) { OutSamples = Points; return; }

    const int32 Segs = bLoop ? N : N - 1;
    const int32 Steps = 8;

    auto Wrap = [N,bLoop](int32 Idx){ return bLoop ? (Idx % N + N) % N : FMath::Clamp(Idx, 0, N - 1); };

    OutSamples.Reserve(Segs * Steps + 1);
    for (int32 s = 0; s < Segs; ++s)
    {
        const FVector& P0 = Points[Wrap(s - 1)];
        const FVector& P1 = Points[Wrap(s)];
        const FVector& P2 = Points[Wrap(s + 1)];
        const FVector& P3 = Points[Wrap(s + 2)];

        for (int32 k = 0; k < Steps; ++k)
        {
            const float T = float(k) / float(Steps);
            FVector S = EvaluateCatmullRom(P0, P1, P2, P3, T);
            if (!OutSamples.IsEmpty())
                S = FMath::Lerp(OutSamples.Last(), S, 0.98f);

            if (OutSamples.IsEmpty() || !S.Equals(OutSamples.Last(), 0.5f))
                OutSamples.Add(S);
        }
    }
    OutSamples.Add(Points[bLoop ? 0 : N - 1]);
}

FVector UUnitPatrolComponent::EvaluateCatmullRom(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T)
{
    const float T2 = T * T, T3 = T2 * T;
    return 0.5f * ((2 * P1) + (-P0 + P2) * T + (2*P0 - 5*P1 + 4*P2 - P3) * T2 + (-P0 + 3*P1 - 3*P2 + P3) * T3);
}

uint32 UUnitPatrolComponent::HashPointsLight(const TArray<FVector>& Points, bool bLoop)
{
    if (Points.IsEmpty())
    	return 0u;

    FVector Sum(0);
    for (const FVector& P : Points) Sum += P;
    const FVector First = Points[0];
    const FVector Last = Points.Last();

    uint32 H = 2166136261u;
    auto Mix = [&H](float V) {
        const uint32 X = *reinterpret_cast<const uint32*>(&V);
        H ^= X; H *= 16777619u;
    };
	
    Mix((float)Points.Num());
    Mix(First.X); Mix(Last.X); Mix(Sum.X);
    Mix(First.Y); Mix(Last.Y); Mix(Sum.Y);
    Mix(First.Z); Mix(Last.Z); Mix(Sum.Z);
    Mix(bLoop ? 1.f : 0.f);
	
    return H;
}

void UUnitPatrolComponent::OnRep_ActivePatrolRoutes()
{
    RoutesVisCache.SetNum(ActivePatrolRoutes.Num());
    for (FPatrolRouteVisCache& C : RoutesVisCache) C.GeometryHash = 0;
    bVisualsDirty = true;
}
