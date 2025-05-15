using System.IO;
using UnrealBuildTool;

public class Profiling : ModuleRules
{
	public Profiling(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(new string[] {});	
		PrivateIncludePaths.AddRange(new string[] {});

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine"});
        PrivateDependencyModuleNames.AddRange(new string[] { "CoreUObject", "Engine"});
		DynamicallyLoadedModuleNames.AddRange(new string[]{});
	}
}
