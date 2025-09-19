#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "HoverPointComponent.generated.h"

USTRUCT(BlueprintType)
struct FHoverPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoverPoint")
	FVector LocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoverPoint")
	float FloatingDistance = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoverPoint")
	float SpringStiffness = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoverPoint")
	float DampingFactor = 10.f;
};


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class STELLARLOCOMOTIONVEHICLESMODULE_API UHoverPointComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UHoverPointComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HoverPoint")
	FHoverPoint HoverPoint;

protected:
	virtual void BeginPlay() override;
	
};
