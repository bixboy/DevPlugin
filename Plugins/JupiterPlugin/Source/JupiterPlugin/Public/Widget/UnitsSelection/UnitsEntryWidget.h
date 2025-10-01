﻿#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UnitsEntryWidget.generated.h"

class UCustomButtonWidget;
class UUnitsSelectionDataAsset;
class ASoldierRts;
class UTextBlock;
class UImage;
class UUnitSpawnComponent;

UCLASS()
class JUPITERPLUGIN_API UUnitsEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
        virtual void NativeOnInitialized() override;

        UFUNCTION()
        void InitEntry(UUnitsSelectionDataAsset* DataAsset);

        FText GetUnitDisplayName() const { return CachedUnitName; }
        const TArray<FName>& GetUnitTags() const { return UnitTags; }
        bool MatchesSearch(const FString& SearchLower) const;
        bool HasTag(FName Tag) const;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UCustomButtonWidget* UnitButton;

protected:

        UFUNCTION()
        void OnUnitSelected(UCustomButtonWidget* Button, int Index);

        UPROPERTY()
        TSubclassOf<ASoldierRts> UnitClass;

        UPROPERTY()
        UUnitSpawnComponent* SpawnComponent;

        UPROPERTY()
        FText CachedUnitName;

        UPROPERTY()
        TArray<FName> UnitTags;
};
