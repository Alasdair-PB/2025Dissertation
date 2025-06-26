using System.IO;
using UnrealBuildTool;

public class VoxelRendering : ModuleRules
{
	public VoxelRendering(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;

        PublicIncludePaths.AddRange(new string[] {});
        PrivateIncludePaths.AddRange(new string[] {});

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "VoxelRenderingUtils", "VoxelShaders", "Octree", "CoreUObject", "Engine", "MaterialShaderQualitySettings", "InputCore", "ProceduralMeshComponent" });
        PrivateDependencyModuleNames.AddRange(new string[] { "CoreUObject", "Core", "VoxelRenderingUtils", "VoxelShaders", "Engine", "ComputeDispatchers", "Renderer", "Octree", "RenderCore", "RHI", "Projects" });

        if (Target.bBuildEditor == true)
            PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "MaterialUtilities", "SlateCore", "Slate", "TargetPlatform" });

        DynamicallyLoadedModuleNames.AddRange(new string[]{});
	}
}
