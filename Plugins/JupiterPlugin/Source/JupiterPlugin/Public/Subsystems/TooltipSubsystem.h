#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/Texture2D.h"
#include "TooltipSubsystem.generated.h"


USTRUCT(BlueprintType)
struct FTooltipData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FText Title;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FText Description;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TObjectPtr<UTexture2D> Icon = nullptr;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TMap<FString, float> Stats;

    FTooltipData() {}
    FTooltipData(FText InTitle, FText InDesc) : Title(InTitle), Description(InDesc) {}
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShowTooltipSignature, const FTooltipData&, Data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHideTooltipSignature);


UCLASS()
class JUPITERPLUGIN_API UTooltipSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "UI|Tooltip")
    void ShowTooltip(const FTooltipData& Data);

    UFUNCTION(BlueprintCallable, Category = "UI|Tooltip")
    void HideTooltip();

    UPROPERTY(BlueprintAssignable, Category = "UI|Tooltip")
    FOnShowTooltipSignature OnShowTooltip;

    UPROPERTY(BlueprintAssignable, Category = "UI|Tooltip")
    FOnHideTooltipSignature OnHideTooltip;

private:
    bool bIsTooltipVisible = false;
};
