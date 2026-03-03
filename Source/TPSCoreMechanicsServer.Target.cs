// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class TPSCoreMechanicsServerTarget : TargetRules
{
	public TPSCoreMechanicsServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;

		// Server builds only the shared gameplay module.
		// TPSCoreMechanicsClient (gRPC, character creation, UI, auth) is excluded.
		ExtraModuleNames.Add("TPSCoreMechanics");
	}
}
