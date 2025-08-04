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
#include "Engine/ObjectLibrary.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SWindow.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Interfaces/IPluginManager.h"


#define LOCTEXT_NAMESPACE "FLevelSelectionModule"

DEFINE_LOG_CATEGORY_STATIC(LogLevelSelection, Log, All);

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

        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        AssetRegistryModule.Get().OnAssetRemoved().AddRaw(this, &FLevelSelectionModule::OnAssetRemoved);
    }
}

void FLevelSelectionModule::ShutdownModule()
{
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
    // Catégories existantes
    for (const FString& Cat : CategoryOrder)
    {
        const TArray<FString>& Levels = LevelCategories.FindChecked(Cat);
        MenuBuilder.AddSubMenu(
            FText::FromString(Cat),
            LOCTEXT("CategoryTooltip", "Options for this category"),
            FNewMenuDelegate::CreateRaw(this, &FLevelSelectionModule::PopulateCategoryMenu, Cat, Levels)
        );
    }

    // Niveaux non catégorisés
    MenuBuilder.AddSubMenu(
        LOCTEXT("UncategorizedLabel", "Uncategorized Levels"),
        LOCTEXT("UncategorizedTooltip", "Levels not in any category"),
        FNewMenuDelegate::CreateRaw(this, &FLevelSelectionModule::PopulateUncategorizedMenu)
    );
}

void FLevelSelectionModule::PopulateCategoryMenu(FMenuBuilder& MenuBuilder, FString CategoryName, TArray<FString> Levels)
{
    if (Levels.Num() == 0)
    {
        MenuBuilder.AddMenuEntry(
            LOCTEXT("EmptyCategory", "No levels in this category."),
            FText::GetEmpty(),
            FSlateIcon(),
            FUIAction()
        );
        return;
    }

    for (const FString& Path : Levels)
    {
        const FString Name = FPaths::GetBaseFilename(Path);

        MenuBuilder.BeginSection(NAME_None, FText::FromString(Name));

        // Ouvrir
        MenuBuilder.AddMenuEntry(
            FText::FromString(Name),
            FText::Format(LOCTEXT("OpenFmt", "Open {0}"), FText::FromString(Name)),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateRaw(this, &FLevelSelectionModule::OnOpenLevel, Path))
        );

        // Options
        MenuBuilder.AddSubMenu(
            LOCTEXT("OptionsLabel", "Options"),
            LOCTEXT("OptionsTooltip", "Hover to show options"),
            FNewMenuDelegate::CreateLambda([this, Path](FMenuBuilder& OptionsBuilder)
            {
                // Copier le nom
                OptionsBuilder.AddMenuEntry(
                    LOCTEXT("CopyName", "Copy Level Name"),
                    LOCTEXT("CopyNameTooltip", "Copy level name to clipboard"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateRaw(this, &FLevelSelectionModule::OnCopyLevelName, Path))
                );

                // Créer nouvelle catégorie
                OptionsBuilder.AddMenuEntry(
                    LOCTEXT("CreateCategory", "Create New Category"),
                    LOCTEXT("CreateCategoryTooltip", "Create a new category and move this level"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateRaw(this, &FLevelSelectionModule::OnCreateNewCategory, Path))
                );

                OptionsBuilder.AddSeparator();

                // Déplacer vers catégories
                for (const auto& Pair : LevelCategories)
                {
                    const FString& TargetCat = Pair.Key;

                    // Ne pas afficher si déjà dans cette catégorie
                    if (Pair.Value.Contains(Path)) continue;

                    OptionsBuilder.AddMenuEntry(
                        FText::FromString(TargetCat),
                        FText::Format(LOCTEXT("MoveFmt", "Move to {0}"), FText::FromString(TargetCat)),
                        FSlateIcon(),
                        FUIAction(FExecuteAction::CreateRaw(this, &FLevelSelectionModule::OnMoveLevelToCategory, Path, TargetCat))
                    );
                }

                // Déplacer vers "Uncategorized" uniquement si ce n'est pas déjà le cas
                bool bInCategory = false;
                for (const auto& Pair : LevelCategories)
                {
                    if (Pair.Value.Contains(Path))
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
                        FUIAction(FExecuteAction::CreateRaw(this, &FLevelSelectionModule::OnMoveLevelToCategory, Path, FString()))
                    );
                }
            })
        );

        MenuBuilder.EndSection();
    }
}

void FLevelSelectionModule::PopulateUncategorizedMenu(FMenuBuilder& MenuBuilder)
{
    TArray<FString> Levels = GetUncategorizedMapPaths();
    PopulateCategoryMenu(MenuBuilder, FString(), Levels);
}



// ===================================================================================
// ==========                 LEVEL & CATEGORY MANAGEMENT                   ==========
// ===================================================================================

void FLevelSelectionModule::OnOpenLevel(FString LevelPath)
{
    if (FEditorFileUtils::SaveDirtyPackages(true, true, true))
    {
        FEditorFileUtils::LoadMap(LevelPath, false);
    }
}

void FLevelSelectionModule::OnCopyLevelName(FString LevelPath)
{
    const FString Name = FPaths::GetBaseFilename(LevelPath);
    FPlatformApplicationMisc::ClipboardCopy(*Name);
    
    ShowNotification(FText::Format(LOCTEXT("CopiedFmt", "Level '{0}' copied!"), FText::FromString(Name)));
}

void FLevelSelectionModule::OnCreateNewCategory(FString LevelPath)
{
    TSharedPtr<SEditableTextBox> InputBox;
    NewCategoryWindow = SNew(SWindow)
        
        .Title(LOCTEXT("NewCategoryTitle", "New Category"))
        .ClientSize(FVector2D(300, 120))
        
        .SupportsMinimize(false).SupportsMaximize(false)
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
                .OnClicked_Lambda([this, InputBox, LevelPath]() -> FReply
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

                    LevelCategories.FindOrAdd(NewCat).Add(LevelPath);
                    CategoryOrder.AddUnique(NewCat);
                    OnMoveLevelToCategory(LevelPath, NewCat);

                    if (NewCategoryWindow.IsValid())
                        NewCategoryWindow->RequestDestroyWindow();

                    return FReply::Handled();
                })
            ]
        ];

    FSlateApplication::Get().AddWindow(NewCategoryWindow.ToSharedRef());
}

void FLevelSelectionModule::OnMoveLevelToCategory(FString LevelPath, FString TargetCategory)
{
    for (auto It = LevelCategories.CreateIterator(); It; ++It)
    {
        TArray<FString>& Levels = It.Value();
        Levels.Remove(LevelPath);

        if (Levels.Num() == 0)
        {
            CategoryOrder.Remove(It.Key());
            It.RemoveCurrent();
        }
    }

    if (!TargetCategory.IsEmpty())
    {
        LevelCategories.FindOrAdd(TargetCategory).AddUnique(LevelPath);
        CategoryOrder.AddUnique(TargetCategory);
        ShowNotification(FText::Format(LOCTEXT("MovedFmt", "Moved to '{0}'"), FText::FromString(TargetCategory)));
    }
    else
    {
        ShowNotification(LOCTEXT("MovedToUncategorized", "Moved to Uncategorized"));
    }

    SaveCategoriesToConfig();
}

void FLevelSelectionModule::OnDeleteCategory(FString CategoryName)
{
    if (LevelCategories.Remove(CategoryName) > 0)
    {
        CategoryOrder.Remove(CategoryName);
        SaveCategoriesToConfig();
        ShowNotification(FText::Format(LOCTEXT("DeletedFmt", "Deleted category '{0}'"), FText::FromString(CategoryName)));
    }
}



// ===================================================================================
// ==========                        Helper Functions                       ==========
// ===================================================================================

void FLevelSelectionModule::OnAssetRemoved(const FAssetData& AssetData)
{
    if (!AssetData.AssetClassPath.ToString().Contains("World"))
        return;
    

    const FString RemovedMapPath = AssetData.GetObjectPathString();

    bool bModified = false;

    for (auto It = LevelCategories.CreateIterator(); It; ++It)
    {
        TArray<FString>& Levels = It.Value();
        int32 Before = Levels.Num();

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
        SaveCategoriesToConfig();
    }
}

void FLevelSelectionModule::ShowNotification(const FText& Message)
{
    FNotificationInfo Info(Message);
    Info.ExpireDuration = 2.5f;
    FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Success);
}



// ===================================================================================
// ==========                     CONFIGURATION LOGIC                       ==========
// ===================================================================================

void FLevelSelectionModule::SaveCategoriesToConfig()
{
    const FString Path = FPaths::ProjectConfigDir() / TEXT("LevelCategories.ini");

    // Config personnalisé
    FConfigFile ConfigFile;

    // Ordre des catégories
    FString OrderString = FString::Join(CategoryOrder, TEXT(","));
    ConfigFile.SetString(TEXT("CategoryOrder"), TEXT("Order"), *OrderString);

    // Catégories et niveaux
    for (const auto& Pair : LevelCategories)
    {
        const FString& Category = Pair.Key;
        const FString Combined = FString::Join(Pair.Value, TEXT(","));
        ConfigFile.SetString(*Category, TEXT("Levels"), *Combined);
    }

    // Sauvegarde
    ConfigFile.Dirty = true;
    ConfigFile.Write(Path);
}

void FLevelSelectionModule::LoadCategoriesFromConfig()
{
    const FString Path = FPaths::ProjectConfigDir() / TEXT("LevelCategories.ini");
    if (!FPaths::FileExists(Path)) return;

    FConfigFile ConfigFile;
    ConfigFile.Read(Path);

    LevelCategories.Empty();
    CategoryOrder.Empty();

    // Charger ordre des catégories
    FString Order;
    if (ConfigFile.GetString(TEXT("CategoryOrder"), TEXT("Order"), Order))
    {
        Order.ParseIntoArray(CategoryOrder, TEXT(","), true);
    }

    // Charger chaque catégorie
    for (const auto& SectionPair : ConfigFile)
    {
        const FString& Section = SectionPair.Key;

        if (Section == TEXT("CategoryOrder")) continue;

        FString LevelsStr;
        if (ConfigFile.GetString(*Section, TEXT("Levels"), LevelsStr))
        {
            TArray<FString> Levels;
            LevelsStr.ParseIntoArray(Levels, TEXT(","), true);
            LevelCategories.Add(Section, Levels);
        }
    }

    // Nettoyage
    const TArray<FString> ExistingLevels = GetAllMapPaths();
    TSet<FString> ValidSet(ExistingLevels);

    for (auto It = LevelCategories.CreateIterator(); It; ++It)
    {
        TArray<FString>& Levels = It.Value();
        Levels.RemoveAll([&ValidSet](const FString& Path)
        {
            return !ValidSet.Contains(Path);
        });

        if (Levels.Num() == 0)
        {
            CategoryOrder.Remove(It.Key());
            It.RemoveCurrent();
        }
    }
}

void FLevelSelectionModule::LoadCategoryOrderFromConfig(const FString& ConfigPath)
{
    FString Order;
    if (GConfig->GetString(TEXT("CategoryOrder"), TEXT("Order"), Order, ConfigPath))
    {
        Order.ParseIntoArray(CategoryOrder, TEXT(","), true);
    }
}



// ===================================================================================
// ==========                          UTILS                                ==========
// ===================================================================================

TArray<FString> FLevelSelectionModule::GetAllMapPaths() const
{
    TSet<FString> UniquePaths;

    auto AddPathsFromMountPoint = [&UniquePaths](const FString& MountPoint)
    {
        UObjectLibrary* Lib = UObjectLibrary::CreateLibrary(UWorld::StaticClass(), false, true);
        Lib->LoadAssetDataFromPath(*MountPoint);

        TArray<FAssetData> Assets;
        Lib->GetAssetDataList(Assets);

        for (const FAssetData& A : Assets)
        {
            UniquePaths.Add(A.GetSoftObjectPath().ToString());
        }
    };

    AddPathsFromMountPoint(TEXT("/Game"));

    const TArray<TSharedRef<IPlugin>> Plugins = IPluginManager::Get().GetDiscoveredPlugins();
    for (const TSharedRef<IPlugin>& Plugin : Plugins)
    {
        if (!Plugin->IsEnabled())
            continue;

        const FString PluginName = Plugin->GetName();
        const FString MountPoint = FString::Printf(TEXT("/%s/"), *PluginName);

        AddPathsFromMountPoint(MountPoint);
    }

    TArray<FString> Result = UniquePaths.Array();
    Result.Sort();
    return Result;
}

TArray<FString> FLevelSelectionModule::GetUncategorizedMapPaths() const
{
    TArray<FString> All = GetAllMapPaths();
    for (auto& Pair : LevelCategories)
    {
        for (auto& P : Pair.Value)
        {
            All.Remove(P);
        }
    }
    
    return All;
}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLevelSelectionModule, LevelSelection)

#endif