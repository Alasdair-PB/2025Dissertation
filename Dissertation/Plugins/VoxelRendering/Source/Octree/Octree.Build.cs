using System.IO;
using UnrealBuildTool;

public class Octree : ModuleRules
{
	public Octree(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(new string[] {});	
		PrivateIncludePaths.AddRange(new string[] {});

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "VoxelRenderingUtils", "CoreUObject", "Engine"});
        PrivateDependencyModuleNames.AddRange(new string[] { "CoreUObject", "VoxelRenderingUtils", "Engine"});
		DynamicallyLoadedModuleNames.AddRange(new string[]{});
	}
}
