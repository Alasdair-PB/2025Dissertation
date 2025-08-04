using System.IO;
using UnrealBuildTool;

public class VoxelGeneration : ModuleRules
{
	public VoxelGeneration(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(new string[] {});	
		PrivateIncludePaths.AddRange(new string[] {});

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "Niagara", "ComputeDispatchers", "Octree", "Profiling", "CoreUObject", "VoxelRendering", "Engine", "InputCore", "ProceduralMeshComponent"});
        PrivateDependencyModuleNames.AddRange(new string[] { "CoreUObject", "Engine","Slate", "SlateCore" });
		DynamicallyLoadedModuleNames.AddRange(new string[]{});
	}
}
