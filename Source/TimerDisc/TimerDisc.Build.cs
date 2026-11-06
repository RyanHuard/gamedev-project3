using UnrealBuildTool;

public class TimerDisc : ModuleRules
{
	public TimerDisc(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "Slate", "SlateCore", "RenderCore"
		});
	}
}
