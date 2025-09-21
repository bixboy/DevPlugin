#pragma once
#include "CoreMinimal.h"
#include "Components/Button.h"
#include "CustomButtonWidget.generated.h"

class UScaleBox;
class UBorder;
class UTextBlock;
class UImage;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FButtonClicked, UCustomButtonWidget*, Button, int, Index);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FButtonHovered, UCustomButtonWidget*, Button, int, Index);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FButtonUnHovered, UCustomButtonWidget*, Button, int, Index);

UCLASS()
class JUPITERPLUGIN_API UCustomButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	
	UFUNCTION(BlueprintCallable)
	void SetButtonText(const FText& InText);

	UFUNCTION(BlueprintCallable)
	void ToggleButtonIsSelected(bool bNewValue);

	UFUNCTION(BlueprintCallable)
	void SetButtonTexture(UTexture2D* NewTexture);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Button")
	int ButtonIndex;

	UPROPERTY(BlueprintReadOnly, BlueprintCallable)
	FButtonClicked OnButtonClicked;
	
	UPROPERTY(BlueprintReadOnly, BlueprintCallable)
	FButtonHovered OnButtonHovered;
	
	UPROPERTY(BlueprintReadOnly, BlueprintCallable)
	FButtonUnHovered OnButtonUnHovered;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* Button;

protected:
	UFUNCTION()
	void ApplyMaterial() const;
	
	UFUNCTION()
	void UpdateButtonText(const FText& InText);
	
	UFUNCTION()
	void SetButtonSettings() const;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* ButtonTextBlock;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UBorder* ButtonBorder;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UScaleBox* TextScaleBox;

	UPROPERTY(EditAnywhere, Category = "Button|Text")
	float TextScale = 15.f;

	UPROPERTY(EditAnywhere, Category = "Button|Text")
	TEnumAsByte<EHorizontalAlignment> TextAlignmentHorizontal = EHorizontalAlignment::HAlign_Center;
	
	UPROPERTY(EditAnywhere, Category = "Button|Text")
	TEnumAsByte<EVerticalAlignment> TextAlignmentVertical = EVerticalAlignment::VAlign_Center;

	UPROPERTY()
	bool bIsSelected = false;

//-------------------- Hover & Press ----------------------
#pragma region Hover & Press Settings

	UFUNCTION()
	void OnCustomUIButtonClickedEvent();
	
	UFUNCTION()
	void OnCustomUIButtonHoveredEvent();
	
	UFUNCTION()
	void OnCustomUIButtonUnHoveredEvent();
	
	UPROPERTY(EditAnywhere, Category = Button)
	UMaterialInstance* Material;
	
	UPROPERTY(EditAnywhere, Category = "Button|Texture")
	UTexture2D* ButtonTexture;

	UPROPERTY(EditAnywhere, Category = Button, meta = (EditCondition = "bOverride_ButtonText"))
	FText ButtonText;

	// Meta bool
	UPROPERTY(EditAnywhere, Category = "Button", meta = (InlineEditConditionToggle))
	uint8 bOverride_Texture_Alpha : 1;
	
	UPROPERTY(EditAnywhere, Category = "Button", meta = (InlineEditConditionToggle))
	uint8 bOverride_Texture_Scale : 1;
	
	UPROPERTY(EditAnywhere, Category = "Button", meta = (InlineEditConditionToggle))
	uint8 bOverride_Texture_Shift : 1;
	
    UPROPERTY(EditAnywhere, Category = Button, meta = (InlineEditConditionToggle))
    uint8 bOverride_ButtonText : 1;

    UPROPERTY(EditAnywhere, Category = "Button|Appearance", meta = (InlineEditConditionToggle))
    uint8 bOverride_FillColor : 1;

    UPROPERTY(EditAnywhere, Category = "Button|Appearance", meta = (InlineEditConditionToggle))
    uint8 bOverride_FillHoverColor : 1;

    UPROPERTY(EditAnywhere, Category = "Button|Appearance", meta = (InlineEditConditionToggle))
    uint8 bOverride_BorderColor : 1;

    UPROPERTY(EditAnywhere, Category = "Button|Appearance", meta = (InlineEditConditionToggle))
    uint8 bOverride_BorderHoverColor : 1;

    // Texture Alpha
    UPROPERTY(EditAnywhere, Category = Button, meta = (EditCondition = "bOverride_Texture_Alpha"))
    float TextureAlpha = 1.f;
    UPROPERTY(EditAnywhere, Category = Button, meta = (EditCondition = "bOverride_Texture_Alpha"))
	float TextureHoverAlpha = 1.f;

	// Texture Scale
	UPROPERTY(EditAnywhere, Category = Button, meta = (EditCondition = "bOverride_Texture_Scale"))
	float TextureScale = 0.75f;
	UPROPERTY(EditAnywhere, Category = Button, meta = (EditCondition = "bOverride_Texture_Scale"))
	float TextureHoverScale = 1.f;

	// Texture Shift
	UPROPERTY(EditAnywhere, Category = Button, meta = (EditCondition = "bOverride_Texture_Shift"))
    float TextureShiftX = 0.f;
    UPROPERTY(EditAnywhere, Category = Button, meta = (EditCondition = "bOverride_Texture_Shift"))
    float TextureShiftY = 0.f;

    UPROPERTY(EditAnywhere, Category = "Button|Appearance", meta = (EditCondition = "bOverride_FillColor"))
    FLinearColor FillColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, Category = "Button|Appearance", meta = (EditCondition = "bOverride_FillHoverColor"))
    FLinearColor FillHoverColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, Category = "Button|Appearance", meta = (EditCondition = "bOverride_BorderColor"))
    FLinearColor BorderColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, Category = "Button|Appearance", meta = (EditCondition = "bOverride_BorderHoverColor"))
    FLinearColor BorderHoverColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, Category = "Button|Appearance")
    bool bEnableFill = true;

    UPROPERTY(EditAnywhere, Category = "Button|Appearance")
    bool bAllowFillWhenTextureIsSet = false;

    UPROPERTY(EditAnywhere, Category = "Button|Appearance")
    bool bEnableTexture = true;

#pragma endregion
};