using UnrealBuildTool;
using System.Collections.Generic;

public class DownfallTarget : TargetRules
{
    public DownfallTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Downfall");
    }
}
