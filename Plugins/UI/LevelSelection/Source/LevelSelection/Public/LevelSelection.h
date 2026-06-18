// Copyright (c) Bixboy, 2025. All Rights Reserved.
#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class FMenuBarBuilder;
class FMenuBuilder;
class SWindow;
class FAssetThumbnailPool;
class SToolTip;
struct FAssetData;

/**
 * Editor module handling the custom Level Selection menu.
 */
class FLevelSelectionModule : public IModuleInterface
{
public:
	
	// ========== MODULE INITIALIZATION / SHUTDOWN =========
	
	/** Called right after the module DLL has been loaded and the module object has been created */
	virtual void StartupModule() override;
	
	/** Called before the module is unloaded, right before the module object is destroyed */
	virtual void ShutdownModule() override;

private:
	
	// ========== MENU =========
	
	/** Adds the pull-down menu to the main menu bar */
	void AddMenuEntry(FMenuBarBuilder& MenuBuilder);
	
	/** Fills the pull-down menu with categories and uncategorized levels */
	void FillSubmenu(FMenuBuilder& MenuBuilder);
	
	/** Populates a specific category with its associated levels */
	void PopulateCategoryMenu(FMenuBuilder& MenuBuilder, const FString& InCategoryName, const TArray<FString>& InLevels);
	
	/** Populates the menu with levels that do not belong to any category */
	void PopulateUncategorizedMenu(FMenuBuilder& MenuBuilder);
	
	/** Generates the menu entry for a single level, including its options submenu */
	void AddLevelEntryToMenu(FMenuBuilder& MenuBuilder, const FString& InLevelPath);

	// ========== ACTIONS =========
	
	/** Opens the specified level */
	void OnOpenLevel(const FString& InLevelPath);
	
	/** Copies the name of the specified level to the clipboard */
	void OnCopyLevelName(const FString& InLevelPath);
	
	/** Opens a dialog to create a new category and moves the specified level into it */
	void OnCreateNewCategory(const FString& InLevelPath);
	
	/** Sets the specified level as the default editor startup map */
	void OnSetAsDefaultEditorLevel(const FString& InLevelPath);
	
	/** Moves the specified level to the target category, or makes it uncategorized if target is empty */
	void OnMoveLevelToCategory(const FString& InLevelPath, const FString& InTargetCategory);
	
	/** Deletes the specified category and makes all its levels uncategorized */
	void OnDeleteCategory(const FString& InCategoryName);
	
	/** Toggles the favorite status of the specified level */
	void OnToggleFavorite(const FString& InLevelPath);
	
	/** Opens the Content Browser and highlights the specified level asset */
	void OnLocateInContentBrowser(const FString& InLevelPath);

	// ========== HELPERS =========
	
	/** Creates a Slate Tooltip containing the level's thumbnail */
	TSharedPtr<SToolTip> CreateCustomTooltip(const FString& InLevelPath);
	
	/** Ensures the level cache is populated */
	void EnsureMapCache();
	
	/** Validates saved categories against the currently known levels */
	void ValidateCategories();
	
	/** Retrieves all uncategorized map paths based on the cache */
	TArray<FString> GetUncategorizedMapPaths() const;
	
	/** Displays a temporary non-intrusive notification */
	void ShowNotification(const FText& InMessage);

	// ========== ASSET REGISTRY CALLBACKS =========
	
	/** Called when the asset registry has finished its initial background discovery */
	void OnFilesLoaded();
	
	/** Called when a new asset is added to the registry */
	void OnAssetAdded(const FAssetData& InAssetData);
	
	/** Called when an asset is removed from the registry */
	void OnAssetRemoved(const FAssetData& InAssetData);
	
	/** Called when an asset is renamed in the registry */
	void OnAssetRenamed(const FAssetData& InAssetData, const FString& InOldObjectPath);

	// ========== CONFIGURATION PERSISTENCE =========
	
	/** Saves the current categories and order to the config file */
	void SaveCategoriesToConfig();
	
	/** Loads the categories from the config file */
	void LoadCategoriesFromConfig();
	
	/** Loads the category order from the config file */
	void LoadCategoryOrderFromConfig(const FString& InConfigPath);

	// ========== VARIABLES =========
	
	/** Map of category names to lists of level paths */
	TMap<FString, TArray<FString>> LevelCategories;
	
	/** Ordered list of category names for menu display */
	TArray<FString> CategoryOrder;
	
	/** Set of level paths marked as favorites */
	TSet<FString> FavoriteLevels;

	/** Set of all discovered map paths (cached for performance) */
	TSet<FString> CachedMapPaths;
	
	/** Flag indicating if the map cache has been populated */
	bool bIsCacheValid = false;

	/** Pointer to the currently open "New Category" window */
	TSharedPtr<SWindow> NewCategoryWindow;
	
	/** Thumbnail pool for asynchronous loading of map preview images */
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;
};

#endif // WITH_EDITOR