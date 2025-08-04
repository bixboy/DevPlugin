// Copyright (c) Bixboy, 2025. All Rights Reserved.
#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

class FMenuBuilder;

class FLevelSelectionModule : public IModuleInterface
{
public:
	
	// ========== MODULE INITIALIZATION / SHUTDOWN =========
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	
	// ========== MENU =========
	void AddMenuEntry(FMenuBarBuilder& MenuBuilder);
	void FillSubmenu(FMenuBuilder& MenuBuilder);
	void PopulateCategoryMenu(FMenuBuilder& MenuBuilder, FString CategoryName, TArray<FString> Levels);
	void PopulateUncategorizedMenu(FMenuBuilder& MenuBuilder);

	// ========== ACTIONS =========
	void OnOpenLevel(FString LevelPath);
	void OnCopyLevelName(FString LevelPath);
	void OnCreateNewCategory(FString LevelPath);
	void OnMoveLevelToCategory(FString LevelPath, FString TargetCategory);
	void OnDeleteCategory(FString CategoryName);
	void OnAssetRemoved(const FAssetData& AssetData);

	
	// ========== HELPERS =========
	TArray<FString> GetAllMapPaths() const;
	TArray<FString> GetUncategorizedMapPaths() const;
	void ShowNotification(const FText& Message);

	// ========== CONFIGURATION PERSISTENCE =========
	void SaveCategoriesToConfig();
	void LoadCategoriesFromConfig();
	void LoadCategoryOrderFromConfig(const FString& ConfigPath);

	// ========== VARIABLES =========
	TMap<FString, TArray<FString>> LevelCategories;
	
	TArray<FString> CategoryOrder;

	TSharedPtr<SWindow> NewCategoryWindow;
};

#endif // WITH_EDITOR