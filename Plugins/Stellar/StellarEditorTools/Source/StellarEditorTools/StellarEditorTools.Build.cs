using UnrealBuildTool;

public class StellarEditorTools : ModuleRules
{
	public StellarEditorTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);
			
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"Blutility",
				"UMGEditor",
				"DesktopPlatform",
				"ImageWrapper",
				"EditorFramework",
				"ToolMenus",
				"ContentBrowser",
				"EditorStyle",
				"EditorSubsystem",
				"PropertyEditor",
				"StellarLocomotionVehiclesModule"
			}
		);
	}
}
