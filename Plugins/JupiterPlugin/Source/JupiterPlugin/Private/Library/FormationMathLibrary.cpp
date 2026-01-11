#include "Library/FormationMathLibrary.h"
#include "Math/UnrealMathUtility.h"

void UFormationMathLibrary::GetLineOffsets(int32 Count, float Spacing, TArray<FVector>& OutOffsets)
{
    OutOffsets.Reset();
    if (Count <= 0) return;
    OutOffsets.Reserve(Count);

    const float HalfStep = Spacing * 0.5f;
    const float HalfWidth = (Count - 1) * HalfStep;

    for (int32 i = 0; i < Count; ++i)
    {
        // 0 is Center. (0, Y, 0)
        OutOffsets.Add(FVector(0.f, i * Spacing - HalfWidth, 0.f));
    }
}

void UFormationMathLibrary::GetColumnOffsets(int32 Count, float Spacing, TArray<FVector>& OutOffsets)
{
    OutOffsets.Reset();
    if (Count <= 0) return;
    OutOffsets.Reserve(Count);

    const float HalfStep = Spacing * 0.5f;
    const float HalfDepth = (Count - 1) * HalfStep;

    for (int32 i = 0; i < Count; ++i)
    {
        // (X, 0, 0)
        OutOffsets.Add(FVector(i * Spacing - HalfDepth, 0.f, 0.f));
    }
}

void UFormationMathLibrary::GetSquareOffsets(int32 Count, float Spacing, TArray<FVector>& OutOffsets)
{
    OutOffsets.Reset();
    if (Count <= 0) return;
    OutOffsets.Reserve(Count);

    const int32 Cols = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(Count)));
    // Rows is effectively derived, but we iterate until Count
    const int32 Rows = FMath::CeilToInt(static_cast<float>(Count) / Cols);
    
    const float HalfStep = Spacing * 0.5f;
    const float HalfW = (Cols - 1) * HalfStep;
    const float HalfH = (Rows - 1) * HalfStep;

    for (int32 i = 0; i < Count; ++i)
    {
        const int32 Row = i / Cols;
        const int32 Col = i % Cols;
        
        // Match UnitSpawnComponent logic: X=Row, Y=Col. 
        // Note: UnitSpawnComponent had X * Spacing - HH, Y * Spacing - HW.
        // X corresponds to forward/backward (Depth), Y to right/left (Width).
        OutOffsets.Add(FVector(Row * Spacing - HalfH, Col * Spacing - HalfW, 0.f));
    }
}

void UFormationMathLibrary::GetWedgeOffsets(int32 Count, float Spacing, TArray<FVector>& OutOffsets)
{
    OutOffsets.Reset();
    if (Count <= 0) return;
    OutOffsets.Reserve(Count);

    int32 Placed = 0;
    // Row 0 has 1 unit, Row 1 has 2 units, etc.
    for (int32 Row = 0; Placed < Count; ++Row)
    {
        const int32 UnitsInRow = Row + 1;
        const float RowWidth = (UnitsInRow - 1) * Spacing;
        const float StartY = -RowWidth * 0.5f;
        const float XPos = -Row * Spacing; // Rows go backwards

        for (int32 c = 0; c < UnitsInRow && Placed < Count; ++c)
        {
            OutOffsets.Add(FVector(XPos, StartY + c * Spacing, 0.f));
            Placed++;
        }
    }
}

void UFormationMathLibrary::GetBlobOffsets(int32 Count, float Spacing, TArray<FVector>& OutOffsets)
{
    OutOffsets.Reset();
    if (Count <= 0) return;
    OutOffsets.Reserve(Count);

    // Deterministic Blob:
    // User requested "Seed based on Unit Count/Index" to prevent jittering.
    // If we rely on Count for the seed (like Stream(1337 + Count)), adding 1 unit reshuffles everyone.
    // Ideally, for Index 0..N, the position should depend on Index to be stable.
    
    // First unit at center
    OutOffsets.Add(FVector::ZeroVector);

    constexpr float TwoPi = 6.28318530718f;

    for (int32 i = 1; i < Count; ++i)
    {
        // Use Index to seed the stream for this specific unit's position.
        // This ensures Unit[i] always spawns at the same relative spot regardless of total Count (mostly).
        // However, max radius calculation depends on Count in the original logic. 
        // "MaxRadius = Spacing * Sqrt(Count)". 
        // If we want total stability, we should probably ignore Count for the radius calculation if possible, 
        // or accept slight expansion. 
        // BUT, the request specifically said "prevent jittering".
        // Let's stick to the UnitFormationComponent approach which was per-index.
        
        FRandomStream Stream(i * 137);
        // UnitFormationComponent logic:
        // Angle = (Index / TotalUnits) * TwoPi -> This depends on TotalUnits (Count), so Angle changes if Count changes!
        // That causes jitter.
        
        // UnitSpawnComponent logic:
        // Stream(1337 + Count) -> Reshuffles everything.

        // Fix: Use a fully deterministic spiral or randomized but index-only dependent approach.
        // Simple Vogel Spiral or similar is best for blobs, but let's stick to random-like distribution but independent of Count.
        // Allow Radius to grow with Index (SQRT(Index)) to keep density uniform.
        
        const float Angle = Stream.FRandRange(0.f, TwoPi);
        
        // Radius grows with sqrt(index) to maintain constant density
        // Original logic: Radius = Stream.FRandRange(0.f, MaxRadius).
        // New Stable Logic:
        // We want it to be random-ish but stable.
        
        const float NormalizedRadius = Stream.FRandRange(0.2f, 1.0f);
        // Approximate density: Area ~ Index. Pi*R^2 ~ Index. R ~ Sqrt(Index).
        const float MaxRadiusForIndex = Spacing * FMath::Sqrt(static_cast<float>(i)); 
        // Actually, let's just use Sqrt(i) * Spacing roughly.
        
        // Let's refine: To look like a blob, we just place them.
        // Using `i` as seed.
        const float Dist = Spacing * FMath::Sqrt((float)i) * NormalizedRadius; 
        
        OutOffsets.Add(FVector(Dist * FMath::Cos(Angle), Dist * FMath::Sin(Angle), 0.f));
    }
}

void UFormationMathLibrary::GetCustomGridOffsets(int32 Count, int32 Rows, int32 Cols, float Spacing, TArray<FVector>& OutOffsets)
{
    OutOffsets.Reset();
    if (Count <= 0) return;
    OutOffsets.Reserve(Count);
    
    // Safety
    Cols = FMath::Max(1, Cols);
    Rows = FMath::Max(1, Rows);

    // Center the grid
    const float HalfStep = Spacing * 0.5f;
    const float HalfW = (Cols - 1) * HalfStep;
    const float HalfH = (Rows - 1) * HalfStep;

    // UnitSpawnComponent Custom Logic:
    // W = CustomFormationDimensions.X (Cols?)
    // X = i % W
    // Y = i / W
    // Out: (Y * Spacing - HH, X * Spacing - HW, 0)
    // Note: It treats X dim as Width (Cols).
    
    for (int32 i = 0; i < Count; ++i)
    {
        const int32 X = i % Cols;
        const int32 Y = i / Cols;
        
        // X * Spacing is "Width" (Side to side), Y is Depth.
        // Wait, the original code had:
        // OutOffsets.Add(FVector(Y * Spacing - HH, X * Spacing - HW, 0));
        // This puts Y (Index/Cols) on FVector.X (Forward) and X (Index%Cols) on FVector.Y (Right).
        // This matches (Row, Col).
        
        OutOffsets.Add(FVector(Y * Spacing - HalfH, X * Spacing - HalfW, 0.f));
    }
}

FVector UFormationMathLibrary::CalculateClosestPointOnPath(const FVector& Point, const TArray<FVector>& PathPoints, int32& OutIdx)
{
    OutIdx = 0;
    if (PathPoints.Num() == 0) return Point;
    if (PathPoints.Num() == 1) return PathPoints[0];

    float BestDistSq = MAX_FLT;
    FVector BestPoint = PathPoints[0];

    for (int32 i = 0; i < PathPoints.Num() - 1; ++i)
    {
        const FVector P0 = PathPoints[i];
        const FVector P1 = PathPoints[i + 1];
        
        const FVector Projected = FMath::ClosestPointOnSegment(Point, P0, P1);
        const float DistSq = FVector::DistSquared(Projected, Point);
        
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            BestPoint = Projected;
            OutIdx = i; // The segment starts at i
        }
    }
    
    return BestPoint;
}
