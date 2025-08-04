// Copyright (c) Bixboy, 2024. All Rights Reserved.
using UnrealBuildTool;

public class LevelSelection : ModuleRules
{
	public LevelSelection(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore"
			}
			);
		
		
		// Dépendances privées pour l'éditeur
		PrivateDependencyModuleNames.AddRange(new string[] {
			"Projects",
			"InputCore",
			"EditorFramework",
			"UnrealEd",
			"ToolMenus",
			"PropertyEditor",
			"AssetRegistry",
			"EditorSubsystem",
			"ApplicationCore"
		});
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
