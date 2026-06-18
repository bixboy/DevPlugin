#include "Subsystems/StellarThumbnailSubsystem.h"
#include "ToolMenus.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetData.h"
#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Misc/FileHelper.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/ObjectThumbnail.h"
#include "Framework/Application/SlateApplication.h"
#include "Editor.h"
#include "ObjectTools.h"

#define LOCTEXT_NAMESPACE "StellarThumbnailSubsystem"


void UStellarThumbnailSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &UStellarThumbnailSubsystem::RegisterMenu));
}

void UStellarThumbnailSubsystem::Deinitialize()
{
	UToolMenus::UnRegisterStartupCallback(this);
	Super::Deinitialize();
}

void UStellarThumbnailSubsystem::RegisterMenu()
{
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu");
	if (!Menu)
		return;

	FToolMenuSection& Section = Menu->FindOrAddSection("AssetContextAdvancedActions");
	Section.AddMenuEntry(
		"SetCustomThumbnail",
		LOCTEXT("SetCustomThumbnail", "Set Custom Thumbnail"),
		LOCTEXT("SetCustomThumbnailTooltip", "Set a custom thumbnail for the selected assets from an image file."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.AssetActions.Edit"),
		FUIAction(FExecuteAction::CreateUObject(this, &UStellarThumbnailSubsystem::OnSetCustomThumbnailClicked))
	);
}

void UStellarThumbnailSubsystem::OnSetCustomThumbnailClicked()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	if (SelectedAssets.IsEmpty())
		return;

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
		return;

	const void* ParentWindowWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

	TArray<FString> OutFilenames;
	bool bOpened = DesktopPlatform->OpenFileDialog(
		ParentWindowWindowHandle,
		TEXT("Select Custom Thumbnail Image"),
		TEXT(""),
		TEXT(""),
		TEXT("Image Files (*.png, *.jpg, *.jpeg)|*.png;*.jpg;*.jpeg"),
		EFileDialogFlags::None,
		OutFilenames
	);

	if (!bOpened || OutFilenames.IsEmpty())
		return;

	FString ImagePath = OutFilenames[0];
	TArray<uint8> RawFileData;
	if (!FFileHelper::LoadFileToArray(RawFileData, *ImagePath))
		return;

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	EImageFormat ImageFormat = ImageWrapperModule.DetectImageFormat(RawFileData.GetData(), RawFileData.Num());
	
	if (ImageFormat == EImageFormat::Invalid)
		return;

	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);
	if (!ImageWrapper.IsValid() || !ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
		return;

	TArray<uint8> UncompressedBGRA;
	if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
		return;

	const int32 ImageWidth = ImageWrapper->GetWidth();
	const int32 ImageHeight = ImageWrapper->GetHeight();

	FObjectThumbnail NewThumbnail;
	NewThumbnail.SetImageSize(ImageWidth, ImageHeight);
	NewThumbnail.AccessImageData() = UncompressedBGRA;

	for (const FAssetData& AssetData : SelectedAssets)
	{
		UObject* Asset = AssetData.GetAsset();
		if (Asset)
		{
			ThumbnailTools::CacheThumbnail(Asset->GetFullName(), &NewThumbnail, Asset->GetOutermost());
			Asset->MarkPackageDirty();
		}
	}
}

#undef LOCTEXT_NAMESPACE
