#pragma once
#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "PatrolData.generated.h"


UENUM(BlueprintType)
enum class EPatrolType : uint8
{
	Once        UMETA(DisplayName = "Once"),
	Loop        UMETA(DisplayName = "Loop"),
	PingPong    UMETA(DisplayName = "Ping Pong")
};

UENUM(BlueprintType)
enum class EPatrolVisualQuality : uint8
{
	Low      UMETA(DisplayName = "Low - Simple Lines"),
	Medium   UMETA(DisplayName = "Medium - Lines + Waypoints"),
	High     UMETA(DisplayName = "High - Full Effects"),
	Ultra    UMETA(DisplayName = "Ultra - Maximum Quality")
};


USTRUCT(BlueprintType)
struct FPatrolRouteExtended
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Patrol")
	TArray<FVector> PatrolPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Patrol")
	EPatrolType PatrolType = EPatrolType::Loop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Patrol")
	FName RouteName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Patrol|Visuals")
	FLinearColor RouteColor = FLinearColor(0.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Patrol|Behavior", meta = (ClampMin = "0.0"))
	float WaitTimeAtWaypoints = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Patrol|Visuals")
	bool bShowDirectionArrows = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Patrol|Visuals")
	bool bShowWaypointNumbers = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Patrol|Visuals", meta = (ClampMin = "0"))
	int32 RenderPriority = 0;
};

UENUM(BlueprintType)
enum class EPatrolVisualizationState : uint8
{
	Hidden,
	
	Preview,
	
	Active,
	
	Selected
};

USTRUCT(BlueprintType)
struct FPatrolRoute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid PatrolID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector> PatrolPoints;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPatrolType PatrolType = EPatrolType::Loop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName RouteName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor RouteColor = FLinearColor::Blue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WaitTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bShowArrows = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bShowNumbers = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<AActor>> AssignedUnits;
};


USTRUCT(BlueprintType)
struct FPatrolCreationParams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FVector> Points;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<AActor*> Units;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EPatrolType Type = EPatrolType::Loop;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName Name = NAME_None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FLinearColor Color = FLinearColor::Blue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bShowArrows = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bShowNumbers = true;
};

USTRUCT(BlueprintType)
struct FPatrolRouteItem : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	FPatrolRoute RouteData;

	FPatrolRouteItem() {}
	FPatrolRouteItem(const FPatrolRoute& InRoute) : RouteData(InRoute) {}

	void PreReplicatedRemove(const struct FPatrolRouteArray& InArraySerializer);
	void PostReplicatedAdd(const FPatrolRouteArray& InArraySerializer);
	void PostReplicatedChange(const FPatrolRouteArray& InArraySerializer);
};

USTRUCT()
struct FPatrolRouteArray : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FPatrolRouteItem> Items;

	UPROPERTY(NotReplicated)
	TObjectPtr<class UUnitPatrolComponent> OwnerComponent;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FastArrayDeltaSerialize<FPatrolRouteItem, FPatrolRouteArray>(Items, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FPatrolRouteArray> : public TStructOpsTypeTraitsBase2<FPatrolRouteArray>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
