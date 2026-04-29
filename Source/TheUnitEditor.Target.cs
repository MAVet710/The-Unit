using UnrealBuildTool;
using System.Collections.Generic;

public class TheUnitEditorTarget : TargetRules
{
    public TheUnitEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        ExtraModuleNames.Add("TheUnit");
    }
}
