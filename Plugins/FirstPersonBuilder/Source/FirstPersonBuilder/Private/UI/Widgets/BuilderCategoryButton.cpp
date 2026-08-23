// Copyright 2026

#include "UI/Widgets/BuilderCategoryButton.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"

UBuilderCategoryButton::UBuilderCategoryButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Visible);
	CurrentBgColor = NormalColor;
	TargetBgColor = NormalColor;
	CurrentIndicatorColor = FLinearColor::Transparent;
	TargetIndicatorColor = FLinearColor::Transparent;
}

void UBuilderCategoryButton::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshStyle();
}

void UBuilderCategoryButton::RefreshStyle()
{
	UpdateTargetColors();
	CurrentBgColor = TargetBgColor;
	CurrentIndicatorColor = TargetIndicatorColor;

	if (BackgroundBorder)
	{
		FSlateBrush FillBrush;
		FillBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
		FillBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		FillBrush.OutlineSettings.CornerRadii = FVector4(CornerRadius, CornerRadius, CornerRadius, CornerRadius);
		FillBrush.TintColor = FSlateColor(FLinearColor::White);
		BackgroundBorder->SetBrush(FillBrush);
		BackgroundBorder->SetPadding(FMargin(0.0f));
		BackgroundBorder->SetBrushColor(CurrentBgColor);
		BackgroundBorder->SetVisibility((CurrentBgColor.A > 0.005f || HoverColor.A > 0.005f || SelectedColor.A > 0.005f)
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (IndicatorBorder && !bIsActionButton)
	{
		IndicatorBorder->SetBrushColor(CurrentIndicatorColor);
		IndicatorBorder->SetVisibility((CurrentIndicatorColor.A > 0.005f)
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (OutlineBorder)
	{
		FSlateBrush OutlineBrush;
		OutlineBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
		OutlineBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		OutlineBrush.OutlineSettings.CornerRadii = FVector4(CornerRadius, CornerRadius, CornerRadius, CornerRadius);
		OutlineBrush.OutlineSettings.Width = (StrokeColor != FLinearColor::Transparent) ? 1.0f : 0.0f;
		OutlineBrush.OutlineSettings.Color = FSlateColor(FLinearColor::White);
		OutlineBrush.TintColor = FSlateColor(FLinearColor::Transparent);
		OutlineBorder->SetBrush(OutlineBrush);
		OutlineBorder->SetPadding(ContentPadding);
		OutlineBorder->SetBrushColor(StrokeColor);
		OutlineBorder->SetVisibility(StrokeColor == FLinearColor::Transparent
			? ESlateVisibility::Collapsed
			: ESlateVisibility::SelfHitTestInvisible);
	}
}

void UBuilderCategoryButton::SetAsActionButton()
{
	bIsActionButton = true;
	if (WidgetTree)
	{
		WidgetTree->RootWidget = nullptr;
		BuildDefaultLayout();
	}
}

void UBuilderCategoryButton::SetText(FText InText)
{
	ButtonText = InText;
	if (TextBlock)
	{
		TextBlock->SetText(InText);
		TextBlock->SetVisibility(InText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
	if (IconImage)
	{
		if (UHorizontalBoxSlot* IconSlot = Cast<UHorizontalBoxSlot>(IconImage->Slot))
		{
			IconSlot->SetPadding(InText.IsEmpty() ? FMargin(0.0f) : FMargin(0.0f, 0.0f, 6.0f, 0.0f));
		}
	}
}

void UBuilderCategoryButton::SetContentPadding(FMargin InPadding)
{
	ContentPadding = InPadding;
	if (ContentHBox)
	{
		if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(ContentHBox->Slot))
		{
			OverlaySlot->SetPadding(InPadding);
		}
	}
}

void UBuilderCategoryButton::SetFontSize(int32 InSize)
{
	if (TextBlock)
	{
		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = InSize;
		TextBlock->SetFont(Font);
	}
}

void UBuilderCategoryButton::SetIcon(UTexture2D* InIcon)
{
	IconTexture = InIcon;
	if (IconImage)
	{
		if (InIcon)
		{
			FSlateBrush Brush;
			Brush.SetResourceObject(InIcon);
			Brush.ImageSize = (IconSize.X > 0 && IconSize.Y > 0) ? IconSize : FVector2D(16.0f, 16.0f);
			Brush.DrawAs = ESlateBrushDrawType::Image;
			IconImage->SetBrush(Brush);
			IconImage->SetColorAndOpacity(FLinearColor::White);
			IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UBuilderCategoryButton::SetIconSize(FVector2D InSize)
{
	IconSize = InSize;
	if (IconImage)
	{
		FSlateBrush Brush = IconImage->GetBrush();
		Brush.ImageSize = (IconSize.X > 0 && IconSize.Y > 0) ? IconSize : FVector2D(16.0f, 16.0f);
		IconImage->SetBrush(Brush);
	}
}

void UBuilderCategoryButton::SetIsSelected(bool bInSelected)
{
	if (bIsSelected == bInSelected)
		return;

	bIsSelected = bInSelected;
	UpdateTargetColors();
	RequestTransitionTick();
}

void UBuilderCategoryButton::UpdateTargetColors()
{
	const EPrismWidgetState State = GetCurrentState();

	if (bIsSelected)
	{
		if (State == EPrismWidgetState::Pressed)
			TargetBgColor = SelectedPressedColor;
		else if (State == EPrismWidgetState::Hovered)
			TargetBgColor = SelectedHoverColor;
		else
			TargetBgColor = SelectedColor;

		TargetIndicatorColor = FLinearColor(0.22f, 0.74f, 0.97f, 1.0f); // Cyan indicator line
	}
	else
	{
		if (State == EPrismWidgetState::Pressed)
			TargetBgColor = PressedColor;
		else if (State == EPrismWidgetState::Hovered)
			TargetBgColor = HoverColor;
		else
			TargetBgColor = NormalColor;

		TargetIndicatorColor = FLinearColor::Transparent;
	}
}

void UBuilderCategoryButton::OnStateChanged(EPrismWidgetState InNewState)
{
	Super::OnStateChanged(InNewState);
	UpdateTargetColors();
	RequestTransitionTick();
}

bool UBuilderCategoryButton::TickTransitions(float DeltaTime)
{
	bool bBaseWorking = Super::TickTransitions(DeltaTime);

	const float InterpSpeed = 12.0f;
	CurrentBgColor = FMath::CInterpTo(CurrentBgColor, TargetBgColor, DeltaTime, InterpSpeed);
	CurrentIndicatorColor = FMath::CInterpTo(CurrentIndicatorColor, TargetIndicatorColor, DeltaTime, InterpSpeed);

	if (BackgroundBorder)
	{
		BackgroundBorder->SetBrushColor(CurrentBgColor);
		BackgroundBorder->SetVisibility((CurrentBgColor.A > 0.005f || TargetBgColor.A > 0.005f)
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (IndicatorBorder && !bIsActionButton)
	{
		IndicatorBorder->SetBrushColor(CurrentIndicatorColor);
		IndicatorBorder->SetVisibility((CurrentIndicatorColor.A > 0.005f || TargetIndicatorColor.A > 0.005f)
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	const bool bBgDone = CurrentBgColor.Equals(TargetBgColor, 0.005f);
	const bool bIndDone = CurrentIndicatorColor.Equals(TargetIndicatorColor, 0.005f);

	return bBaseWorking || !(bBgDone && bIndDone);
}

void UBuilderCategoryButton::BuildDefaultLayout()
{
	UWidgetTree* Tree = WidgetTree;
	if (!Tree) return;

	USizeBox* RootSizeBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	RootSizeBox->SetMinDesiredHeight(bIsActionButton ? 32.0f : 64.0f);
	Tree->RootWidget = RootSizeBox;

	UOverlay* BgOverlay = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	RootSizeBox->AddChild(BgOverlay);

	// 1. Background Fill
	if (!BackgroundBorder)
	{
		BackgroundBorder = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		FSlateBrush BgBrush;
		BgBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
		BackgroundBorder->SetBrush(BgBrush);
	}
	else
	{
		BackgroundBorder->ClearChildren();
	}
	
	UOverlaySlot* FillSlot = BgOverlay->AddChildToOverlay(BackgroundBorder);
	FillSlot->SetHorizontalAlignment(HAlign_Fill);
	FillSlot->SetVerticalAlignment(VAlign_Fill);

	if (!bIsActionButton)
	{
		// 2. Left selection indicator
		IndicatorBorder = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		FSlateBrush IndBrush;
		IndBrush.DrawAs = ESlateBrushDrawType::Box;
		IndBrush.TintColor = FSlateColor(FLinearColor::White);
		IndicatorBorder->SetBrush(IndBrush);
		IndicatorBorder->SetBrushColor(FLinearColor::Transparent);
		
		USizeBox* IndSizeBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		IndSizeBox->SetWidthOverride(4.0f);
		IndSizeBox->AddChild(IndicatorBorder);
		
		UOverlaySlot* IndSlot = BgOverlay->AddChildToOverlay(IndSizeBox);
		IndSlot->SetHorizontalAlignment(HAlign_Left);
		IndSlot->SetVerticalAlignment(VAlign_Fill);

		// Bottom Separator Line
		UBorder* BottomSeparator = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		FSlateBrush SepBrush;
		SepBrush.DrawAs = ESlateBrushDrawType::Box;
		SepBrush.TintColor = FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.08f));
		BottomSeparator->SetBrush(SepBrush);
		
		USizeBox* SepSizeBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		SepSizeBox->SetHeightOverride(1.0f);
		SepSizeBox->AddChild(BottomSeparator);
		
		UOverlaySlot* SepSlot = BgOverlay->AddChildToOverlay(SepSizeBox);
		SepSlot->SetHorizontalAlignment(HAlign_Fill);
		SepSlot->SetVerticalAlignment(VAlign_Bottom);
	}
	else
	{
		if (!OutlineBorder)
			OutlineBorder = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		else
			OutlineBorder->ClearChildren();
		
		UOverlaySlot* OutlineSlot = BgOverlay->AddChildToOverlay(OutlineBorder);
		OutlineSlot->SetHorizontalAlignment(HAlign_Fill);
		OutlineSlot->SetVerticalAlignment(VAlign_Fill);
	}

	RefreshStyle();

	ContentHBox = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	
	UOverlaySlot* ContentSlot = BgOverlay->AddChildToOverlay(ContentHBox);
	ContentSlot->SetHorizontalAlignment(HAlign_Fill);
	ContentSlot->SetVerticalAlignment(VAlign_Fill);
	ContentSlot->SetPadding(bIsActionButton ? FMargin(4.0f, 0.0f) : FMargin(20.0f, 0.0f, 12.0f, 0.0f)); 

	if (!IconImage)
	{
		IconImage = Tree->ConstructWidget<UImage>(UImage::StaticClass());
	}

	if (IconTexture)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(IconTexture);
		Brush.ImageSize = (IconSize.X > 0 && IconSize.Y > 0) ? IconSize : FVector2D(16.0f, 16.0f);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		IconImage->SetBrush(Brush);
		IconImage->SetColorAndOpacity(FLinearColor::White);
		IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		IconImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (IconImage->GetParent())
		IconImage->RemoveFromParent();
	UHorizontalBoxSlot* IconSlot = ContentHBox->AddChildToHorizontalBox(IconImage);
	IconSlot->SetVerticalAlignment(VAlign_Center);
	IconSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));

	// Vertical Box for Category Name + Subtext
	UVerticalBox* TextVBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBoxSlot* TextVBoxSlot = ContentHBox->AddChildToHorizontalBox(TextVBox);
	TextVBoxSlot->SetVerticalAlignment(VAlign_Center);
	if (bIsActionButton)
	{
		TextVBoxSlot->SetHorizontalAlignment(HAlign_Center);
		TextVBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	if (!TextBlock)
	{
		TextBlock = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = bIsActionButton ? 11 : 18;
		Font.TypefaceFontName = FName("Bold");
		TextBlock->SetFont(Font);
		TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.95f, 0.95f, 1.0f)));
	}

	if (TextBlock)
	{
		TextBlock->SetText(ButtonText);
		if (bIsActionButton)
			TextBlock->SetJustification(ETextJustify::Center);
		if (TextBlock->GetParent()) TextBlock->RemoveFromParent();
		TextVBox->AddChildToVerticalBox(TextBlock);
	}
	
	if (!bIsActionButton)
	{
		UTextBlock* SubText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		SubText->SetText(FText::FromString(TEXT("KAC73M")));
		SubText->SetColorAndOpacity(FSlateColor(FLinearColor(0.25f, 0.5f, 0.65f, 0.4f)));
		FSlateFontInfo SubFont = SubText->GetFont();
		SubFont.Size = 9;
		SubText->SetFont(SubFont);
		UVerticalBoxSlot* SubTextSlot = TextVBox->AddChildToVerticalBox(SubText);
		SubTextSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
	}
}
