#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UnitSpatialGridSubsystem.generated.h"

class AActor;


UCLASS()
class JUPITERPLUGIN_API UUnitSpatialGridSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "RTS|Spatial")
    void RegisterUnit(AActor* Unit);

    UFUNCTION(BlueprintCallable, Category = "RTS|Spatial")
    void UnregisterUnit(AActor* Unit);

    UFUNCTION(BlueprintCallable, Category = "RTS|Spatial")
    void UpdateUnitPosition(AActor* Unit);

    UFUNCTION(BlueprintCallable, Category = "RTS|Spatial")
    TArray<AActor*> GetUnitsInBounds(const FVector2D& Min, const FVector2D& Max);

    static FIntPoint GetCell(const FVector& Location, float CellSize);

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Spatial")
    float CellSize = 2000.f;

private:
   TMap<FIntPoint, TArray<TWeakObjectPtr<AActor>>> Grid;
   
   TMap<TWeakObjectPtr<AActor>, FIntPoint> UnitCellCache;
};
