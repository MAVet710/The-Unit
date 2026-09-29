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
            "AIModule",
            "NavigationSystem",
            "InputCore",
            "UMG",
            "Slate",
            "SlateCore"
        });

        // Persistent participant identity uses FUniqueNetIdWrapper::ToString from CoreOnline.
        PrivateDependencyModuleNames.AddRange(new[] { "CoreOnline", "AnimationCore" });
    }
}
