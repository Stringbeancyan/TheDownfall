using UnrealBuildTool;
using System.Collections.Generic;

public class DownfallServerTarget : TargetRules
{
    public DownfallServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
        ExtraModuleNames.Add("Downfall");
    }
}
