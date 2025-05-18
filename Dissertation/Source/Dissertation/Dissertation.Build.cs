using UnrealBuildTool;

public class Dissertation : ModuleRules
{
	public Dissertation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "ComputeDispatchers", "Octree", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "ProceduralMeshComponent" });
        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        //PublicIncludePaths.AddRange(new string[] { "Dissertation/Octrees/Public", "Dissertation/VoxelRenderer/Public", "Dissertation/VoxelGeneration/Public" });
        //PrivateIncludePaths.AddRange(new string[] { "Dissertation/Octrees/Private", "Dissertation/VoxelRenderer/Private", "Dissertation/VoxelGeneration/Private" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
