// Copyright 2026

#include "UI/BuilderCatalogMenu.h"
#include "UI/BuilderCatalogTypes.h"
#include "UI/BuilderMenuTheme.h"
#include "UI/Widgets/BuilderCatalogEntry.h"
#include "UI/Widgets/BuilderCategoryButton.h"
#include "Objects/PlayerConstructionObject.h"
#include "Objects/ConstructionSystemSettings.h"
#include "Interaction/Deployment/Data/PlacementPropData.h"
#include "DataClasses/Entities/Items/PDA_ItemClass.h"
#include "Components/TileView.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/BackgroundBlur.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

// ─────────────────────────────────────────────────────────────────────────────
// Color Tokens (Fallback values)
// ─────────────────────────────────────────────────────────────────────────────
namespace CatalogUI
{
	static const FLinearColor Cyan            = FLinearColor(0.22f, 0.74f, 0.97f, 1.0f);
	static const FLinearColor MainBg          = FLinearColor(0.06f, 0.08f, 0.10f, 0.92f);
	static const FLinearColor SidebarBg       = FLinearColor(0.04f, 0.06f, 0.08f, 0.40f);
	static const FLinearColor BottomBarBg     = FLinearColor(0.05f, 0.07f, 0.10f, 0.85f);
	static const FLinearColor CatBtnNormal    = FLinearColor(0.0f,  0.0f,  0.0f,  0.0f);
	static const FLinearColor CatBtnActive    = FLinearColor(0.10f, 0.18f, 0.28f, 0.95f);
	static const FLinearColor TextPrimary     = FLinearColor(0.95f, 0.95f, 0.95f, 1.0f);
	static const FLinearColor TextSecondary   = FLinearColor(0.50f, 0.58f, 0.65f, 1.0f);
	static const FLinearColor Orange          = FLinearColor(0.98f, 0.45f, 0.09f, 1.0f);
	static const FLinearColor OrangeFill      = FLinearColor(0.98f, 0.45f, 0.09f, 0.25f);
	static const FLinearColor SearchBg        = FLinearColor(0.05f, 0.06f, 0.08f, 0.4f);
	static const FLinearColor WireframeBorder = FLinearColor(0.35f, 0.45f, 0.55f, 0.35f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Proxies Implementation
// ─────────────────────────────────────────────────────────────────────────────
void UBuilderCategoryProxy::OnClicked(UPrismButtonBase* Button)
{
	if (Menu.IsValid())
	{
		Menu->PopulateGridForCategory(CategoryTag);
	}
}

void UBuilderPrimaryCategoryProxy::OnClicked(UPrismButtonBase* Button)
{
	if (Menu.IsValid())
	{
		Menu->SelectPrimaryCategory(PrimaryIndex);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor & Lifecycle
// ─────────────────────────────────────────────────────────────────────────────
UBuilderCatalogMenu::UBuilderCatalogMenu(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);
}

void UBuilderCatalogMenu::NativeConstruct()
{
	Super::NativeConstruct();

	if (MainBackground)
		MainBackground->SetVisibility(ESlateVisibility::Visible);

	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(true);
	}
}

void UBuilderCatalogMenu::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ScrambleTimerHandle);
		World->GetTimerManager().ClearTimer(TabSlideTimerHandle);
	}
	ActiveScrambleTargets.Empty();

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}

	Super::NativeDestruct();
}

// ─────────────────────────────────────────────────────────────────────────────
// Master Layout Builder (Modular Assembly)
// ─────────────────────────────────────────────────────────────────────────────
void UBuilderCatalogMenu::BuildDefaultLayout()
{
	UWidgetTree* Tree = WidgetTree;
	if (!Tree)
		return;

	// Root Overlay (Spans entire screen)
	UOverlay* RootOverlay = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("CatalogRootOverlay"));
	Tree->RootWidget = RootOverlay;

	// Window Container
	USizeBox* WindowSizeBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("WindowSizeBox"));
	WindowSizeBox->SetWidthOverride(1240.0f);
	WindowSizeBox->SetHeightOverride(780.0f);

	UOverlaySlot* WindowSlot = RootOverlay->AddChildToOverlay(WindowSizeBox);
	WindowSlot->SetHorizontalAlignment(HAlign_Center);
	WindowSlot->SetVerticalAlignment(VAlign_Center);

	UVerticalBox* WindowVBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("WindowVBox"));
	WindowSizeBox->AddChild(WindowVBox);

	// 1. Top Header Bar
	BuildWindowHeader(WindowVBox);

	// 2. Main Window Framed Container (Sidebar + Right Area)
	MainBackground = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MainBackground"));
	FSlateBrush FrameBrush;
	FrameBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	FrameBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	FrameBrush.OutlineSettings.CornerRadii = FVector4(12.0f, 12.0f, 12.0f, 12.0f);
	FrameBrush.OutlineSettings.Width = 1.0f;
	FrameBrush.OutlineSettings.Color = FSlateColor(FLinearColor(0.25f, 0.40f, 0.55f, 0.5f));
	FrameBrush.TintColor = FSlateColor(CatalogUI::MainBg);
	MainBackground->SetBrush(FrameBrush);
	MainBackground->SetPadding(FMargin(0.0f));

	UVerticalBoxSlot* FrameSlot = WindowVBox->AddChildToVerticalBox(MainBackground);
	FrameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UHorizontalBox* ContentHBox = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ContentHBox"));
	MainBackground->AddChild(ContentHBox);

	// 3. Left Sidebar
	BuildLeftSidebar(ContentHBox);

	// 4. Vertical Separator Line
	UBorder* SeparatorLine = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SeparatorLine"));
	SeparatorLine->SetBrushColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.08f));
	USizeBox* SepSizeBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	SepSizeBox->SetWidthOverride(1.0f);
	SeparatorLine->AddChild(SepSizeBox);
	ContentHBox->AddChildToHorizontalBox(SeparatorLine);

	// 5. Right Main Area (Top Tabs + Grid + Detail Panel)
	BuildRightArea(ContentHBox);

	// 6. Window Footer (Back Button)
	BuildWindowFooter(WindowVBox);
}

void UBuilderCatalogMenu::BuildWindowHeader(UVerticalBox* InRootVBox)
{
	UWidgetTree* Tree = WidgetTree;

	UHorizontalBox* TopBar = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TopBar"));
	UVerticalBoxSlot* TopBarSlot = InRootVBox->AddChildToVerticalBox(TopBar);
	TopBarSlot->SetPadding(FMargin(4.0f, 0.0f, 4.0f, 6.0f));
	TopBarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	// Breadcrumb / Title Area
	UVerticalBox* TitleVBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TitleVBox"));
	TopBar->AddChildToHorizontalBox(TitleVBox)->SetVerticalAlignment(VAlign_Center);

	TitleText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(FText::FromString(TEXT("CONSTRUCT : CATALOG")));
	TitleText->SetColorAndOpacity(FSlateColor(CatalogUI::Cyan));
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 13;
	TitleFont.TypefaceFontName = FName("Bold");
	TitleText->SetFont(TitleFont);
	TitleVBox->AddChildToVerticalBox(TitleText);

	SubtitleText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SubtitleText"));
	SubtitleText->SetText(FText::FromString(TEXT("DEPLOYABLE LOGISTICS & INFRASTRUCTURE")));
	SubtitleText->SetColorAndOpacity(FSlateColor(CatalogUI::TextSecondary));
	FSlateFontInfo SubFont = SubtitleText->GetFont();
	SubFont.Size = 9;
	SubtitleText->SetFont(SubFont);
	TitleVBox->AddChildToVerticalBox(SubtitleText);

	// Spacer
	USpacer* TopSpacer = Tree->ConstructWidget<USpacer>(USpacer::StaticClass());
	TopBar->AddChildToHorizontalBox(TopSpacer)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// ─── TACTICAL SEARCH BAR ─────────────────────────────────────────────
	UBorder* SearchBorder = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SearchBorder"));
	FSlateBrush SearchBrush;
	SearchBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	SearchBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	SearchBrush.OutlineSettings.CornerRadii = FVector4(5.0f, 5.0f, 5.0f, 5.0f);
	SearchBrush.OutlineSettings.Width = 1.0f;
	SearchBrush.OutlineSettings.Color = FSlateColor(FLinearColor(0.25f, 0.40f, 0.55f, 0.5f));
	SearchBrush.TintColor = FSlateColor(FLinearColor(0.02f, 0.04f, 0.07f, 0.85f));
	SearchBorder->SetBrush(SearchBrush);
	SearchBorder->SetPadding(FMargin(10.0f, 3.0f, 10.0f, 3.0f));

	USizeBox* SearchSize = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SearchSizeBox"));
	SearchSize->SetWidthOverride(280.0f);
	SearchSize->SetMinDesiredHeight(32.0f);
	SearchSize->AddChild(SearchBorder);

	UHorizontalBox* SearchHBox = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SearchHBox"));
	SearchBorder->AddChild(SearchHBox);

	// Search Glyph / Reticle Icon
	UTextBlock* SearchIcon = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SearchIcon"));
	SearchIcon->SetText(FText::FromString(TEXT("\x25C8"))); // ◈ Sci-fi reticle diamond icon
	SearchIcon->SetColorAndOpacity(FSlateColor(CatalogUI::Cyan));
	FSlateFontInfo IconFont = SearchIcon->GetFont();
	IconFont.Size = 10;
	SearchIcon->SetFont(IconFont);
	UHorizontalBoxSlot* IconSlot = SearchHBox->AddChildToHorizontalBox(SearchIcon);
	IconSlot->SetVerticalAlignment(VAlign_Center);
	IconSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));

	// Search Text Input
	SearchBox = Tree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("SearchBox"));
	SearchBox->SetHintText(FText::FromString(TEXT("FILTER SCHEMATICS...")));
	SearchBox->OnTextChanged.AddDynamic(this, &UBuilderCatalogMenu::HandleSearchTextChanged);

	// Transparent Slate Style for crisp custom border integration
	FEditableTextBoxStyle SearchStyle;
	FSlateBrush TransparentBrush;
	TransparentBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
	TransparentBrush.TintColor = FSlateColor(FLinearColor::Transparent);

	SearchStyle.SetBackgroundImageNormal(TransparentBrush);
	SearchStyle.SetBackgroundImageHovered(TransparentBrush);
	SearchStyle.SetBackgroundImageFocused(TransparentBrush);
	SearchStyle.SetBackgroundImageReadOnly(TransparentBrush);

	FSlateFontInfo SearchFont = SearchIcon->GetFont();
	SearchFont.Size = 10;
	SearchFont.TypefaceFontName = FName("Regular");
	SearchStyle.SetFont(SearchFont);
	SearchStyle.SetForegroundColor(FSlateColor(CatalogUI::TextPrimary));
	SearchStyle.SetFocusedForegroundColor(FSlateColor(FLinearColor::White));
	SearchStyle.SetBackgroundColor(FSlateColor(FLinearColor::Transparent));
	SearchStyle.SetPadding(FMargin(0.0f, 2.0f));
	SearchBox->SetWidgetStyle(SearchStyle);

	UHorizontalBoxSlot* BoxSlot = SearchHBox->AddChildToHorizontalBox(SearchBox);
	BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	BoxSlot->SetVerticalAlignment(VAlign_Center);

	// Shortcut / Mode Indicator Tag
	UTextBlock* SearchTag = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SearchTag"));
	SearchTag->SetText(FText::FromString(TEXT("SCHM")));
	SearchTag->SetColorAndOpacity(FSlateColor(FLinearColor(0.30f, 0.45f, 0.60f, 0.5f)));
	FSlateFontInfo TagFont = SearchTag->GetFont();
	TagFont.Size = 8;
	TagFont.TypefaceFontName = FName("Bold");
	SearchTag->SetFont(TagFont);
	UHorizontalBoxSlot* TagSlot = SearchHBox->AddChildToHorizontalBox(SearchTag);
	TagSlot->SetVerticalAlignment(VAlign_Center);
	TagSlot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));

	UHorizontalBoxSlot* SearchSlot = TopBar->AddChildToHorizontalBox(SearchSize);
	SearchSlot->SetVerticalAlignment(VAlign_Center);
}

void UBuilderCatalogMenu::BuildLeftSidebar(UHorizontalBox* InContentHBox)
{
	UWidgetTree* Tree = WidgetTree;

	USizeBox* SidebarSizeBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SidebarSizeBox"));
	SidebarSizeBox->SetWidthOverride(210.0f);

	UHorizontalBoxSlot* SidebarSlot = InContentHBox->AddChildToHorizontalBox(SidebarSizeBox);
	SidebarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	SidebarSlot->SetVerticalAlignment(VAlign_Fill);

	CategorySidebar = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CategorySidebar"));
	SidebarSizeBox->AddChild(CategorySidebar);
}

void UBuilderCatalogMenu::BuildRightArea(UHorizontalBox* InContentHBox)
{
	UWidgetTree* Tree = WidgetTree;

	UVerticalBox* RightAreaVBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RightAreaVBox"));
	UHorizontalBoxSlot* RightAreaSlot = InContentHBox->AddChildToHorizontalBox(RightAreaVBox);
	RightAreaSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	RightAreaSlot->SetVerticalAlignment(VAlign_Fill);
	RightAreaSlot->SetPadding(FMargin(16.0f, 8.0f, 16.0f, 12.0f));

	// 1. Top Primary Category Segmented Capsule
	BuildTopCategoryCapsule(RightAreaVBox);

	// 2. Prop Cards Grid
	BuildPropGrid(RightAreaVBox);

	// 3. Bottom Detail Panel
	BuildBottomDetailPanel(RightAreaVBox);
}

void UBuilderCatalogMenu::BuildTopCategoryCapsule(UVerticalBox* InRightAreaVBox)
{
	UWidgetTree* Tree = WidgetTree;

	// Outer Capsule Container
	UBorder* SegmentedCapsuleBorder = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SegmentedCapsuleBorder"));
	FSlateBrush CapsuleBrush;
	CapsuleBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	CapsuleBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	CapsuleBrush.OutlineSettings.CornerRadii = FVector4(6.0f, 6.0f, 6.0f, 6.0f);
	CapsuleBrush.OutlineSettings.Width = 1.0f;
	CapsuleBrush.OutlineSettings.Color = FSlateColor(FLinearColor(0.20f, 0.32f, 0.45f, 0.5f));
	CapsuleBrush.TintColor = FSlateColor(FLinearColor(0.02f, 0.04f, 0.06f, 0.85f));
	SegmentedCapsuleBorder->SetBrush(CapsuleBrush);
	SegmentedCapsuleBorder->SetPadding(FMargin(2.0f, 2.0f));

	// Overlay inside Capsule (Layer 0 = Sliding Indicator, Layer 1 = Buttons)
	UOverlay* CapsuleOverlay = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("CapsuleOverlay"));
	SegmentedCapsuleBorder->AddChild(CapsuleOverlay);

	// Layer 0: Sliding Indicator Pill (Takes entire button segment)
	UCanvasPanel* IndicatorCanvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("IndicatorCanvas"));
	CapsuleOverlay->AddChildToOverlay(IndicatorCanvas);

	SlidingIndicatorBorder = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SlidingIndicatorBorder"));
	FSlateBrush IndBrush;
	IndBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	IndBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	IndBrush.OutlineSettings.CornerRadii = FVector4(4.0f, 4.0f, 4.0f, 4.0f);
	IndBrush.OutlineSettings.Width = 1.0f;
	IndBrush.OutlineSettings.Color = FSlateColor(CatalogUI::Cyan);
	IndBrush.TintColor = FSlateColor(FLinearColor(0.12f, 0.42f, 0.65f, 0.65f));
	SlidingIndicatorBorder->SetBrush(IndBrush);

	UCanvasPanelSlot* IndSlot = IndicatorCanvas->AddChildToCanvas(SlidingIndicatorBorder);
	IndSlot->SetPosition(FVector2D(0.0f, 0.0f));
	IndSlot->SetSize(FVector2D(TabWidth, 32.0f));

	// Layer 1: Buttons Row
	TopCategoryBar = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TopCategoryBar"));
	CapsuleOverlay->AddChildToOverlay(TopCategoryBar);

	UVerticalBoxSlot* CapsuleSlot = InRightAreaVBox->AddChildToVerticalBox(SegmentedCapsuleBorder);
	CapsuleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	CapsuleSlot->SetHorizontalAlignment(HAlign_Center);
	CapsuleSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 12.0f));
}

void UBuilderCatalogMenu::BuildPropGrid(UVerticalBox* InRightAreaVBox)
{
	UWidgetTree* Tree = WidgetTree;

	PropGrid = Tree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("PropGrid"));

#if WITH_EDITOR
	UBuilderCatalogEntry::StaticClass()->bCooked = true;
#endif

	if (FClassProperty* ClassProp = FindFProperty<FClassProperty>(UListViewBase::StaticClass(), TEXT("EntryWidgetClass")))
		ClassProp->SetObjectPropertyValue_InContainer(PropGrid, UBuilderCatalogEntry::StaticClass());

	PropGrid->SetEntryWidth(185.0f);
	PropGrid->SetEntryHeight(165.0f);
	PropGrid->SetSelectionMode(ESelectionMode::Single);
	
	UVerticalBoxSlot* GridSlot = InRightAreaVBox->AddChildToVerticalBox(PropGrid);
	GridSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	GridSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

	PropGrid->OnGetEntryClassForItem().BindUObject(this, &UBuilderCatalogMenu::GetCatalogEntryClass);
	PropGrid->OnEntryWidgetGenerated().AddUObject(this, &UBuilderCatalogMenu::HandleEntryGenerated);
	PropGrid->OnItemClicked().AddUObject(this, &UBuilderCatalogMenu::HandleEntryClicked);

	// ─── PAGINATION BAR (Centered Sleek Capsule below grid) ───────────────
	PaginationContainer = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PaginationContainer"));
	FSlateBrush PagBgBrush;
	PagBgBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	PagBgBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	PagBgBrush.OutlineSettings.CornerRadii = FVector4(5.0f, 5.0f, 5.0f, 5.0f);
	PagBgBrush.OutlineSettings.Width = 1.0f;
	PagBgBrush.OutlineSettings.Color = FSlateColor(FLinearColor(0.20f, 0.32f, 0.45f, 0.4f));
	PagBgBrush.TintColor = FSlateColor(FLinearColor(0.02f, 0.04f, 0.07f, 0.85f));
	PaginationContainer->SetBrush(PagBgBrush);
	PaginationContainer->SetPadding(FMargin(6.0f, 2.0f));

	UHorizontalBox* PaginationHBox = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PaginationHBox"));
	PaginationContainer->AddChild(PaginationHBox);

	// Prev Page Button [ < ]
	PrevPageButton = Tree->ConstructWidget<UBuilderCategoryButton>(UBuilderCategoryButton::StaticClass(), TEXT("PrevPageButton"));
	PrevPageButton->SetAsActionButton();
	PrevPageButton->SetText(FText::FromString(TEXT("<")));
	PrevPageButton->SetContentPadding(FMargin(12.0f, 3.0f));
	PrevPageButton->NormalColor = FLinearColor(0.06f, 0.09f, 0.13f, 0.6f);
	PrevPageButton->HoverColor = FLinearColor(0.12f, 0.22f, 0.35f, 0.85f);
	PrevPageButton->PressedColor = FLinearColor(0.18f, 0.32f, 0.50f, 0.95f);
	PrevPageButton->CornerRadius = 3.0f;
	PrevPageButton->StrokeColor = CatalogUI::WireframeBorder;
	PrevPageButton->OnClicked.AddDynamic(this, &UBuilderCatalogMenu::HandlePrevPageClicked);
	PrevPageButton->RefreshStyle();
	PaginationHBox->AddChildToHorizontalBox(PrevPageButton)->SetVerticalAlignment(VAlign_Center);

	// Page Text [ PAGE 1 / 3 ]
	PageText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PageText"));
	PageText->SetText(FText::FromString(TEXT("PAGE 1 / 1")));
	PageText->SetColorAndOpacity(FSlateColor(CatalogUI::Cyan));
	FSlateFontInfo PageFont = PageText->GetFont();
	PageFont.Size = 10;
	PageFont.TypefaceFontName = FName("Bold");
	PageText->SetFont(PageFont);
	UHorizontalBoxSlot* PageTxtSlot = PaginationHBox->AddChildToHorizontalBox(PageText);
	PageTxtSlot->SetVerticalAlignment(VAlign_Center);
	PageTxtSlot->SetPadding(FMargin(14.0f, 0.0f));

	// Next Page Button [ > ]
	NextPageButton = Tree->ConstructWidget<UBuilderCategoryButton>(UBuilderCategoryButton::StaticClass(), TEXT("NextPageButton"));
	NextPageButton->SetAsActionButton();
	NextPageButton->SetText(FText::FromString(TEXT(">")));
	NextPageButton->SetContentPadding(FMargin(12.0f, 3.0f));
	NextPageButton->NormalColor = FLinearColor(0.06f, 0.09f, 0.13f, 0.6f);
	NextPageButton->HoverColor = FLinearColor(0.12f, 0.22f, 0.35f, 0.85f);
	NextPageButton->PressedColor = FLinearColor(0.18f, 0.32f, 0.50f, 0.95f);
	NextPageButton->CornerRadius = 3.0f;
	NextPageButton->StrokeColor = CatalogUI::WireframeBorder;
	NextPageButton->OnClicked.AddDynamic(this, &UBuilderCatalogMenu::HandleNextPageClicked);
	NextPageButton->RefreshStyle();
	PaginationHBox->AddChildToHorizontalBox(NextPageButton)->SetVerticalAlignment(VAlign_Center);

	UVerticalBoxSlot* PagSlot = InRightAreaVBox->AddChildToVerticalBox(PaginationContainer);
	PagSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	PagSlot->SetHorizontalAlignment(HAlign_Center);
	PagSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
}

void UBuilderCatalogMenu::BuildBottomDetailPanel(UVerticalBox* InRightAreaVBox)
{
	UWidgetTree* Tree = WidgetTree;

	UBorder* DetailBoxBorder = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DetailBoxBorder"));
	FSlateBrush DetailBoxBrush;
	DetailBoxBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	DetailBoxBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	DetailBoxBrush.OutlineSettings.CornerRadii = FVector4(8.0f, 8.0f, 8.0f, 8.0f);
	DetailBoxBrush.OutlineSettings.Width = 1.0f;
	DetailBoxBrush.OutlineSettings.Color = FSlateColor(FLinearColor(0.20f, 0.32f, 0.45f, 0.45f));
	DetailBoxBrush.TintColor = FSlateColor(CatalogUI::BottomBarBg);
	DetailBoxBorder->SetBrush(DetailBoxBrush);
	DetailBoxBorder->SetPadding(FMargin(18.0f, 12.0f, 18.0f, 12.0f));

	UVerticalBoxSlot* DetailBoxSlot = InRightAreaVBox->AddChildToVerticalBox(DetailBoxBorder);
	DetailBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	UHorizontalBox* DetailColumnsHBox = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DetailColumnsHBox"));
	DetailBoxBorder->AddChild(DetailColumnsHBox);

	// ─── COLUMN 1: IDENTITY & DESCRIPTION (40%) ─────────────────────────
	UVerticalBox* Col1VBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Col1VBox"));
	UHorizontalBoxSlot* Col1Slot = DetailColumnsHBox->AddChildToHorizontalBox(Col1VBox);
	Col1Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Col1Slot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));

	UTextBlock* IntelTag = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	IntelTag->SetText(FText::FromString(TEXT("// 01 ITEM INTEL")));
	IntelTag->SetColorAndOpacity(FSlateColor(CatalogUI::Cyan.CopyWithNewOpacity(0.7f)));
	FSlateFontInfo TagFont = IntelTag->GetFont();
	TagFont.Size = 9;
	TagFont.TypefaceFontName = FName("Bold");
	IntelTag->SetFont(TagFont);
	Col1VBox->AddChildToVerticalBox(IntelTag)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 3.0f));

	UHorizontalBox* NameHBox = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Col1VBox->AddChildToVerticalBox(NameHBox)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

	DetailNameText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailNameText"));
	DetailNameText->SetText(FText::FromString(TEXT("PREFAB MODULE")));
	DetailNameText->SetColorAndOpacity(FSlateColor(CatalogUI::TextPrimary));
	FSlateFontInfo NameFont = DetailNameText->GetFont();
	NameFont.Size = 16;
	NameFont.TypefaceFontName = FName("Bold");
	DetailNameText->SetFont(NameFont);
	NameHBox->AddChildToHorizontalBox(DetailNameText)->SetVerticalAlignment(VAlign_Center);

	DetailBadgeText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailBadgeText"));
	DetailBadgeText->SetText(FText::FromString(TEXT(" [ PROP ]")));
	DetailBadgeText->SetColorAndOpacity(FSlateColor(CatalogUI::Cyan));
	FSlateFontInfo BadgeFont = DetailBadgeText->GetFont();
	BadgeFont.Size = 11;
	BadgeFont.TypefaceFontName = FName("Bold");
	DetailBadgeText->SetFont(BadgeFont);
	UHorizontalBoxSlot* BadgeSlot = NameHBox->AddChildToHorizontalBox(DetailBadgeText);
	BadgeSlot->SetVerticalAlignment(VAlign_Center);
	BadgeSlot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));

	DetailDescriptionText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailDescriptionText"));
	DetailDescriptionText->SetText(FText::FromString(TEXT("Standard reinforced deployable module. Engineered for rapid construction and structural integrity.")));
	DetailDescriptionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.75f, 0.85f, 0.85f)));
	DetailDescriptionText->SetAutoWrapText(true);
	FSlateFontInfo DescFont = DetailDescriptionText->GetFont();
	DescFont.Size = 10;
	DetailDescriptionText->SetFont(DescFont);
	Col1VBox->AddChildToVerticalBox(DetailDescriptionText)->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));

	// Separator 1
	UBorder* Sep1 = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Sep1->SetBrushColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.08f));
	USizeBox* Sep1Size = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Sep1Size->SetWidthOverride(1.0f);
	Sep1->AddChild(Sep1Size);
	UHorizontalBoxSlot* Sep1Slot = DetailColumnsHBox->AddChildToHorizontalBox(Sep1);
	Sep1Slot->SetPadding(FMargin(0.0f, 2.0f, 16.0f, 2.0f));

	// ─── COLUMN 2: TECHNICAL SPECIFICATIONS (28%) ────────────────────────
	UVerticalBox* Col2VBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Col2VBox"));
	UHorizontalBoxSlot* Col2Slot = DetailColumnsHBox->AddChildToHorizontalBox(Col2VBox);
	Col2Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Col2Slot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));

	UTextBlock* SpecTag = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	SpecTag->SetText(FText::FromString(TEXT("// 02 SPECIFICATIONS")));
	SpecTag->SetColorAndOpacity(FSlateColor(CatalogUI::Cyan.CopyWithNewOpacity(0.7f)));
	SpecTag->SetFont(TagFont);
	Col2VBox->AddChildToVerticalBox(SpecTag)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

	FSlateFontInfo SpecLabelFont = TagFont;
	SpecLabelFont.Size = 10;
	FSlateFontInfo SpecValFont = TagFont;
	SpecValFont.Size = 10;
	SpecValFont.TypefaceFontName = FName("Bold");

	// Spec 1: Hits
	UHorizontalBox* Spec1HBox = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Col2VBox->AddChildToVerticalBox(Spec1HBox)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	UTextBlock* Spec1Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Spec1Label->SetText(FText::FromString(TEXT("BUILD EFFORT: ")));
	Spec1Label->SetColorAndOpacity(FSlateColor(CatalogUI::TextSecondary));
	Spec1Label->SetFont(SpecLabelFont);
	Spec1HBox->AddChildToHorizontalBox(Spec1Label)->SetVerticalAlignment(VAlign_Center);

	DetailCol1Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailCol1Text"));
	DetailCol1Text->SetText(FText::FromString(TEXT("5 HITS")));
	DetailCol1Text->SetColorAndOpacity(FSlateColor(CatalogUI::Cyan));
	DetailCol1Text->SetFont(SpecValFont);
	Spec1HBox->AddChildToHorizontalBox(DetailCol1Text)->SetVerticalAlignment(VAlign_Center);

	// Spec 2: Deployment Role
	UHorizontalBox* Spec2HBox = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Col2VBox->AddChildToVerticalBox(Spec2HBox)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	UTextBlock* Spec2Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Spec2Label->SetText(FText::FromString(TEXT("DEPLOY ROLE: ")));
	Spec2Label->SetColorAndOpacity(FSlateColor(CatalogUI::TextSecondary));
	Spec2Label->SetFont(SpecLabelFont);
	Spec2HBox->AddChildToHorizontalBox(Spec2Label)->SetVerticalAlignment(VAlign_Center);

	DetailCol2Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailCol2Text"));
	DetailCol2Text->SetText(FText::FromString(TEXT("STANDARD PREFAB")));
	DetailCol2Text->SetColorAndOpacity(FSlateColor(CatalogUI::TextPrimary));
	DetailCol2Text->SetFont(SpecValFont);
	Spec2HBox->AddChildToHorizontalBox(DetailCol2Text)->SetVerticalAlignment(VAlign_Center);

	// Spec 3: Grid Snapping
	UHorizontalBox* Spec3HBox = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Col2VBox->AddChildToVerticalBox(Spec3HBox)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 2.0f));
	UTextBlock* Spec3Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Spec3Label->SetText(FText::FromString(TEXT("SNAPPING: ")));
	Spec3Label->SetColorAndOpacity(FSlateColor(CatalogUI::TextSecondary));
	Spec3Label->SetFont(SpecLabelFont);
	Spec3HBox->AddChildToHorizontalBox(Spec3Label)->SetVerticalAlignment(VAlign_Center);

	DetailCol3Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailCol3Text"));
	DetailCol3Text->SetText(FText::FromString(TEXT("GRID ALIGNED")));
	DetailCol3Text->SetColorAndOpacity(FSlateColor(CatalogUI::TextPrimary));
	DetailCol3Text->SetFont(SpecValFont);
	Spec3HBox->AddChildToHorizontalBox(DetailCol3Text)->SetVerticalAlignment(VAlign_Center);

	// Separator 2
	UBorder* Sep2 = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Sep2->SetBrushColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.08f));
	USizeBox* Sep2Size = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Sep2Size->SetWidthOverride(1.0f);
	Sep2->AddChild(Sep2Size);
	UHorizontalBoxSlot* Sep2Slot = DetailColumnsHBox->AddChildToHorizontalBox(Sep2);
	Sep2Slot->SetPadding(FMargin(0.0f, 2.0f, 16.0f, 2.0f));

	// ─── COLUMN 3: COST MANIFEST & ACTIONS (32%) ─────────────────────────
	UVerticalBox* Col3VBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Col3VBox"));
	UHorizontalBoxSlot* Col3Slot = DetailColumnsHBox->AddChildToHorizontalBox(Col3VBox);
	Col3Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// Top Header
	UHorizontalBox* Col3HeaderHBox = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Col3VBox->AddChildToVerticalBox(Col3HeaderHBox)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));

	UTextBlock* CostTag = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CostTag->SetText(FText::FromString(TEXT("// 03 COST & DEPLOY")));
	CostTag->SetColorAndOpacity(FSlateColor(CatalogUI::Cyan.CopyWithNewOpacity(0.7f)));
	CostTag->SetFont(TagFont);
	Col3HeaderHBox->AddChildToHorizontalBox(CostTag)->SetVerticalAlignment(VAlign_Center);

	// Resource Cost Badges (Middle)
	CostContainer = Tree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("CostContainer"));
	CostContainer->SetInnerSlotPadding(FVector2D(10.0f, 4.0f));
	UVerticalBoxSlot* CostSlot = Col3VBox->AddChildToVerticalBox(CostContainer);
	CostSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 8.0f));

	// Action Row: [ PLACE ] + (★)
	UHorizontalBox* ActionHBox = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Col3VBox->AddChildToVerticalBox(ActionHBox);

	PlaceButton = Tree->ConstructWidget<UBuilderCategoryButton>(UBuilderCategoryButton::StaticClass(), TEXT("PlaceButton"));
	PlaceButton->SetAsActionButton();
	PlaceButton->SetText(FText::FromString(TEXT("PLACE")));
	PlaceButton->SetContentPadding(FMargin(28.0f, 7.0f));
	PlaceButton->NormalColor = CatalogUI::OrangeFill;
	PlaceButton->HoverColor = FLinearColor(1.0f, 0.5f, 0.1f, 0.4f);
	PlaceButton->PressedColor = FLinearColor(1.0f, 0.5f, 0.1f, 0.7f);
	PlaceButton->CornerRadius = 4.0f;
	PlaceButton->StrokeColor = CatalogUI::Orange;
	PlaceButton->OnClicked.AddDynamic(this, &UBuilderCatalogMenu::HandlePlaceClicked);
	PlaceButton->RefreshStyle();
	UHorizontalBoxSlot* PlaceBtnSlot = ActionHBox->AddChildToHorizontalBox(PlaceButton);
	PlaceBtnSlot->SetVerticalAlignment(VAlign_Center);
	PlaceBtnSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));

	FavoriteButton = Tree->ConstructWidget<UBuilderCategoryButton>(UBuilderCategoryButton::StaticClass(), TEXT("FavoriteButton"));
	FavoriteButton->SetAsActionButton();
	FavoriteButton->SetText(FText::GetEmpty());
	FavoriteButton->SetContentPadding(FMargin(4.0f, 4.0f));
	FavoriteButton->NormalColor = FLinearColor::Transparent;
	FavoriteButton->HoverColor = FLinearColor::Transparent;
	FavoriteButton->PressedColor = FLinearColor::Transparent;
	FavoriteButton->CornerRadius = 0.0f;
	FavoriteButton->StrokeColor = FLinearColor::Transparent;
	FavoriteButton->OnClicked.AddDynamic(this, &UBuilderCatalogMenu::HandleFavoriteClicked);
	FavoriteButton->RefreshStyle();
	UHorizontalBoxSlot* FavBtnSlot = ActionHBox->AddChildToHorizontalBox(FavoriteButton);
	FavBtnSlot->SetVerticalAlignment(VAlign_Center);
}

void UBuilderCatalogMenu::BuildWindowFooter(UVerticalBox* InRootVBox)
{
	UWidgetTree* Tree = WidgetTree;

	UHorizontalBox* BottomBar = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BottomBar"));
	UVerticalBoxSlot* BottomBarSlot = InRootVBox->AddChildToVerticalBox(BottomBar);
	BottomBarSlot->SetPadding(FMargin(4.0f, 8.0f, 4.0f, 0.0f));
	BottomBarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	BackButton = Tree->ConstructWidget<UBuilderCategoryButton>(UBuilderCategoryButton::StaticClass(), TEXT("BackButton"));
	BackButton->SetAsActionButton();
	BackButton->SetText(FText::FromString(TEXT("BACK")));
	BackButton->SetContentPadding(FMargin(24.0f, 6.0f));
	BackButton->NormalColor = FLinearColor(0.06f, 0.08f, 0.10f, 0.7f);
	BackButton->HoverColor = FLinearColor(0.12f, 0.15f, 0.18f, 0.85f);
	BackButton->PressedColor = FLinearColor(0.15f, 0.20f, 0.25f, 0.95f);
	BackButton->CornerRadius = 4.0f;
	BackButton->StrokeColor = CatalogUI::WireframeBorder;
	BackButton->OnClicked.AddDynamic(this, &UBuilderCatalogMenu::HandleBackClicked);
	BackButton->RefreshStyle();

	BottomBar->AddChildToHorizontalBox(BackButton)->SetVerticalAlignment(VAlign_Center);
}

// ─────────────────────────────────────────────────────────────────────────────
// Initialization & Top Tabs
// ─────────────────────────────────────────────────────────────────────────────
void UBuilderCatalogMenu::InitializeMenu(UPlayerConstructionObject* InBuilder, const TArray<UPlacementPropData*>& InRecipes)
{
	OwningBuilder = InBuilder;
	if (InBuilder)
	{
		if (InBuilder->MenuTheme)
		{
			MenuTheme = InBuilder->MenuTheme;
		}
		else if (InBuilder->Settings && InBuilder->Settings->MenuTheme)
		{
			MenuTheme = InBuilder->Settings->MenuTheme;
		}
	}
	AvailableRecipes = InRecipes;
	SelectedPropData = nullptr;
	CurrentPage = 0;
	SearchFilter.Empty();
	bShowingFavorites = false;

	UpdatePrimaryCategoryTabs();
	SelectPrimaryCategory(0);
}

void UBuilderCatalogMenu::UpdatePrimaryCategoryTabs()
{
	if (!TopCategoryBar)
		return;

	TopCategoryBar->ClearChildren();
	PrimaryCategoryButtons.Empty();
	PrimaryCategoryProxies.Empty();

	TArray<FBuilderPrimaryCategory> Categories;
	if (MenuTheme && MenuTheme->PrimaryCategories.Num() > 0)
	{
		Categories = MenuTheme->PrimaryCategories;
	}
	else
	{
		auto AddDefaultCat = [&](const FString& InName, const FName& InTag)
		{
			FBuilderPrimaryCategory Cat;
			Cat.DisplayName = FText::FromString(InName);
			Cat.CategoryTag = FGameplayTag::RequestGameplayTag(InTag, false);
			Categories.Add(Cat);
		};

		AddDefaultCat(TEXT("DECO"), TEXT("Category.Deco"));
		AddDefaultCat(TEXT("DEFENCE"), TEXT("Category.Defence"));
		AddDefaultCat(TEXT("INFRASTRUCTURE"), TEXT("Category.Infrastructure"));
		AddDefaultCat(TEXT("STOCKAGE"), TEXT("Category.Stockage"));
	}

	for (int32 i = 0; i < Categories.Num(); ++i)
	{
		const FBuilderPrimaryCategory& Cat = Categories[i];

		USizeBox* TabSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		TabSizeBox->SetWidthOverride(TabWidth);
		TabSizeBox->SetHeightOverride(32.0f);

		UBuilderCategoryButton* TabBtn = WidgetTree->ConstructWidget<UBuilderCategoryButton>(UBuilderCategoryButton::StaticClass());
		TabBtn->SetAsActionButton();
		TabBtn->SetFontSize(11);

		FText TabLabel = Cat.DisplayName;
		if (TabLabel.IsEmpty())
		{
			FString TagStr = Cat.CategoryTag.GetTagName().ToString();
			int32 DotIdx;
			if (TagStr.FindLastChar(TEXT('.'), DotIdx))
				TagStr = TagStr.Mid(DotIdx + 1);
			TabLabel = FText::FromString(TagStr.ToUpper());
		}
		TabBtn->SetText(TabLabel);
		TabBtn->SetContentPadding(FMargin(4.0f, 4.0f));
		TabBtn->CornerRadius = 4.0f;
		TabBtn->NormalColor = FLinearColor::Transparent;
		TabBtn->HoverColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.06f);
		TabBtn->PressedColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.12f);
		TabBtn->StrokeColor = FLinearColor::Transparent;

		UBuilderPrimaryCategoryProxy* Proxy = NewObject<UBuilderPrimaryCategoryProxy>(this);
		Proxy->PrimaryIndex = i;
		Proxy->PrimaryTag = Cat.CategoryTag;
		Proxy->Menu = this;
		PrimaryCategoryProxies.Add(Proxy);

		TabBtn->OnClicked.AddDynamic(Proxy, &UBuilderPrimaryCategoryProxy::OnClicked);
		TabBtn->RefreshStyle();

		TabSizeBox->AddChild(TabBtn);
		TopCategoryBar->AddChildToHorizontalBox(TabSizeBox);

		PrimaryCategoryButtons.Add(TabBtn);
	}

	AnimateTabTo(SelectedPrimaryCategoryIndex);
}

void UBuilderCatalogMenu::SelectPrimaryCategory(int32 InIndex)
{
	SelectedPrimaryCategoryIndex = InIndex;
	bShowingFavorites = false;
	SelectedPropData = nullptr;

	AnimateTabTo(InIndex);
	UpdatePrimaryTabHighlight(InIndex);
	UpdateSubcategoriesForPrimary(InIndex);
}

void UBuilderCatalogMenu::AnimateTabTo(int32 InIndex)
{
	TargetIndicatorX = InIndex * TabWidth;

	if (SlidingIndicatorBorder)
	{
		SlidingIndicatorBorder->SetRenderOpacity(1.0f);
	}

	if (UWorld* World = GetWorld())
	{
		if (!World->GetTimerManager().IsTimerActive(TabSlideTimerHandle))
		{
			World->GetTimerManager().SetTimer(TabSlideTimerHandle, this, &UBuilderCatalogMenu::UpdateTabSlideAnimation, 0.016f, true);
		}
	}
}

void UBuilderCatalogMenu::UpdateTabSlideAnimation()
{
	if (!SlidingIndicatorBorder)
		return;

	const float InterpSpeed = 16.0f;
	CurrentIndicatorX = FMath::FInterpTo(CurrentIndicatorX, TargetIndicatorX, 0.016f, InterpSpeed);

	SlidingIndicatorBorder->SetRenderTranslation(FVector2D(CurrentIndicatorX, 0.0f));

	if (FMath::IsNearlyEqual(CurrentIndicatorX, TargetIndicatorX, 0.5f))
	{
		CurrentIndicatorX = TargetIndicatorX;
		SlidingIndicatorBorder->SetRenderTranslation(FVector2D(CurrentIndicatorX, 0.0f));

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(TabSlideTimerHandle);
		}
	}
}

void UBuilderCatalogMenu::UpdatePrimaryTabHighlight(int32 InActiveIndex)
{
	if (bShowingFavorites || InActiveIndex < 0)
	{
		if (SlidingIndicatorBorder)
			SlidingIndicatorBorder->SetRenderOpacity(0.0f);
	}
	else
	{
		if (SlidingIndicatorBorder)
			SlidingIndicatorBorder->SetRenderOpacity(1.0f);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Sidebar & Subcategories
// ─────────────────────────────────────────────────────────────────────────────
void UBuilderCatalogMenu::UpdateCategoryList()
{
	UpdatePrimaryCategoryTabs();
	SelectPrimaryCategory(SelectedPrimaryCategoryIndex);
}

void UBuilderCatalogMenu::UpdateSubcategoriesForPrimary(int32 InPrimaryIndex)
{
	if (!CategorySidebar)
		return;

	CategorySidebar->ClearChildren();
	CategoryProxies.Empty();
	ActiveCategoryButton = nullptr;
	PropsByCategory.Empty();

	FString PrimaryName;
	FGameplayTag PrimaryTag;

	if (MenuTheme && MenuTheme->PrimaryCategories.IsValidIndex(InPrimaryIndex))
	{
		PrimaryTag = MenuTheme->PrimaryCategories[InPrimaryIndex].CategoryTag;
		PrimaryName = MenuTheme->PrimaryCategories[InPrimaryIndex].DisplayName.ToString();
		if (PrimaryName.IsEmpty())
		{
			FString TagStr = PrimaryTag.GetTagName().ToString();
			int32 DotIdx;
			if (TagStr.FindLastChar(TEXT('.'), DotIdx))
				PrimaryName = TagStr.Mid(DotIdx + 1);
			else
				PrimaryName = TagStr;
		}
	}
	else
	{
		TArray<FString> Defaults = { TEXT("DECO"), TEXT("DEFENCE"), TEXT("INFRASTRUCTURE"), TEXT("STOCKAGE") };
		if (Defaults.IsValidIndex(InPrimaryIndex))
			PrimaryName = Defaults[InPrimaryIndex];
	}

	SelectedPrimaryCategoryTag = PrimaryTag;

	// Filter recipes
	TArray<UPlacementPropData*> MatchingProps;
	for (UPlacementPropData* Prop : AvailableRecipes)
	{
		if (!Prop)
			continue;

		if (Prop->MatchesPrimaryCategory(PrimaryTag))
		{
			MatchingProps.Add(Prop);
		}
		else if (!PrimaryTag.IsValid())
		{
			for (const FGameplayTag& Tag : Prop->EntityTags)
			{
				if (Tag.GetTagName().ToString().Contains(PrimaryName, ESearchCase::IgnoreCase))
				{
					MatchingProps.Add(Prop);
					break;
				}
			}
		}
	}

	if (MatchingProps.Num() == 0 && InPrimaryIndex == 0)
		MatchingProps = AvailableRecipes;

	// ALL subcategory
	const FGameplayTag AllTag = FGameplayTag::RequestGameplayTag(FName(*(TEXT("Category.") + PrimaryName + TEXT(".All"))), false);
	PropsByCategory.Add(AllTag, MatchingProps);

	for (UPlacementPropData* Prop : MatchingProps)
	{
		if (!Prop)
			continue;

		const FGameplayTag SubTag = Prop->GetEffectiveSubCategoryTag();
		if (SubTag.IsValid())
		{
			PropsByCategory.FindOrAdd(SubTag).Add(Prop);
		}
		else
		{
			for (const FGameplayTag& Tag : Prop->EntityTags)
			{
				FString TagStr = Tag.GetTagName().ToString();
				int32 LastDot;
				if (TagStr.FindLastChar(TEXT('.'), LastDot))
				{
					FString SubName = TagStr.Mid(LastDot + 1);
					if (!SubName.Equals(PrimaryName, ESearchCase::IgnoreCase) && !SubName.Equals(TEXT("Category"), ESearchCase::IgnoreCase))
					{
						PropsByCategory.FindOrAdd(Tag).Add(Prop);
					}
				}
			}
		}
	}

	auto AddSubBtn = [&](const FString& InDisplayName, const FGameplayTag& InTag) -> UBuilderCategoryButton*
	{
		UBuilderCategoryButton* SubBtn = WidgetTree->ConstructWidget<UBuilderCategoryButton>(UBuilderCategoryButton::StaticClass());
		SubBtn->SetText(FText::FromString(InDisplayName));
		SubBtn->SetContentPadding(FMargin(18.0f, 0.0f, 12.0f, 0.0f));

		UBuilderCategoryProxy* Proxy = NewObject<UBuilderCategoryProxy>(this);
		Proxy->Menu = this;
		Proxy->CategoryTag = InTag;
		CategoryProxies.Add(Proxy);

		SubBtn->OnClicked.AddDynamic(Proxy, &UBuilderCategoryProxy::OnClicked);

		UVerticalBoxSlot* SubSlot = CategorySidebar->AddChildToVerticalBox(SubBtn);
		SubSlot->SetPadding(FMargin(0.0f));

		return SubBtn;
	};

	UBuilderCategoryButton* FirstBtn = AddSubBtn(TEXT("ALL"), AllTag);

	for (const auto& Pair : PropsByCategory)
	{
		if (Pair.Key == AllTag)
			continue;

		FString TagName = Pair.Key.GetTagName().ToString();
		int32 DotIdx;
		if (TagName.FindLastChar(TEXT('.'), DotIdx))
			TagName = TagName.Mid(DotIdx + 1);
		TagName = TagName.ToUpper();

		AddSubBtn(TagName, Pair.Key);
	}

	// Spacer + MY SAVED button
	USpacer* FavSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("FavSpacer"));
	UVerticalBoxSlot* FavSpacerSlot = CategorySidebar->AddChildToVerticalBox(FavSpacer);
	FavSpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	FavoritesCategoryButton = WidgetTree->ConstructWidget<UBuilderCategoryButton>(UBuilderCategoryButton::StaticClass());
	FavoritesCategoryButton->SetText(FText::FromString(TEXT("MY SAVED")));
	FavoritesCategoryButton->SetContentPadding(FMargin(18.0f, 0.0f, 12.0f, 0.0f));

	if (MenuTheme)
	{
		const FVector2D DesiredSize = (MenuTheme->FavoriteIconSize.X > 0 && MenuTheme->FavoriteIconSize.Y > 0)
			? MenuTheme->FavoriteIconSize
			: FVector2D(18.0f, 18.0f);
		FavoritesCategoryButton->SetIconSize(DesiredSize);

		UTexture2D* StarTex = MenuTheme->StarFilledIcon ? MenuTheme->StarFilledIcon.Get() : MenuTheme->StarOutlineIcon.Get();
		if (StarTex)
			FavoritesCategoryButton->SetIcon(StarTex);
	}

	FavoritesCategoryButton->OnClicked.AddDynamic(this, &UBuilderCatalogMenu::HandleFavoritesCategoryClicked);

	CategorySidebar->AddChildToVerticalBox(FavoritesCategoryButton)->SetPadding(FMargin(0.0f));

	if (FirstBtn)
	{
		ActiveCategoryTag = AllTag;
		UpdateCategoryHighlight(FirstBtn);
		PopulateGridForCategory(AllTag);
	}
}

void UBuilderCatalogMenu::UpdateCategoryHighlight(UBuilderCategoryButton* InNewActive)
{
	if (ActiveCategoryButton.IsValid())
	{
		ActiveCategoryButton->SetIsSelected(false);
	}

	ActiveCategoryButton = InNewActive;

	if (InNewActive)
	{
		InNewActive->SetIsSelected(true);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Grid Population & Filtering
// ─────────────────────────────────────────────────────────────────────────────
void UBuilderCatalogMenu::PopulateGridForCategory(const FGameplayTag& CategoryTag)
{
	ActiveCategoryTag = CategoryTag;
	bShowingFavorites = false;
	CurrentPage = 0;
	SelectedPropData = nullptr;

	for (int32 i = 0; i < CategoryProxies.Num(); ++i)
	{
		if (CategoryProxies[i] && CategoryProxies[i]->CategoryTag == CategoryTag)
		{
			if (UBuilderCategoryButton* Btn = Cast<UBuilderCategoryButton>(CategorySidebar->GetChildAt(i)))
				UpdateCategoryHighlight(Btn);
			break;
		}
	}

	FilterAndPopulate();
}

void UBuilderCatalogMenu::ShowFavorites()
{
	bShowingFavorites = true;
	CurrentPage = 0;
	SelectedPropData = nullptr;

	UpdatePrimaryTabHighlight(-1);
	UpdateCategoryHighlight(FavoritesCategoryButton);
	FilterAndPopulate();
}

void UBuilderCatalogMenu::FilterAndPopulate()
{
	if (!PropGrid)
		return;

	FilteredItems.Empty();

	if (bShowingFavorites)
	{
		for (UPlacementPropData* Prop : AvailableRecipes)
		{
			if (Prop && FavoriteIDs.Contains(Prop->EntityID))
				FilteredItems.Add(Prop);
		}
	}
	else if (const TArray<UPlacementPropData*>* CategoryItems = PropsByCategory.Find(ActiveCategoryTag))
	{
		FilteredItems = *CategoryItems;
	}

	if (!SearchFilter.IsEmpty())
	{
		FilteredItems = FilteredItems.FilterByPredicate([this](const UPlacementPropData* Prop)
		{
			return Prop && Prop->EntityID.ToString().Contains(SearchFilter);
		});
	}

	const int32 TotalItems = FilteredItems.Num();
	const int32 StartIdx = CurrentPage * ItemsPerPage;
	const int32 EndIdx = FMath::Min(StartIdx + ItemsPerPage, TotalItems);

	PropGrid->ClearListItems();

	for (int32 i = StartIdx; i < EndIdx; ++i)
		PropGrid->AddItem(FilteredItems[i]);

	UpdatePagination();

	if (FilteredItems.Num() > 0 && !SelectedPropData)
		SelectItem(FilteredItems[0]);
}

void UBuilderCatalogMenu::UpdatePagination()
{
	const int32 TotalPages = FMath::Max(1, FMath::DivideAndRoundUp(FilteredItems.Num(), FMath::Max(1, ItemsPerPage)));
	const int32 DisplayPage = CurrentPage + 1;

	// Hide pagination capsule if not useful (only 1 page)
	if (PaginationContainer)
	{
		if (TotalPages <= 1)
		{
			PaginationContainer->SetVisibility(ESlateVisibility::Collapsed);
			return;
		}
		else
		{
			PaginationContainer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}

	if (PageText)
	{
		PageText->SetText(FText::FromString(FString::Printf(TEXT("PAGE %d / %d"), DisplayPage, TotalPages)));
	}

	// Update button enabled states and opacities
	if (PrevPageButton)
	{
		const bool bCanPrev = (CurrentPage > 0);
		PrevPageButton->SetIsEnabled(bCanPrev);
		PrevPageButton->SetRenderOpacity(bCanPrev ? 1.0f : 0.3f);
	}

	if (NextPageButton)
	{
		const bool bCanNext = (CurrentPage < TotalPages - 1);
		NextPageButton->SetIsEnabled(bCanNext);
		NextPageButton->SetRenderOpacity(bCanNext ? 1.0f : 0.3f);
	}
}

void UBuilderCatalogMenu::NextPage()
{
	const int32 TotalPages = FMath::Max(1, FMath::DivideAndRoundUp(FilteredItems.Num(), FMath::Max(1, ItemsPerPage)));
	if (CurrentPage < TotalPages - 1)
	{
		++CurrentPage;
		FilterAndPopulate();
	}
}

void UBuilderCatalogMenu::PrevPage()
{
	if (CurrentPage > 0)
	{
		--CurrentPage;
		FilterAndPopulate();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Selection & Detail Bar
// ─────────────────────────────────────────────────────────────────────────────
void UBuilderCatalogMenu::SelectItem(UPlacementPropData* InProp)
{
	if (!InProp)
		return;

	SelectedPropData = InProp;
	UpdateDetailBar(InProp);

	if (PropGrid)
		PropGrid->SetSelectedItem(InProp);

	UE_LOG(LogTemp, Display, TEXT("[BuilderCatalog] Selected: %s"), *InProp->EntityID.ToString());
}

// ─────────────────────────────────────────────────────────────────────────────
// Sci-Fi Text Decryption Animation (Scramble Effect)
// ─────────────────────────────────────────────────────────────────────────────
void UBuilderCatalogMenu::StartScrambleText(UTextBlock* InTextBlock, const FString& InFinalText, float InDuration)
{
	if (!InTextBlock)
		return;

	if (InFinalText.IsEmpty())
	{
		InTextBlock->SetText(FText::GetEmpty());
		return;
	}

	ActiveScrambleTargets.RemoveAll([InTextBlock](const FBuilderTextScrambleTarget& Target)
	{
		return Target.TargetTextBlock.Get() == InTextBlock;
	});

	FBuilderTextScrambleTarget NewTarget;
	NewTarget.TargetTextBlock = InTextBlock;
	NewTarget.FinalText = InFinalText;
	NewTarget.Duration = FMath::Max(0.05f, InDuration);
	NewTarget.ElapsedTime = 0.0f;
	NewTarget.TotalCharacters = InFinalText.Len();
	NewTarget.bIsComplete = false;

	ActiveScrambleTargets.Add(NewTarget);

	if (UWorld* World = GetWorld())
	{
		if (!World->GetTimerManager().IsTimerActive(ScrambleTimerHandle))
		{
			World->GetTimerManager().SetTimer(ScrambleTimerHandle, this, &UBuilderCatalogMenu::UpdateScrambleAnimation, 0.025f, true);
		}
	}
}

void UBuilderCatalogMenu::UpdateScrambleAnimation()
{
	static const TCHAR ScrambleGlyphs[] = TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!#%&*+-_/<>[]");
	const int32 GlyphCount = UE_ARRAY_COUNT(ScrambleGlyphs) - 1;

	const float StepDelta = 0.025f;
	bool bAllComplete = true;

	for (FBuilderTextScrambleTarget& Target : ActiveScrambleTargets)
	{
		if (Target.bIsComplete || !Target.TargetTextBlock.IsValid())
			continue;

		Target.ElapsedTime += StepDelta;
		const float Progress = FMath::Clamp(Target.ElapsedTime / Target.Duration, 0.0f, 1.0f);
		const int32 ResolvedCount = FMath::FloorToInt(Progress * Target.TotalCharacters);

		if (Progress >= 1.0f)
		{
			Target.TargetTextBlock->SetText(FText::FromString(Target.FinalText));
			Target.bIsComplete = true;
		}
		else
		{
			bAllComplete = false;

			FString DisplayStr;
			DisplayStr.Reserve(Target.TotalCharacters);

			for (int32 i = 0; i < Target.TotalCharacters; ++i)
			{
				const TCHAR FinalChar = Target.FinalText[i];
				if (FChar::IsWhitespace(FinalChar))
				{
					DisplayStr.AppendChar(FinalChar);
				}
				else if (i < ResolvedCount)
				{
					DisplayStr.AppendChar(FinalChar);
				}
				else if (i < ResolvedCount + 3)
				{
					const int32 RandIdx = FMath::RandRange(0, GlyphCount - 1);
					DisplayStr.AppendChar(ScrambleGlyphs[RandIdx]);
				}
			}

			Target.TargetTextBlock->SetText(FText::FromString(DisplayStr));
		}
	}

	if (bAllComplete)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ScrambleTimerHandle);
		}
		ActiveScrambleTargets.Empty();
	}
}

void UBuilderCatalogMenu::UpdateDetailBar(UPlacementPropData* InProp)
{
	if (!InProp)
		return;

	// 1. Name with Decryption Animation
	if (DetailNameText)
	{
		StartScrambleText(DetailNameText, InProp->EntityID.ToString().ToUpper(), 0.20f);
	}

	// 2. Badge
	if (DetailBadgeText)
	{
		FString BadgeStr = TEXT(" [ PROP ]");
		if (InProp->EntityTags.IsValid() && InProp->EntityTags.Num() > 0)
		{
			FString TagName = InProp->EntityTags.First().GetTagName().ToString();
			int32 DotIdx;
			if (TagName.FindLastChar(TEXT('.'), DotIdx))
				TagName = TagName.Mid(DotIdx + 1);
			BadgeStr = FString::Printf(TEXT(" [ %s ]"), *TagName.ToUpper());
		}
		DetailBadgeText->SetText(FText::FromString(BadgeStr));
	}

	// 3. Short Description with Decryption Animation
	if (DetailDescriptionText)
	{
		const FString DescStr = !InProp->EntityDescription.IsEmpty()
			? InProp->EntityDescription.ToString()
			: TEXT("Standard reinforced deployable module. Engineered for rapid construction and structural integrity.");
		StartScrambleText(DetailDescriptionText, DescStr, 0.32f);
	}

	// 4. Thumbnail
	if (DetailThumbnail)
	{
		if (UTexture2D* Tex = InProp->EntityThumbnail.LoadSynchronous())
		{
			DetailThumbnail->SetBrushFromTexture(Tex);
			DetailThumbnail->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			DetailThumbnail->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 5. Tech specs: Hits with Decryption Animation
	if (DetailCol1Text)
	{
		StartScrambleText(DetailCol1Text, FString::Printf(TEXT("%d HITS"), InProp->HitsRequired), 0.18f);
	}

	// 6. Col 2: Deployment Role
	if (DetailCol2Text)
	{
		const FString Col2Str = InProp->bIsFOB ? TEXT("OUTPOST CORE (FOB)") : TEXT("STANDARD PREFAB");
		StartScrambleText(DetailCol2Text, Col2Str, 0.22f);
	}

	// 7. Col 3: Grid Snapping
	if (DetailCol3Text)
	{
		const FString Col3Str = InProp->bCanBeSnappedTo ? TEXT("GRID ALIGNED") : TEXT("FREE PLACEMENT");
		StartScrambleText(DetailCol3Text, Col3Str, 0.24f);
	}

	// 8. Resource Cost Badges (Zero-GC Pool)
	UpdateResourceCosts(InProp);

	// 9. Favorite Button
	if (FavoriteButton)
	{
		const bool bFav = IsFavorite(InProp);

		FavoriteButton->SetText(FText::GetEmpty());

		if (MenuTheme)
		{
			const FVector2D DesiredSize = (MenuTheme->FavoriteIconSize.X > 0 && MenuTheme->FavoriteIconSize.Y > 0)
				? MenuTheme->FavoriteIconSize
				: FVector2D(18.0f, 18.0f);
			FavoriteButton->SetIconSize(DesiredSize);

			UTexture2D* StarTex = bFav ? MenuTheme->StarFilledIcon.Get() : MenuTheme->StarOutlineIcon.Get();
			if (StarTex)
			{
				FavoriteButton->SetIcon(StarTex);
			}
			else
			{
				FavoriteButton->SetIcon(nullptr);
				FavoriteButton->SetText(bFav ? FText::FromString(TEXT("\x2605")) : FText::FromString(TEXT("\x2606")));
			}
		}
		else
		{
			FavoriteButton->SetIcon(nullptr);
			FavoriteButton->SetText(bFav ? FText::FromString(TEXT("\x2605")) : FText::FromString(TEXT("\x2606")));
		}

		FavoriteButton->StrokeColor = FLinearColor::Transparent;
		FavoriteButton->NormalColor = FLinearColor::Transparent;
		FavoriteButton->HoverColor = FLinearColor::Transparent;
		FavoriteButton->PressedColor = FLinearColor::Transparent;
		FavoriteButton->SetVisibility(ESlateVisibility::Visible);
		FavoriteButton->RefreshStyle();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Resource Costs (Zero-GC Object Pooling)
// ─────────────────────────────────────────────────────────────────────────────
void UBuilderCatalogMenu::UpdateResourceCosts(UPlacementPropData* InProp)
{
	if (!CostContainer || !WidgetTree)
		return;

	const int32 NumCosts = (InProp) ? InProp->ConstructionCosts.Num() : 0;

	while (CostBadgePool.Num() < NumCosts)
	{
		FBuilderResourceCostBadge NewBadge;

		NewBadge.BadgeBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

		USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		IconBox->SetWidthOverride(26.0f);
		IconBox->SetHeightOverride(26.0f);

		UBorder* IconBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		FSlateBrush IconBgBrush;
		IconBgBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
		IconBgBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		IconBgBrush.OutlineSettings.CornerRadii = FVector4(4.0f, 4.0f, 4.0f, 4.0f);
		IconBgBrush.OutlineSettings.Width = 1.0f;
		IconBgBrush.OutlineSettings.Color = FSlateColor(FLinearColor(0.25f, 0.40f, 0.55f, 0.6f));
		IconBgBrush.TintColor = FSlateColor(FLinearColor(0.02f, 0.04f, 0.07f, 0.95f));
		IconBorder->SetBrush(IconBgBrush);
		IconBorder->SetPadding(FMargin(2.0f));
		IconBox->AddChild(IconBorder);

		NewBadge.IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		IconBorder->AddChild(NewBadge.IconImage);

		UHorizontalBoxSlot* IconSlot = NewBadge.BadgeBox->AddChildToHorizontalBox(IconBox);
		IconSlot->SetVerticalAlignment(VAlign_Center);

		NewBadge.LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		NewBadge.LabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.95f, 0.98f, 1.0f)));
		FSlateFontInfo Font = NewBadge.LabelText->GetFont();
		Font.Size = 11;
		Font.TypefaceFontName = FName("Bold");
		NewBadge.LabelText->SetFont(Font);

		UHorizontalBoxSlot* TextSlot = NewBadge.BadgeBox->AddChildToHorizontalBox(NewBadge.LabelText);
		TextSlot->SetVerticalAlignment(VAlign_Center);
		TextSlot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));

		CostContainer->AddChild(NewBadge.BadgeBox);
		CostBadgePool.Add(NewBadge);
	}

	for (int32 i = 0; i < CostBadgePool.Num(); ++i)
	{
		FBuilderResourceCostBadge& Badge = CostBadgePool[i];
		if (!Badge.BadgeBox)
			continue;

		if (i < NumCosts)
		{
			const FJupiterResourceCost& Cost = InProp->ConstructionCosts[i];

			UPDA_ItemClass* ItemAsset = Cost.ResourceItem.LoadSynchronous();
			UTexture2D* IconTex = ItemAsset ? ItemAsset->EntityThumbnail.LoadSynchronous() : nullptr;

			FString ItemName = ItemAsset ? ItemAsset->EntityID.ToString() : Cost.ResourceItem.GetAssetName();
			if (ItemName.StartsWith(TEXT("DA_Item_")))
				ItemName = ItemName.Mid(8);
			else if (ItemName.StartsWith(TEXT("DA_")))
				ItemName = ItemName.Mid(3);

			if (Badge.IconImage)
			{
				if (IconTex)
				{
					Badge.IconImage->SetBrushFromTexture(IconTex, true);
					Badge.IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
				}
				else
				{
					Badge.IconImage->SetVisibility(ESlateVisibility::Collapsed);
				}
			}

			if (Badge.LabelText)
			{
				Badge.LabelText->SetText(FText::FromString(FString::Printf(TEXT("%d %s"), Cost.Amount, *ItemName.ToUpper())));
			}

			Badge.BadgeBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			Badge.BadgeBox->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Favorites Management
// ─────────────────────────────────────────────────────────────────────────────
void UBuilderCatalogMenu::ToggleFavorite()
{
	if (!SelectedPropData)
		return;

	const FName ID = SelectedPropData->EntityID;
	if (FavoriteIDs.Contains(ID))
		FavoriteIDs.Remove(ID);
	else
		FavoriteIDs.Add(ID);

	UpdateDetailBar(SelectedPropData);

	if (bShowingFavorites)
		FilterAndPopulate();
}

bool UBuilderCatalogMenu::IsFavorite(UPlacementPropData* InProp) const
{
	return InProp && FavoriteIDs.Contains(InProp->EntityID);
}

// ─────────────────────────────────────────────────────────────────────────────
// Event Handlers
// ─────────────────────────────────────────────────────────────────────────────
void UBuilderCatalogMenu::HandleEntryClicked(UObject* Item)
{
	if (UPlacementPropData* Prop = Cast<UPlacementPropData>(Item))
		SelectItem(Prop);
}

void UBuilderCatalogMenu::HandleEntryGenerated(UUserWidget& Widget)
{
	if (UBuilderCatalogEntry* Entry = Cast<UBuilderCatalogEntry>(&Widget))
	{
		if (MenuTheme)
			Entry->ApplyTheme(MenuTheme);

		Entry->OnClicked.RemoveAll(this);
		Entry->OnClicked.AddDynamic(this, &UBuilderCatalogMenu::HandleEntryButtonClicked);
	}
}

void UBuilderCatalogMenu::HandleEntryButtonClicked(UPrismButtonBase* Button)
{
	if (UBuilderCatalogEntry* Entry = Cast<UBuilderCatalogEntry>(Button))
	{
		if (UPlacementPropData* Prop = Entry->GetPropData())
		{
			SelectItem(Prop);
		}
	}
}

void UBuilderCatalogMenu::HandlePlaceClicked(UPrismButtonBase* Button)
{
	if (!SelectedPropData || !OwningBuilder.IsValid())
		return;

	OwningBuilder->EnterBuildMode(SelectedPropData);
	CloseBuilderMenu();
}

void UBuilderCatalogMenu::HandleFavoriteClicked(UPrismButtonBase* Button)
{
	ToggleFavorite();
}

void UBuilderCatalogMenu::HandleFavoritesCategoryClicked(UPrismButtonBase* Button)
{
	ShowFavorites();
}

void UBuilderCatalogMenu::HandlePrevPageClicked(UPrismButtonBase* Button)
{
	PrevPage();
}

void UBuilderCatalogMenu::HandleNextPageClicked(UPrismButtonBase* Button)
{
	NextPage();
}

void UBuilderCatalogMenu::HandleBackClicked(UPrismButtonBase* Button)
{
	CloseBuilderMenu();
}

void UBuilderCatalogMenu::HandleSearchTextChanged(const FText& InText)
{
	SearchFilter = InText.ToString();
	CurrentPage = 0;
	FilterAndPopulate();
}

void UBuilderCatalogMenu::OpenBuilderMenu()
{
	AddToViewport();
	SetWidgetState(EPrismWidgetState::Normal);
}

void UBuilderCatalogMenu::CloseBuilderMenu()
{
	RemoveFromParent();
}

TSubclassOf<UUserWidget> UBuilderCatalogMenu::GetCatalogEntryClass(UObject* Item)
{
	return EntryWidgetClass ? *EntryWidgetClass : UBuilderCatalogEntry::StaticClass();
}
