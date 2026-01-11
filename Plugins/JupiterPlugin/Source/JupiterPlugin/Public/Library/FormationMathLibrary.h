#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FormationMathLibrary.generated.h"

/**
 * Library for calculating generic RTS formation offsets.
 * Centralizes logic previously found in UnitSpawnComponent and UnitFormationComponent.
 */
UCLASS()
class JUPITERPLUGIN_API UFormationMathLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /** Calculates offsets for a Line formation centered around (0,0,0). */
    UFUNCTION(BlueprintPure, Category = "RTS|Formations")
    static void GetLineOffsets(int32 Count, float Spacing, TArray<FVector>& OutOffsets);

    /** Calculates offsets for a Column formation centered around (0,0,0). */
    UFUNCTION(BlueprintPure, Category = "RTS|Formations")
    static void GetColumnOffsets(int32 Count, float Spacing, TArray<FVector>& OutOffsets);

    /** Calculates offsets for a Square/Grid formation centered around (0,0,0). */
    UFUNCTION(BlueprintPure, Category = "RTS|Formations")
    static void GetSquareOffsets(int32 Count, float Spacing, TArray<FVector>& OutOffsets);

    /** Calculates offsets for a Wedge/Triangle formation. */
    UFUNCTION(BlueprintPure, Category = "RTS|Formations")
    static void GetWedgeOffsets(int32 Count, float Spacing, TArray<FVector>& OutOffsets);

    /** 
     * Calculates offsets for a Blob/Circle formation. 
     * Uses a deterministic seed based on Index to ensure stability (prevent jitter).
     */
    UFUNCTION(BlueprintPure, Category = "RTS|Formations")
    static void GetBlobOffsets(int32 Count, float Spacing, TArray<FVector>& OutOffsets);

    /** Calculates offsets for a Custom Rectangle Grid (Rows x Cols). */
    UFUNCTION(BlueprintPure, Category = "RTS|Formations")
    static void GetCustomGridOffsets(int32 Count, int32 Rows, int32 Cols, float Spacing, TArray<FVector>& OutOffsets);

    /** 
     * Finds the closest point on a path segment and returns the connection index (start of segment).
     * @param Point The query point.
     * @param PathPoints The ordered list of path points.
     * @param OutIdx The index of the point preceding the closest segment.
     * @return The closest FVector on the path.
     */
    UFUNCTION(BlueprintPure, Category = "RTS|Math")
    static FVector CalculateClosestPointOnPath(const FVector& Point, const TArray<FVector>& PathPoints, int32& OutIdx);

};
