using UnrealBuildTool;

public class StellarLocomotionVehiclesModule : ModuleRules
{
    public StellarLocomotionVehiclesModule(ReadOnlyTargetRules Target) : base(Target)
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
                "StellarLocomotionPlugin", 
                "EnhancedInput", 
                "ProceduralMeshComponent", 
                "Niagara",
                "UMG",
                "InputCore",
            }
        );
    }
}