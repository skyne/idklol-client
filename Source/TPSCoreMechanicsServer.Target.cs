// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class TPSCoreMechanicsServerTarget : TargetRules
{
	public TPSCoreMechanicsServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;

		// Server builds only the shared gameplay module.
		// TPSCoreMechanicsClient (gRPC, character creation, UI, auth) is excluded.
		ExtraModuleNames.Add("TPSCoreMechanics");

		if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			bOverrideBuildEnvironment = true;
			AdditionalCompilerArguments += " -Wno-deprecated-literal-operator -Wno-deprecated-builtins";
		}
	}
}
