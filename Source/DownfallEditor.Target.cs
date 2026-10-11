using UnrealBuildTool;
using System.Collections.Generic;

public class DownfallEditorTarget : TargetRules
{
    public DownfallEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Downfall");
    }
}
