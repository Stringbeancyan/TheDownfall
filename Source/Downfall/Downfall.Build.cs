using UnrealBuildTool;

public class Downfall : ModuleRules
{
    public Downfall(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
            "NetCore", "AIModule", "GameplayTasks", "NavigationSystem",
            "GameplayTags", "GameplayAbilities", "Niagara"
        });
    }
}
