using System.IO;
using UnrealBuildTool;

public class VoxelShaders : ModuleRules
{
    public VoxelShaders(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange( new string[] {});
        PrivateIncludePaths.AddRange( new string[] {});

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "MaterialShaderQualitySettings"});
        PrivateDependencyModuleNames.AddRange(new string[] { "CoreUObject", "Renderer", "RenderCore", "RHI", "Projects" });

        if (Target.bBuildEditor == true)
            PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "MaterialUtilities", "SlateCore", "Slate", "TargetPlatform" });

        DynamicallyLoadedModuleNames.AddRange(new string[] { });
    }

}