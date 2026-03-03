// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TPSCoreMechanics : ModuleRules
{
	public TPSCoreMechanics(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "GameplayAbilities", "GameplayTags", "GameplayTasks", "HTTP", "Json", "JsonUtilities" });

		// NatsClient drives all server↔service communication (map management, character loading, etc.)
		// Compiles as no-op stubs until nats.c is vendored via Plugins/NatsClient/Source/NatsClient/ThirdParty/fetch_nats.sh
		PrivateDependencyModuleNames.Add("NatsClient");
	}
}
