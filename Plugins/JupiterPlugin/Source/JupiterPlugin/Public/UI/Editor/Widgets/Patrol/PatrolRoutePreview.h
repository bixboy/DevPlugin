#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PatrolRoutePreview.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPatrolPreviewClicked);

UCLASS()
class JUPITERPLUGIN_API UPatrolRoutePreview : public UUserWidget
{
	GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPatrolPreviewClicked OnPreviewClicked;
	UFUNCTION(BlueprintCallable, Category = "Patrol Preview")
	void SetPatrolPoints(const TArray<FVector>& WorldPoints, bool bIsLoop = false);

    UFUNCTION(BlueprintCallable, Category = "Patrol Preview")
    void SetMapTexture(UTexture2D* Texture, FVector2D MapWorldSize, FVector2D MapWorldOrigin);

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    
    // --- Components ---
    
    UPROPERTY(meta = (BindWidgetOptional))
    class UImage* MapBackgroundImage;
    
    // --- Config ---

    UPROPERTY(EditAnywhere, Category = "Settings|Map Config")
    UTexture2D* WorldMapTexture;

    UPROPERTY(EditAnywhere, Category = "Settings|Map Config")
    FVector2D WorldMapOrigin = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, Category = "Settings|Map Config")
    FVector2D WorldMapSize = FVector2D(10000.f, 10000.f);
	
	UPROPERTY(EditAnywhere, Category = "Settings|Style")
	FLinearColor PathColor = FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "Settings|Style")
	float LineThickness = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Settings|Style")
	float PointSize = 6.0f;


private:
	TArray<FVector> CachedPoints;
    bool CachedIsLoop = false;
	FVector2D BoundsMin;
	FVector2D BoundsMax;
	
	FVector2D WorldToWidget(FVector WorldPos, FVector2D WidgetSize) const;

    UPROPERTY()
    UMaterialInstanceDynamic* MapMaterialInstance;

    void UpdateMapZoom(const FGeometry& AllottedGeometry) const;

    // View Projection Vars
    mutable FVector2D ViewBoundsMin;
    mutable FVector2D ViewBoundsMax;
    mutable float ViewMaxDim;

};