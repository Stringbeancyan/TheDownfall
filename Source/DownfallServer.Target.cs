using UnrealBuildTool;
using System.Collections.Generic;

public class DownfallServerTarget : TargetRules
{
    public DownfallServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Downfall");
    }
}
