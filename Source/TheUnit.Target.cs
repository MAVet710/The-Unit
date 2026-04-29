using UnrealBuildTool;
using System.Collections.Generic;

public class TheUnitTarget : TargetRules
{
    public TheUnitTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        ExtraModuleNames.Add("TheUnit");
    }
}
