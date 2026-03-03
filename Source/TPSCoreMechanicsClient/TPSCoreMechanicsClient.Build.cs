// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TPSCoreMechanicsClient : ModuleRules
{
	public TPSCoreMechanicsClient(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"TPSCoreMechanics",
			"TurboLinkGrpc",
			"HTTP",
			"Json",
			"JsonUtilities",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"UMG",
		});

		CircularlyReferencedDependentModules.Add("TPSCoreMechanics");

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"InputCore",
			"EnhancedInput",
			"Slate",
			"SlateCore",
		});
	}
}
