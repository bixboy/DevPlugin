using System.IO;
using UnrealBuildTool;

public class StellarLocomotionCharactersModule : ModuleRules
{
    public StellarLocomotionCharactersModule(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", 
                "AnimationBudgetAllocator",
            }
        );
        
        

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "StellarLocomotionPlugin", 
                "EnhancedInput", 
                "AnimGraphRuntime",
            }
        );
    }
}