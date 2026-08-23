// Copyright 2026

#include "UI/Widgets/BuilderCatalogEntry.h"
#include "Interaction/Deployment/Data/PlacementPropData.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/Image.h"
#include "Components/RetainerBox.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Blueprint/WidgetTree.h"
#include "UI/BuilderMenuTheme.h"
#include "Components/Button.h"
#include "Styling/SlateTypes.h"
// Removed UI/BuilderCatalogMenu.h as it's no longer used
// Hardcoded colors moved to UBuilderMenuTheme

UBuilderCatalogEntry::UBuilderCatalogEntry(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bHasScriptImplementedTick = true;
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);
	bIsVertical = true;
	ContentPadding = FMargin(4.0f);
	IconSize = FVector2D(80.0f, 80.0f);
	SetPadding(FMargin(5.0f)); // Adds 5px padding around the entire card (10px gap between cards)
}

void UBuilderCatalogEntry::BuildDefaultLayout()
{
	// DO NOT call Super::BuildDefaultLayout(); we build a custom card layout here.
	UWidgetTree* Tree = WidgetTree;
	if (!Tree) return;

	// Root: Size constraint
	USizeBox* RootSizeBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	RootSizeBox->SetVisibility(ESlateVisibility::Visible);
	RootSizeBox->SetWidthOverride(180.0f);
	RootSizeBox->SetHeightOverride(160.0f);
	Tree->RootWidget = RootSizeBox;

	// Use a UOverlay as the root scaling element
	CardOverlayRoot = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	CardOverlayRoot->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	CardOverlayRoot->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	
	if (USizeBoxSlot* RootSlot = Cast<USizeBoxSlot>(RootSizeBox->AddChild(CardOverlayRoot)))
	{
		RootSlot->SetPadding(FMargin(8.0f)); // Guarantees 8px of transparent space around the card visuals!
	}

	// 1a. Background Fill is handled by an Image widget with the UI Material
	CardBackground = Tree->ConstructWidget<UImage>(UImage::StaticClass());
	CardBackground->SetVisibility(ESlateVisibility::HitTestInvisible);
	
	UOverlaySlot* BgSlot = CardOverlayRoot->AddChildToOverlay(CardBackground);
	BgSlot->SetHorizontalAlignment(HAlign_Fill);
	BgSlot->SetVerticalAlignment(VAlign_Fill);
	
	// Base Overlay (Visuals on top of background)
	UOverlay* CardOverlay = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	CardOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	
	UOverlaySlot* CardOverlaySlot = CardOverlayRoot->AddChildToOverlay(CardOverlay);
	CardOverlaySlot->SetHorizontalAlignment(HAlign_Fill);
	CardOverlaySlot->SetVerticalAlignment(VAlign_Fill);

	// 1b. Static Wireframe Outline (Removed to use Material Outline)
	// 2. Icon (Centered)
	IconImage = Tree->ConstructWidget<UImage>(UImage::StaticClass());
	IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	IconImage->SetColorAndOpacity(FLinearColor::Transparent); // Hide by default to prevent white squares
	if (IconTexture)
	{
		IconImage->SetBrushFromTexture(IconTexture, true);
		FSlateBrush Brush = IconImage->GetBrush();
		if (IconSize.X > 0 && IconSize.Y > 0)
		{
			Brush.ImageSize = IconSize;
			IconImage->SetBrush(Brush);
		}
	}
	
	UOverlaySlot* IconSlot = CardOverlay->AddChildToOverlay(IconImage);
	IconSlot->SetHorizontalAlignment(HAlign_Center);
	IconSlot->SetVerticalAlignment(VAlign_Center);
	IconSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f)); // Push up slightly to leave room for text

	// 3. Text Area (Bottom Left)
	ContentVBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	ContentVBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlaySlot* TextOverlaySlot = CardOverlay->AddChildToOverlay(ContentVBox);
	TextOverlaySlot->SetHorizontalAlignment(HAlign_Left);
	TextOverlaySlot->SetVerticalAlignment(VAlign_Bottom);
	TextOverlaySlot->SetPadding(FMargin(16.0f, 8.0f, 8.0f, 12.0f)); // Increased left/bottom padding to avoid the chamfer mask

	if (!EntryNameText)
	{
		EntryNameText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	}
	if (EntryNameText)
	{
		EntryNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
		EntryNameText->SetText(ButtonText);
		FSlateFontInfo Font = EntryNameText->GetFont();
		Font.Size = 10; 
		Font.TypefaceFontName = FName("Bold");
		EntryNameText->SetFont(Font);
		EntryNameText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		
		if (EntryNameText->GetParent()) EntryNameText->RemoveFromParent();
		UVerticalBoxSlot* TextSlot = ContentVBox->AddChildToVerticalBox(EntryNameText);
		TextSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 2.0f));
	}
	CategoryTagText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CategoryTagText"));
	CategoryTagText->SetVisibility(ESlateVisibility::HitTestInvisible);
	FSlateFontInfo TagFont = CategoryTagText->GetFont();
	TagFont.Size = 8;
	CategoryTagText->SetFont(TagFont);
	CategoryTagText->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.45f, 0.5f, 1.0f)));
	CategoryTagText->SetText(FText::FromString(TEXT("PROP"))); // Default, overridden in NativeOnListItemObjectSet
	
	UVerticalBoxSlot* TagSlot = ContentVBox->AddChildToVerticalBox(CategoryTagText);
	TagSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));

	// Invisible Interaction Layer is REMOVED!
	// UPrismButtonBase natively handles Hover, Click, and state management.
}

void UBuilderCatalogEntry::ApplyTheme(UBuilderMenuTheme* InTheme)
{
	if (!InTheme) return;
	CachedTheme = InTheme;

	if (IsValid(CardBackground))
	{
		CardMaterial = InTheme->CardRetainerMaterial;
		if (CardMaterial)
		{
			CardMID = UMaterialInstanceDynamic::Create(CardMaterial, this);
			CardBackground->SetBrushFromMaterial(CardMID);
			// Fix: If the theme still uses the old percentage value (e.g. 0.04), force it to 2.0 pixels
			float FinalOutlineThick = (InTheme->OutlineThickness < 1.0f) ? 2.0f : InTheme->OutlineThickness;
			CardMID->SetScalarParameterValue(TEXT("OutlineThickness"), FinalOutlineThick);
			
			CardMID->SetVectorParameterValue(TEXT("CardSize"), FVector(180.0f, 160.0f, 0.0f));
		}
	}
}

void UBuilderCatalogEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	PropData = Cast<UPlacementPropData>(ListItemObject);
	if (!PropData)
		return;

	// Build layout if it hasn't been built yet so pointers are valid
	if (!CardOverlayRoot)
		BuildDefaultLayout();

	// Item name — uppercase, truncated by widget bounds
	FString NameStr = PropData->EntityID.ToString();
	if (NameStr.IsEmpty() || NameStr == TEXT("None"))
	{
		NameStr = PropData->GetName();
	}
	FText ItemName = FText::FromString(NameStr.ToUpper());
	
	if (EntryNameText)
	{
		EntryNameText->SetText(ItemName);
	}

	// Thumbnail
	if (IconImage)
	{
		if (UTexture2D* Tex = PropData->EntityThumbnail.LoadSynchronous())
		{
			IconTexture = Tex;
			IconImage->SetBrushFromTexture(IconTexture, true);
			
			// Force the brush size just in case bMatchSize fails or texture size is 0
			FSlateBrush Brush = IconImage->GetBrush();
			if (IconSize.X > 0 && IconSize.Y > 0)
			{
				Brush.ImageSize = IconSize;
				IconImage->SetBrush(Brush);
			}
			
			IconImage->SetColorAndOpacity(FLinearColor::White); // Show the texture
			IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			IconImage->SetColorAndOpacity(FLinearColor::Transparent); // Hide
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// Category tag (extract last segment of first gameplay tag)
	if (CategoryTagText)
	{
		FString TagStr = TEXT("PROP");
		if (PropData->EntityTags.IsValid() && PropData->EntityTags.Num() > 0)
		{
			TagStr = PropData->EntityTags.First().GetTagName().ToString();
			int32 DotIdx;
			if (TagStr.FindLastChar(TEXT('.'), DotIdx))
				TagStr = TagStr.Mid(DotIdx + 1);
			TagStr = TagStr.ToUpper();
		}
		CategoryTagText->SetText(FText::FromString(TagStr));
	}

	// Initial visual state
	TargetHoverInterp = 0.0f;
	TargetPressInterp = 0.0f;
	RequestTransitionTick();
}

void UBuilderCatalogEntry::NativeOnItemSelectionChanged(bool bIsSelected)
{
	bEntrySelected = bIsSelected;
	if (CachedTheme && CardMID)
	{
		const FLinearColor TargetColor = bEntrySelected ? CachedTheme->PrimaryHighlightColor : CachedTheme->WireframeColor;
		const FLinearColor TargetBgColor = bEntrySelected ? CachedTheme->EntrySelectedBgColor : CachedTheme->EntryBgColor;
		CardMID->SetVectorParameterValue(TEXT("OutlineColor"), TargetColor);
		CardMID->SetVectorParameterValue(TEXT("FillColor"), TargetBgColor);
	}
	RequestTransitionTick();
}

void UBuilderCatalogEntry::OnStateChanged(EPrismWidgetState InNewState)
{
	Super::OnStateChanged(InNewState);

	TargetHoverInterp = (InNewState == EPrismWidgetState::Hovered) ? 1.0f : 0.0f;
	TargetPressInterp = (InNewState == EPrismWidgetState::Pressed) ? 1.0f : 0.0f;
	
	RequestTransitionTick();
}

bool UBuilderCatalogEntry::TickTransitions(float DeltaTime)
{
	bool bBaseWorking = Super::TickTransitions(DeltaTime);
	
	if (!GetWorld())
		return bBaseWorking;

	// Ultra premium smooth animation: Linear time update + EaseInOut curve
	const float HoverSpeed = 5.0f; // 200ms transition
	if (TargetHoverInterp > CurrentHoverInterp)
		CurrentHoverInterp = FMath::Min(CurrentHoverInterp + DeltaTime * HoverSpeed, 1.0f);
	else
		CurrentHoverInterp = FMath::Max(CurrentHoverInterp - DeltaTime * HoverSpeed, 0.0f);

	const float PressSpeed = 10.0f; // 100ms
	if (TargetPressInterp > CurrentPressInterp)
		CurrentPressInterp = FMath::Min(CurrentPressInterp + DeltaTime * PressSpeed, 1.0f);
	else
		CurrentPressInterp = FMath::Max(CurrentPressInterp - DeltaTime * PressSpeed, 0.0f);

	const float SmoothHover = FMath::InterpEaseInOut(0.0f, 1.0f, CurrentHoverInterp, 2.0f);
	const float SmoothPress = FMath::InterpEaseInOut(0.0f, 1.0f, CurrentPressInterp, 2.0f);

	const float HoverScale = 1.0f + (SmoothHover * 0.05f);
	const float PressScale = 1.0f - (SmoothPress * 0.04f);
	const float FinalScale = HoverScale * PressScale;
	
	if (IsValid(CardOverlayRoot))
	{
		CardOverlayRoot->SetRenderScale(FVector2D(FinalScale, FinalScale));
	}

	// Calculate highlight colors using Data Asset (Theme) colors
	if (CachedTheme && CardMID)
	{
		const FLinearColor TargetColor = bEntrySelected
			? CachedTheme->PrimaryHighlightColor
			: FMath::Lerp(CachedTheme->WireframeColor, FLinearColor::White.CopyWithNewOpacity(0.9f), CurrentHoverInterp);
		CardMID->SetVectorParameterValue(TEXT("OutlineColor"), TargetColor);
		
		const FLinearColor TargetBgColor = bEntrySelected
			? CachedTheme->EntrySelectedBgColor
			: FMath::Lerp(CachedTheme->EntryBgColor, CachedTheme->EntryHoverBgColor, CurrentHoverInterp);
		CardMID->SetVectorParameterValue(TEXT("FillColor"), TargetBgColor);
	}

	const bool bHoverDone = FMath::IsNearlyEqual(CurrentHoverInterp, TargetHoverInterp, 0.001f);
	const bool bPressDone = FMath::IsNearlyEqual(CurrentPressInterp, TargetPressInterp, 0.001f);
	
	// Only continue ticking during active interpolation (0% CPU when settled)
	return bBaseWorking || !(bHoverDone && bPressDone);
}

// End of PrismUI refactor
