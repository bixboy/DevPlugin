#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FormationMathLibrary.generated.h"


UCLASS()
class JUPITERPLUGIN_API UFormationMathLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintPure, Category = "RTS|Formations")
    static void GetLineOffsets(int32 Count, float Spacing, TArray<FVector>& OutOffsets);

    UFUNCTION(BlueprintPure, Category = "RTS|Formations")
    static void GetColumnOffsets(int32 Count, float Spacing, TArray<FVector>& OutOffsets);

    UFUNCTION(BlueprintPure, Category = "RTS|Formations")
    static void GetSquareOffsets(int32 Count, float Spacing, TArray<FVector>& OutOffsets);

    UFUNCTION(BlueprintPure, Category = "RTS|Formations")
    static void GetWedgeOffsets(int32 Count, float Spacing, TArray<FVector>& OutOffsets);


    UFUNCTION(BlueprintPure, Category = "RTS|Formations")
    static void GetBlobOffsets(int32 Count, float Spacing, TArray<FVector>& OutOffsets);

    UFUNCTION(BlueprintPure, Category = "RTS|Formations")
    static void GetCustomGridOffsets(int32 Count, int32 Rows, int32 Cols, float Spacing, TArray<FVector>& OutOffsets);
	
    UFUNCTION(BlueprintPure, Category = "RTS|Math")
    static FVector CalculateClosestPointOnPath(const FVector& Point, const TArray<FVector>& PathPoints, int32& OutIdx);

};
