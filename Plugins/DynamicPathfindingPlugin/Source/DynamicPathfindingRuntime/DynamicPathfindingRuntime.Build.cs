using UnrealBuildTool;

public class DynamicPathfindingRuntime : ModuleRules
{
    public DynamicPathfindingRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "AIModule",
                "NavigationSystem"
            });

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Projects",
                "GameplayTasks"
            });
    }
}
