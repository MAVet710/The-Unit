using UnrealBuildTool;

public class TheUnit : ModuleRules
{
    public TheUnit(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "UMG",
            "Slate",
            "SlateCore",
            "RawInput"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });
    }
}
