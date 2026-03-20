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
    if (Count <= 0)
    	return;
	
    OutOffsets.Reserve(Count);

    const int32 Cols = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(Count)));
    const int32 Rows = FMath::CeilToInt(static_cast<float>(Count) / Cols);
    
    const float HalfStep = Spacing * 0.5f;
    const float HalfW = (Cols - 1) * HalfStep;
    const float HalfH = (Rows - 1) * HalfStep;

    for (int32 i = 0; i < Count; ++i)
    {
        const int32 Row = i / Cols;
        const int32 Col = i % Cols;
    	
        OutOffsets.Add(FVector(Row * Spacing - HalfH, Col * Spacing - HalfW, 0.f));
    }
}

void UFormationMathLibrary::GetWedgeOffsets(int32 Count, float Spacing, TArray<FVector>& OutOffsets)
{
    OutOffsets.Reset();
    if (Count <= 0)
    	return;
	
    OutOffsets.Reserve(Count);

    int32 Placed = 0;
    for (int32 Row = 0; Placed < Count; ++Row)
    {
        const int32 UnitsInRow = Row + 1;
        const float RowWidth = (UnitsInRow - 1) * Spacing;
        const float StartY = -RowWidth * 0.5f;
        const float XPos = -Row * Spacing;

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
    if (Count <= 0)
    	return;
	
    OutOffsets.Reserve(Count);
    OutOffsets.Add(FVector::ZeroVector);

    constexpr float TwoPi = 6.28318530718f;

    for (int32 i = 1; i < Count; ++i)
    {
        
        FRandomStream Stream(i * 137);
        const float Angle = Stream.FRandRange(0.f, TwoPi);
    	
        
        const float NormalizedRadius = Stream.FRandRange(0.2f, 1.0f);
        const float MaxRadiusForIndex = Spacing * FMath::Sqrt(static_cast<float>(i)); 
        const float Dist = Spacing * FMath::Sqrt(static_cast<float>(i)) * NormalizedRadius;
    	
        OutOffsets.Add(FVector(Dist * FMath::Cos(Angle), Dist * FMath::Sin(Angle), 0.f));
    }
}

void UFormationMathLibrary::GetCustomGridOffsets(int32 Count, int32 Rows, int32 Cols, float Spacing, TArray<FVector>& OutOffsets)
{
    OutOffsets.Reset();
    if (Count <= 0)
    	return;
	
    OutOffsets.Reserve(Count);
    
    Cols = FMath::Max(1, Cols);
    Rows = FMath::Max(1, Rows);

    const float HalfStep = Spacing * 0.5f;
    const float HalfW = (Cols - 1) * HalfStep;
    const float HalfH = (Rows - 1) * HalfStep;
    
    for (int32 i = 0; i < Count; ++i)
    {
        const int32 X = i % Cols;
        const int32 Y = i / Cols;
        
        OutOffsets.Add(FVector(Y * Spacing - HalfH, X * Spacing - HalfW, 0.f));
    }
}

FVector UFormationMathLibrary::CalculateClosestPointOnPath(const FVector& Point, const TArray<FVector>& PathPoints, int32& OutIdx)
{
    OutIdx = 0;
    if (PathPoints.Num() == 0)
    	return Point;
	
    if (PathPoints.Num() == 1)
    	return PathPoints[0];

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
            OutIdx = i;
        }
    }
    
    return BestPoint;
}
