#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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

    UFUNCTION(BlueprintCallable)
    void SetButtonColor(FLinearColor NewColor);

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Jupiter UI | Interaction")
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
    void UpdateButtonText(const FText& InText);

    UFUNCTION()
    void SetButtonSettings();

    UFUNCTION()
    void UpdateButtonVisuals(bool bForceStateUpdate = false);

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* ButtonTextBlock;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UBorder* ButtonBorder;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UImage* ButtonImage;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	UBorder* BorderImage;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UScaleBox* TextScaleBox;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Text")
    float TextScale = 15.f;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Text")
    TEnumAsByte<EHorizontalAlignment> TextAlignmentHorizontal = EHorizontalAlignment::HAlign_Center;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Text")
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

    // --- Content ---
    
    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Content")
    FText ButtonText;
    
    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Content")
    UTexture2D* ButtonTexture;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Content", meta = (InlineEditConditionToggle))
    uint8 bOverride_ButtonText : 1;

    // --- Content Overrides (Texture) ---

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Texture Settings", meta = (InlineEditConditionToggle))
    uint8 bOverride_Texture_Alpha : 1;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Texture Settings", meta = (InlineEditConditionToggle))
    uint8 bOverride_Texture_Scale : 1;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Texture Settings", meta = (InlineEditConditionToggle))
    uint8 bOverride_Texture_Shift : 1;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Texture Settings", meta = (InlineEditConditionToggle))
    uint8 bOverride_Texture_Size : 1;


    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Texture Settings", meta = (EditCondition = "bOverride_Texture_Alpha"))
    float TextureAlpha = 1.f;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Texture Settings", meta = (EditCondition = "bOverride_Texture_Alpha"))
    float TextureHoverAlpha = 1.f;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Texture Settings", meta = (EditCondition = "bOverride_Texture_Scale"))
    float TextureScale = 0.75f;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Texture Settings", meta = (EditCondition = "bOverride_Texture_Scale"))
    float TextureHoverScale = 1.f;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Texture Settings", meta = (EditCondition = "bOverride_Texture_Shift"))
    float TextureShiftX = 0.f;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Texture Settings", meta = (EditCondition = "bOverride_Texture_Shift"))
    float TextureShiftY = 0.f;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Texture Settings", meta = (EditCondition = "bOverride_Texture_Size"))
    FVector2D TextureSize = FVector2D(32.f, 32.f);

    // --- Appearance ---

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Appearance")
    bool bEnableFill = true;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Appearance")
    bool bAllowFillWhenTextureIsSet = false;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Appearance")
    bool bEnableTexture = true;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Appearance", meta = (InlineEditConditionToggle))
    uint8 bOverride_FillColor : 1;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Appearance", meta = (InlineEditConditionToggle))
    uint8 bOverride_FillHoverColor : 1;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Appearance", meta = (InlineEditConditionToggle))
    uint8 bOverride_BorderColor : 1;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Appearance", meta = (InlineEditConditionToggle))
    uint8 bOverride_BorderHoverColor : 1;

	UPROPERTY(EditAnywhere, Category = "Jupiter UI | Appearance", meta = (InlineEditConditionToggle))
	uint8 bOverride_BorderTexture : 1;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Appearance", meta = (EditCondition = "bOverride_FillColor"))
    FLinearColor FillColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Appearance", meta = (EditCondition = "bOverride_FillHoverColor"))
    FLinearColor FillHoverColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Appearance", meta = (EditCondition = "bOverride_BorderColor"))
    FLinearColor BorderColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, Category = "Jupiter UI | Appearance", meta = (EditCondition = "bOverride_BorderHoverColor"))
    FLinearColor BorderHoverColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Jupiter UI | Appearance", meta = (EditCondition = "bOverride_BorderTexture"))
	FLinearColor BorderTextureColor = FLinearColor::White;

	
    // --- Cached State ---

    UPROPERTY(Transient)
    bool bIsHovered = false;

    UPROPERTY(Transient)
    bool bUseTexture = false;

	UPROPERTY(Transient)
	bool bUseBorderTexture = false;

    UPROPERTY(Transient)
    bool bUseFill = true;

    UPROPERTY(Transient)
    FLinearColor CachedFillColor = FLinearColor::White;

    UPROPERTY(Transient)
    FLinearColor CachedFillHoverColor = FLinearColor::White;

    UPROPERTY(Transient)
    FLinearColor CachedBorderColor = FLinearColor::White;

    UPROPERTY(Transient)
    FLinearColor CachedBorderHoverColor = FLinearColor::White;

    UPROPERTY(Transient)
    float CachedTextureAlpha = 1.f;

    UPROPERTY(Transient)
    float CachedTextureHoverAlpha = 1.f;

    UPROPERTY(Transient)
    float CachedTextureScale = 1.f;

    UPROPERTY(Transient)
    float CachedTextureHoverScale = 1.f;

    UPROPERTY(Transient)
    FVector2D CachedTextureShift = FVector2D::ZeroVector;

    UPROPERTY(Transient)
    FVector2D CachedTextureSize = FVector2D(32.f, 32.f);
    
    UPROPERTY(Transient)
    bool bUsingTextureSizeOverride = false;

#pragma endregion
};
