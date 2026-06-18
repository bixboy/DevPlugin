// Copyright (c) Bixboy, 2025. All Rights Reserved.

#if WITH_EDITOR
#include "LevelSelection.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "FileHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SWindow.h"
#include "Widgets/SToolTip.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Interfaces/IPluginManager.h"
#include "GameMapsSettings.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetThumbnail.h"

#define LOCTEXT_NAMESPACE "FLevelSelectionModule"

static const FName LevelSelectionMenuName("LevelSelection");

// ===================================================================================
// ==========           MODULE INITIALIZATION / SHUTDOWN FUNCTIONS           =========
// ===================================================================================

void FLevelSelectionModule::StartupModule()
{
    LoadCategoriesFromConfig();

    if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
    {
        FLevelEditorModule& EditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
        auto Extender = MakeShared<FExtender>();

        Extender->AddMenuBarExtension(
            "Help",
            EExtensionHook::After,
            nullptr,
            FMenuBarExtensionDelegate::CreateRaw(this, &FLevelSelectionModule::AddMenuEntry)
        );

        EditorModule.GetMenuExtensibilityManager()->AddExtender(Extender);
    }

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    
    AssetRegistry.OnFilesLoaded().AddRaw(this, &FLevelSelectionModule::OnFilesLoaded);
    AssetRegistry.OnAssetAdded().AddRaw(this, &FLevelSelectionModule::OnAssetAdded);
    AssetRegistry.OnAssetRemoved().AddRaw(this, &FLevelSelectionModule::OnAssetRemoved);
    AssetRegistry.OnAssetRenamed().AddRaw(this, &FLevelSelectionModule::OnAssetRenamed);
    
    if (!AssetRegistry.IsLoadingAssets())
    {
        OnFilesLoaded();
    }
}

void FLevelSelectionModule::ShutdownModule()
{
    if (FModuleManager::Get().IsModuleLoaded("AssetRegistry"))
    {
        FAssetRegistryModule& Arm = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        Arm.Get().OnFilesLoaded().RemoveAll(this);
        Arm.Get().OnAssetAdded().RemoveAll(this);
        Arm.Get().OnAssetRemoved().RemoveAll(this);
        Arm.Get().OnAssetRenamed().RemoveAll(this);
    }

    UToolMenus::UnregisterOwner(this);
}

// ===================================================================================
// ==========                       MENU STRUCTURE                          ==========
// ===================================================================================

void FLevelSelectionModule::AddMenuEntry(FMenuBarBuilder& MenuBuilder)
{
    MenuBuilder.AddPullDownMenu(
        LOCTEXT("MenuLabel", "Level Selection"),
        LOCTEXT("MenuTooltip", "Open the Level Selection menu"),
        FNewMenuDelegate::CreateRaw(this, &FLevelSelectionModule::FillSubmenu),
        LevelSelectionMenuName
    );
}

void FLevelSelectionModule::FillSubmenu(FMenuBuilder& MenuBuilder)
{
    EnsureMapCache();

    if (FavoriteLevels.Num() > 0)
    {
        MenuBuilder.BeginSection("Favorites", LOCTEXT("FavoritesSection", "Favorites"));
        
        TArray<FString> FavList = FavoriteLevels.Array();
        FavList.Sort();
        for (const FString& FavPath : FavList)
        {
            AddLevelEntryToMenu(MenuBuilder, FavPath);
        }
        
        MenuBuilder.EndSection();
    }

    MenuBuilder.BeginSection("Categories", LOCTEXT("CategoriesSection", "Categories"));
    
    for (const FString& Cat : CategoryOrder)
    {
        const TArray<FString>& Levels = LevelCategories.FindChecked(Cat);
        MenuBuilder.AddSubMenu(
            FText::FromString(Cat),
            LOCTEXT("CategoryTooltip", "Options for this category"),
            FNewMenuDelegate::CreateLambda([this, Cat, Levels](FMenuBuilder& SubMenuBuilder) { PopulateCategoryMenu(SubMenuBuilder, Cat, Levels); })
        );
    }

    MenuBuilder.AddSubMenu(
        LOCTEXT("UncategorizedLabel", "Uncategorized Levels"),
        LOCTEXT("UncategorizedTooltip", "Levels not in any category"),
        FNewMenuDelegate::CreateRaw(this, &FLevelSelectionModule::PopulateUncategorizedMenu)
    );
    
    MenuBuilder.EndSection();
}

void FLevelSelectionModule::PopulateCategoryMenu(FMenuBuilder& MenuBuilder, const FString& InCategoryName, const TArray<FString>& InLevels)
{
    if (InLevels.Num() == 0)
    {
        MenuBuilder.AddMenuEntry(
            LOCTEXT("EmptyCategory", "No levels in this category."),
            FText::GetEmpty(),
            FSlateIcon(),
            FUIAction()
        );
        return;
    }

    for (const FString& Path : InLevels)
    {
        AddLevelEntryToMenu(MenuBuilder, Path);
    }
}

void FLevelSelectionModule::PopulateUncategorizedMenu(FMenuBuilder& MenuBuilder)
{
    TArray<FString> Levels = GetUncategorizedMapPaths();
    PopulateCategoryMenu(MenuBuilder, FString(), Levels);
}

void FLevelSelectionModule::AddLevelEntryToMenu(FMenuBuilder& MenuBuilder, const FString& InPath)
{
    const FString Name = FPaths::GetBaseFilename(InPath);

    TSharedRef<SWidget> MenuContents = SNew(STextBlock)
        .Text(FText::FromString(Name))
        .ToolTip(CreateCustomTooltip(InPath));

    MenuBuilder.AddMenuEntry(
        FUIAction(FExecuteAction::CreateLambda([this, InPath]() { OnOpenLevel(InPath); })),
        MenuContents
    );

    MenuBuilder.AddSubMenu(
        LOCTEXT("OptionsLabel", "Options"),
        LOCTEXT("OptionsTooltip", "Hover to show options"),
        FNewMenuDelegate::CreateLambda([this, InPath](FMenuBuilder& OptionsBuilder)
        {
            const bool bIsFavorite = FavoriteLevels.Contains(InPath);
            OptionsBuilder.AddMenuEntry(
                bIsFavorite ? LOCTEXT("RemoveFavorite", "Remove from Favorites") : LOCTEXT("AddFavorite", "Add to Favorites"),
                LOCTEXT("ToggleFavoriteTooltip", "Pin or unpin this level in the Favorites section"),
                FSlateIcon(),
                FUIAction(FExecuteAction::CreateLambda([this, InPath]() { OnToggleFavorite(InPath); }))
            );

            OptionsBuilder.AddMenuEntry(
                LOCTEXT("LocateInBrowser", "Locate in Content Browser"),
                LOCTEXT("LocateInBrowserTooltip", "Find this level in the Content Browser"),
                FSlateIcon(),
                FUIAction(FExecuteAction::CreateLambda([this, InPath]() { OnLocateInContentBrowser(InPath); }))
            );

            OptionsBuilder.AddSeparator();

            OptionsBuilder.AddMenuEntry(
                LOCTEXT("CopyName", "Copy Level Name"),
                LOCTEXT("CopyNameTooltip", "Copy level name to clipboard"),
                FSlateIcon(),
                FUIAction(FExecuteAction::CreateLambda([this, InPath]() { OnCopyLevelName(InPath); }))
            );

            OptionsBuilder.AddMenuEntry(
                LOCTEXT("SetDefaultEditorLevel", "Set as Default Editor Level"),
                LOCTEXT("SetDefaultEditorLevelTooltip", "Sets this level to open by default when the editor starts"),
                FSlateIcon(),
                FUIAction(FExecuteAction::CreateLambda([this, InPath]() { OnSetAsDefaultEditorLevel(InPath); }))
            );

            OptionsBuilder.AddSeparator();

            OptionsBuilder.AddMenuEntry(
                LOCTEXT("CreateCategory", "Create New Category"),
                LOCTEXT("CreateCategoryTooltip", "Create a new category and move this level"),
                FSlateIcon(),
                FUIAction(FExecuteAction::CreateLambda([this, InPath]() { OnCreateNewCategory(InPath); }))
            );

            for (const auto& Pair : LevelCategories)
            {
                const FString& TargetCat = Pair.Key;

                if (Pair.Value.Contains(InPath))
                    continue;

                OptionsBuilder.AddMenuEntry(
                    FText::FromString(TargetCat),
                    FText::Format(LOCTEXT("MoveFmt", "Move to {0}"), FText::FromString(TargetCat)),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateLambda([this, InPath, TargetCat]() { OnMoveLevelToCategory(InPath, TargetCat); }))
                );
            }

            bool bInCategory = false;
            for (const auto& Pair : LevelCategories)
            {
                if (Pair.Value.Contains(InPath))
                {
                    bInCategory = true;
                    break;
                }
            }

            if (bInCategory)
            {
                OptionsBuilder.AddMenuEntry(
                    LOCTEXT("RemoveFromCategory", "Remove from Category"),
                    LOCTEXT("RemoveFromCategoryTooltip", "Remove from category to make it Uncategorized"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateLambda([this, InPath]() { OnMoveLevelToCategory(InPath, FString()); }))
                );
            }
        })
    );

    MenuBuilder.AddSeparator();
}

// ===================================================================================
// ==========                 LEVEL & CATEGORY MANAGEMENT                   ==========
// ===================================================================================

void FLevelSelectionModule::OnOpenLevel(const FString& InLevelPath)
{
    if (FEditorFileUtils::SaveDirtyPackages(true, true, true))
    {
        FEditorFileUtils::LoadMap(InLevelPath, false);
        UE_LOG(LogTemp, Display, TEXT("[LevelSelection] Opened level: %s"), *InLevelPath);
    }
}

void FLevelSelectionModule::OnCopyLevelName(const FString& InLevelPath)
{
    const FString Name = FPaths::GetBaseFilename(InLevelPath);
    FPlatformApplicationMisc::ClipboardCopy(*Name);
    
    ShowNotification(FText::Format(LOCTEXT("CopiedFmt", "Level '{0}' copied!"), FText::FromString(Name)));
    UE_LOG(LogTemp, Log, TEXT("[LevelSelection] Copied level name to clipboard: %s"), *Name);
}

void FLevelSelectionModule::OnSetAsDefaultEditorLevel(const FString& InLevelPath)
{
    if (UGameMapsSettings* MapsSettings = GetMutableDefault<UGameMapsSettings>())
    {
        const FString FullPath = FString::Printf(TEXT("%s.%s"), *InLevelPath, *FPaths::GetBaseFilename(InLevelPath));
        
        MapsSettings->EditorStartupMap = FSoftObjectPath(FullPath);
        MapsSettings->SaveConfig();
        MapsSettings->TryUpdateDefaultConfigFile();
        
        ShowNotification(LOCTEXT("SetDefaultEditorLevelSuccess", "Set as Default Editor Map"));
        UE_LOG(LogTemp, Log, TEXT("[LevelSelection] Set Default Editor Level to: %s"), *FullPath);
    }
}

void FLevelSelectionModule::OnCreateNewCategory(const FString& InLevelPath)
{
    TSharedPtr<SEditableTextBox> InputBox;
    NewCategoryWindow = SNew(SWindow)
        .Title(LOCTEXT("NewCategoryTitle", "New Category"))
        .ClientSize(FVector2D(300, 120))
        .SupportsMinimize(false)
        .SupportsMaximize(false)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().Padding(8)
            [
                SNew(STextBlock).Text(LOCTEXT("EnterCategory", "Enter category name:"))
            ]
            + SVerticalBox::Slot().Padding(8)
            [
                SAssignNew(InputBox, SEditableTextBox).HintText(LOCTEXT("CategoryHint", "Category Name"))
            ]
            + SVerticalBox::Slot().HAlign(HAlign_Right).Padding(8)
            [
                SNew(SButton)
                .Text(LOCTEXT("OK", "OK"))
                .OnClicked_Lambda([this, InputBox, InLevelPath]() -> FReply
                {
                    const FString NewCat = InputBox->GetText().ToString().TrimStartAndEnd();

                    if (NewCat.IsEmpty())
                    {
                        ShowNotification(LOCTEXT("EmptyError", "Category name cannot be empty."));
                        return FReply::Handled();
                    }

                    if (LevelCategories.Contains(NewCat))
                    {
                        ShowNotification(LOCTEXT("AlreadyExistsError", "This category already exists."));
                        return FReply::Handled();
                    }

                    LevelCategories.FindOrAdd(NewCat).Add(InLevelPath);
                    CategoryOrder.AddUnique(NewCat);
                    OnMoveLevelToCategory(InLevelPath, NewCat);

                    UE_LOG(LogTemp, Log, TEXT("[LevelSelection] Created new category: %s"), *NewCat);

                    if (NewCategoryWindow.IsValid())
                    {
                        NewCategoryWindow->RequestDestroyWindow();
                    }

                    return FReply::Handled();
                })
            ]
        ];

    FSlateApplication::Get().AddWindow(NewCategoryWindow.ToSharedRef());
}

void FLevelSelectionModule::OnMoveLevelToCategory(const FString& InLevelPath, const FString& InTargetCategory)
{
    for (auto It = LevelCategories.CreateIterator(); It; ++It)
    {
        TArray<FString>& Levels = It.Value();
        Levels.Remove(InLevelPath);

        if (Levels.Num() == 0)
        {
            CategoryOrder.Remove(It.Key());
            It.RemoveCurrent();
        }
    }

    if (!InTargetCategory.IsEmpty())
    {
        LevelCategories.FindOrAdd(InTargetCategory).AddUnique(InLevelPath);
        CategoryOrder.AddUnique(InTargetCategory);
        ShowNotification(FText::Format(LOCTEXT("MovedFmt", "Moved to '{0}'"), FText::FromString(InTargetCategory)));
        UE_LOG(LogTemp, Log, TEXT("[LevelSelection] Moved level '%s' to category '%s'"), *InLevelPath, *InTargetCategory);
    }
    else
    {
        ShowNotification(LOCTEXT("MovedToUncategorized", "Moved to Uncategorized"));
        UE_LOG(LogTemp, Log, TEXT("[LevelSelection] Moved level '%s' to Uncategorized"), *InLevelPath);
    }

    SaveCategoriesToConfig();
}

void FLevelSelectionModule::OnDeleteCategory(const FString& InCategoryName)
{
    if (LevelCategories.Remove(InCategoryName) > 0)
    {
        CategoryOrder.Remove(InCategoryName);
        SaveCategoriesToConfig();
        ShowNotification(FText::Format(LOCTEXT("DeletedFmt", "Deleted category '{0}'"), FText::FromString(InCategoryName)));
        UE_LOG(LogTemp, Log, TEXT("[LevelSelection] Deleted category: %s"), *InCategoryName);
    }
}

void FLevelSelectionModule::OnToggleFavorite(const FString& InLevelPath)
{
    if (FavoriteLevels.Contains(InLevelPath))
    {
        FavoriteLevels.Remove(InLevelPath);
        ShowNotification(LOCTEXT("RemovedFavorite", "Removed from Favorites"));
        UE_LOG(LogTemp, Log, TEXT("[LevelSelection] Removed level from favorites: %s"), *InLevelPath);
    }
    else
    {
        FavoriteLevels.Add(InLevelPath);
        ShowNotification(LOCTEXT("AddedFavorite", "Added to Favorites"));
        UE_LOG(LogTemp, Log, TEXT("[LevelSelection] Added level to favorites: %s"), *InLevelPath);
    }
    SaveCategoriesToConfig();
}

void FLevelSelectionModule::OnLocateInContentBrowser(const FString& InLevelPath)
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    const FString FullPath = FString::Printf(TEXT("%s.%s"), *InLevelPath, *FPaths::GetBaseFilename(InLevelPath));
    FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(FullPath));

    if (!AssetData.IsValid())
    {
        AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(InLevelPath));
    }

    if (AssetData.IsValid())
    {
        TArray<FAssetData> AssetsToSync;
        AssetsToSync.Add(AssetData);
        FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
        ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSync);
        
        UE_LOG(LogTemp, Log, TEXT("[LevelSelection] Located level in Content Browser: %s"), *InLevelPath);
    }
    else
    {
        ShowNotification(LOCTEXT("LocateFailed", "Could not find asset in Content Browser"));
    }
}

// ===================================================================================
// ==========                        Helper Functions                       ==========
// ===================================================================================

TSharedPtr<SToolTip> FLevelSelectionModule::CreateCustomTooltip(const FString& InLevelPath)
{
    if (!ThumbnailPool.IsValid())
    {
        ThumbnailPool = MakeShared<FAssetThumbnailPool>(64, false);
    }

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    const FString FullPath = FString::Printf(TEXT("%s.%s"), *InLevelPath, *FPaths::GetBaseFilename(InLevelPath));
    FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(FullPath));
    
    if (!AssetData.IsValid())
    {
        AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(InLevelPath));
    }

    if (AssetData.IsValid())
    {
        TSharedPtr<FAssetThumbnail> Thumbnail = MakeShared<FAssetThumbnail>(AssetData, 256, 256, ThumbnailPool);

        return SNew(SToolTip)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 4)
            [
                SNew(STextBlock)
                .Text(FText::FromString(FPaths::GetBaseFilename(InLevelPath)))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SBox)
                .WidthOverride(256.0f)
                .HeightOverride(256.0f)
                [
                    Thumbnail->MakeThumbnailWidget()
                ]
            ]
        ];
    }
    
    return nullptr;
}

void FLevelSelectionModule::ShowNotification(const FText& InMessage)
{
    FNotificationInfo Info(InMessage);
    Info.ExpireDuration = 2.5f;
    FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Success);
}

static FString NormalizePackagePath(const FString& InPath)
{
    if (InPath.IsEmpty())
        return FString();

    FString Path = InPath;

    int32 ColonIdx = INDEX_NONE;
    if (Path.FindChar(TEXT(':'), ColonIdx))
    {
        Path = Path.Left(ColonIdx);
    }

    int32 DotIdx = INDEX_NONE;
    if (Path.FindChar(TEXT('.'), DotIdx))
    {
        Path = Path.Left(DotIdx);
    }

    Path = Path.TrimStartAndEnd();

    if (Path.IsEmpty() || Path.Contains(TEXT("PersistentLevel")) || Path.Contains(TEXT("persistentlevel")))
        return FString();

    return Path;
}

void FLevelSelectionModule::EnsureMapCache()
{
    if (bIsCacheValid)
        return;

    CachedMapPaths.Empty();

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    auto AddFromPackagePath = [this, &AssetRegistry](const FString& PackagePathStr)
    {
        if (PackagePathStr.IsEmpty())
            return;

        FARFilter Filter;
        Filter.ClassPaths.Add(UWorld::StaticClass()->GetClassPathName());
        Filter.PackagePaths.Add(FName(*PackagePathStr));
        Filter.bRecursivePaths = true;

        TArray<FAssetData> Assets;
        AssetRegistry.GetAssets(Filter, Assets);

        for (const FAssetData& A : Assets)
        {
            const FString PackageName = A.PackageName.ToString();
            const FString SoftPath = A.GetSoftObjectPath().ToString();

            FString Final = NormalizePackagePath(PackageName);
            if (Final.IsEmpty())
            {
                Final = NormalizePackagePath(SoftPath);
            }

            if (!Final.IsEmpty())
            {
                CachedMapPaths.Add(Final);
            }
        }
    };

    AddFromPackagePath(TEXT("/Game"));

    const TArray<TSharedRef<IPlugin>> Plugins = IPluginManager::Get().GetDiscoveredPlugins();
    for (const TSharedRef<IPlugin>& Plugin : Plugins)
    {
        if (!Plugin->IsEnabled())
            continue;

        const FString MountedPath = Plugin->GetMountedAssetPath();
        if (!MountedPath.IsEmpty())
        {
            AddFromPackagePath(MountedPath);
        }
    }

    bIsCacheValid = true;
    UE_LOG(LogTemp, Display, TEXT("[LevelSelection] Map cache generated. Found %d maps."), CachedMapPaths.Num());
}

void FLevelSelectionModule::ValidateCategories()
{
    if (!bIsCacheValid)
        return;

    bool bModified = false;
    
    for (auto It = FavoriteLevels.CreateIterator(); It; ++It)
    {
        if (!CachedMapPaths.Contains(*It))
        {
            It.RemoveCurrent();
            bModified = true;
        }
    }

    for (auto It = LevelCategories.CreateIterator(); It; ++It)
    {
        TArray<FString>& Levels = It.Value();
        const int32 Before = Levels.Num();

        Levels.RemoveAll([this](const FString& Path)
        {
            return !CachedMapPaths.Contains(Path);
        });

        if (Before != Levels.Num())
        {
            bModified = true;
        }

        if (Levels.Num() == 0)
        {
            CategoryOrder.Remove(It.Key());
            It.RemoveCurrent();
            bModified = true;
        }
    }

    if (bModified)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LevelSelection] Cleaned up obsolete levels from configuration."));
        SaveCategoriesToConfig();
    }
}

TArray<FString> FLevelSelectionModule::GetUncategorizedMapPaths() const
{
    TArray<FString> All = CachedMapPaths.Array();
    for (const auto& Pair : LevelCategories)
    {
        for (const FString& P : Pair.Value)
        {
            All.Remove(P);
        }
    }
    
    All.Sort();
    return All;
}

// ===================================================================================
// ==========                 ASSET REGISTRY CALLBACKS                      ==========
// ===================================================================================

void FLevelSelectionModule::OnFilesLoaded()
{
    EnsureMapCache();
    ValidateCategories();
}

void FLevelSelectionModule::OnAssetAdded(const FAssetData& InAssetData)
{
    if (!bIsCacheValid)
        return;

    if (!InAssetData.AssetClassPath.ToString().Contains(TEXT("World")))
        return;

    const FString FinalPath = NormalizePackagePath(InAssetData.PackageName.ToString());
    if (!FinalPath.IsEmpty())
    {
        CachedMapPaths.Add(FinalPath);
        UE_LOG(LogTemp, Log, TEXT("[LevelSelection] New map detected and cached: %s"), *FinalPath);
    }
}

void FLevelSelectionModule::OnAssetRemoved(const FAssetData& InAssetData)
{
    if (!bIsCacheValid)
        return;

    if (!InAssetData.AssetClassPath.ToString().Contains(TEXT("World")))
        return;

    const FString RemovedMapPath = NormalizePackagePath(InAssetData.PackageName.ToString());
    
    if (!RemovedMapPath.IsEmpty())
    {
        CachedMapPaths.Remove(RemovedMapPath);

        bool bModified = false;
        
        if (FavoriteLevels.Remove(RemovedMapPath) > 0)
        {
            bModified = true;
        }

        for (auto It = LevelCategories.CreateIterator(); It; ++It)
        {
            TArray<FString>& Levels = It.Value();
            const int32 Before = Levels.Num();

            Levels.RemoveAll([&](const FString& LevelPath)
            {
                return LevelPath == RemovedMapPath;
            });

            if (Before != Levels.Num())
            {
                bModified = true;
            }

            if (Levels.Num() == 0)
            {
                CategoryOrder.Remove(It.Key());
                It.RemoveCurrent();
            }
        }
        
        if (bModified)
        {
            UE_LOG(LogTemp, Log, TEXT("[LevelSelection] Map removed, updating config: %s"), *RemovedMapPath);
            SaveCategoriesToConfig();
        }
    }
}

void FLevelSelectionModule::OnAssetRenamed(const FAssetData& InAssetData, const FString& InOldObjectPath)
{
    if (!bIsCacheValid)
        return;

    if (!InAssetData.AssetClassPath.ToString().Contains(TEXT("World")))
        return;

    const FString OldPath = NormalizePackagePath(InOldObjectPath);
    const FString NewPath = NormalizePackagePath(InAssetData.PackageName.ToString());
    
    if (!OldPath.IsEmpty())
    {
        CachedMapPaths.Remove(OldPath);
    }
    if (!NewPath.IsEmpty())
    {
        CachedMapPaths.Add(NewPath);
    }
    
    bool bModified = false;
    
    if (FavoriteLevels.Remove(OldPath) > 0)
    {
        FavoriteLevels.Add(NewPath);
        bModified = true;
    }

    for (auto It = LevelCategories.CreateIterator(); It; ++It)
    {
        TArray<FString>& Levels = It.Value();
        const int32 Index = Levels.IndexOfByKey(OldPath);
        if (Index != INDEX_NONE)
        {
            Levels[Index] = NewPath;
            bModified = true;
        }
    }
    
    if (bModified)
    {
        UE_LOG(LogTemp, Log, TEXT("[LevelSelection] Map renamed, updating config: %s -> %s"), *OldPath, *NewPath);
        SaveCategoriesToConfig();
    }
}

// ===================================================================================
// ==========                     CONFIGURATION LOGIC                       ==========
// ===================================================================================

void FLevelSelectionModule::SaveCategoriesToConfig()
{
    const FString Path = FPaths::ProjectConfigDir() / TEXT("LevelCategories.ini");

    FConfigFile ConfigFile;

    const FString OrderString = FString::Join(CategoryOrder, TEXT(","));
    ConfigFile.SetString(TEXT("CategoryOrder"), TEXT("Order"), *OrderString);
    
    const FString FavCombined = FString::Join(FavoriteLevels.Array(), TEXT(","));
    ConfigFile.SetString(TEXT("Favorites"), TEXT("Levels"), *FavCombined);

    for (const auto& Pair : LevelCategories)
    {
        const FString& Category = Pair.Key;
        const FString Combined = FString::Join(Pair.Value, TEXT(","));
        ConfigFile.SetString(*Category, TEXT("Levels"), *Combined);
    }

    ConfigFile.Dirty = true;
    ConfigFile.Write(Path);
    
    UE_LOG(LogTemp, Verbose, TEXT("[LevelSelection] Config saved to: %s"), *Path);
}

void FLevelSelectionModule::LoadCategoriesFromConfig()
{
    const FString Path = FPaths::ProjectConfigDir() / TEXT("LevelCategories.ini");
    if (!FPaths::FileExists(Path))
        return;

    FConfigFile ConfigFile;
    ConfigFile.Read(Path);

    LevelCategories.Empty();
    CategoryOrder.Empty();
    FavoriteLevels.Empty();

    FString Order;
    if (ConfigFile.GetString(TEXT("CategoryOrder"), TEXT("Order"), Order))
    {
        Order.ParseIntoArray(CategoryOrder, TEXT(","), true);
    }
    
    FString FavStr;
    if (ConfigFile.GetString(TEXT("Favorites"), TEXT("Levels"), FavStr))
    {
        TArray<FString> Favs;
        FavStr.ParseIntoArray(Favs, TEXT(","), true);
        FavoriteLevels.Append(Favs);
    }

    const FConfigFile& ConstConfigFile = AsConst(ConfigFile);
    for (const auto& SectionPair : ConstConfigFile)
    {
        const FString& Section = SectionPair.Key;

        if (Section == TEXT("CategoryOrder") || Section == TEXT("Favorites"))
            continue;

        FString LevelsStr;
        if (ConfigFile.GetString(*Section, TEXT("Levels"), LevelsStr))
        {
            TArray<FString> Levels;
            LevelsStr.ParseIntoArray(Levels, TEXT(","), true);
            LevelCategories.Add(Section, Levels);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("[LevelSelection] Config loaded from: %s"), *Path);
}

void FLevelSelectionModule::LoadCategoryOrderFromConfig(const FString& InConfigPath)
{
    FString Order;
    if (GConfig->GetString(TEXT("CategoryOrder"), TEXT("Order"), Order, InConfigPath))
    {
        Order.ParseIntoArray(CategoryOrder, TEXT(","), true);
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLevelSelectionModule, LevelSelection)

#endif