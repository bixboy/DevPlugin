using UnrealBuildTool;

public class UltraFlowFieldRuntime : ModuleRules
{
    public UltraFlowFieldRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "NavigationSystem",
                "AIModule"
            });

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "RenderCore",
                "Projects",
                "DeveloperSettings",
                "ApplicationCore"
            });

        PublicIncludePathModuleNames.AddRange(new string[]
        {
            "Engine",
            "CoreUObject"
        });
    }
}
