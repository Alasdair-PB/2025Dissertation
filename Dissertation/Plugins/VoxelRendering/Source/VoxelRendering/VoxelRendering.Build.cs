using System.IO;
using UnrealBuildTool;

public class VoxelRendering : ModuleRules
{
	public VoxelRendering(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
                Path.Combine(ModuleDirectory, "VoxelGeneration", "Public"),
            });	
		
		PrivateIncludePaths.AddRange(
			new string[] {
                Path.Combine(ModuleDirectory, "VoxelGeneration", "Private"),
            });

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "MyShaders", "Octree", "CoreUObject", "Engine", "InputCore", "ProceduralMeshComponent" });
        PrivateDependencyModuleNames.AddRange(new string[] { "CoreUObject", "Engine","Slate", "SlateCore" });
		DynamicallyLoadedModuleNames.AddRange(new string[]{});
	}
}
