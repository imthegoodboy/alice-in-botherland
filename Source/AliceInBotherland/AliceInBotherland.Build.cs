// Alice in Botherland

using UnrealBuildTool;

public class AliceInBotherland : ModuleRules
{
	public AliceInBotherland(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"ChaosVehicles",
			"PhysicsCore",
			"AIModule",
			"Slate",
			"SlateCore",
			"UMG",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"NetCore",
			"MoviePlayer",
			"AudioMixer",
			"DeveloperSettings",
			"ApplicationCore",
			"Landscape",
			"ProceduralMeshComponent",
			"Sockets"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"RenderCore",
			"Json",
			"JsonUtilities"
		});
	}
}
