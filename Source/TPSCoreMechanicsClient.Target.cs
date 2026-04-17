// Copyright 2026 Skyne Studios
using UnrealBuildTool;
using System.Collections.Generic;

public class TPSCoreMechanicsClientTarget : TargetRules
{
    public TPSCoreMechanicsClientTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        ExtraModuleNames.AddRange(new string[] { "TPSCoreMechanics", "TPSCoreMechanicsClient" });

        if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            bOverrideBuildEnvironment = true;
            AdditionalCompilerArguments += " -Wno-deprecated-literal-operator -Wno-deprecated-builtins";
        }
    }
}
