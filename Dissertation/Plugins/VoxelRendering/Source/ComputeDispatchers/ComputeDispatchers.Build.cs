using System.IO;
using UnrealBuildTool;

public class ComputeDispatchers : ModuleRules
{
    public ComputeDispatchers(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange( new string[] {});
        PrivateIncludePaths.AddRange( new string[] {});

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "VoxelRenderingUtils", "Octree", "CoreUObject", "Engine", "MaterialShaderQualitySettings", "InputCore", "ProceduralMeshComponent" });
        PrivateDependencyModuleNames.AddRange(new string[] { "CoreUObject", "VoxelRenderingUtils", "Renderer", "Octree", "RenderCore", "RHI", "Projects" });

        if (Target.bBuildEditor == true)
            PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "MaterialUtilities", "SlateCore", "Slate", "TargetPlatform" });

        DynamicallyLoadedModuleNames.AddRange(new string[] { });
    }

}