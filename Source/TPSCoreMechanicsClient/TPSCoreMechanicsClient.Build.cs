// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TPSCoreMechanicsClient : ModuleRules
{
	public TPSCoreMechanicsClient(ReadOnlyTargetRules Target) : base(Target)
	{
		if (Target.Type == TargetType.Server)
		{
			throw new BuildException("TPSCoreMechanicsClient must not be built for server targets. Server builds should be NATS-only and exclude TurboLink/gRPC client code.");
		}

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

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"InputCore",
			"EnhancedInput",
			"Slate",
			"SlateCore",
		});
	}
}
