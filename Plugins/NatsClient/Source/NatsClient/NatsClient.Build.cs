// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class NatsClient : ModuleRules
{
	public NatsClient(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnableExceptions = true; // nats.c uses errno / setjmp patterns

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Sockets",
			"SSL",
		});

		// ── nats.c ThirdParty integration ─────────────────────────────────────
		// Vendor nats.c by placing prebuilt static libs or source under:
		//   Plugins/NatsClient/Source/NatsClient/ThirdParty/nats.c/
		//
		// Prebuilt lib layout (preferred for CI):
		//   ThirdParty/nats.c/lib/Win64/nats_static.lib
		//   ThirdParty/nats.c/lib/Mac/libnats_static.a
		//   ThirdParty/nats.c/lib/Linux/libnats_static.a
		//   ThirdParty/nats.c/include/nats/nats.h
		//
		// To build from source, add the nats.c *.c files to PrivateAdditionalSources.

		string ThirdPartyPath = Path.Combine(ModuleDirectory, "ThirdParty", "nats.c");
		string IncludePath = Path.Combine(ThirdPartyPath, "include");
		string LibPath = Path.Combine(ThirdPartyPath, "lib", Target.Platform.ToString());

		if (Directory.Exists(IncludePath))
		{
			PublicIncludePaths.Add(IncludePath);
		}

		if (Directory.Exists(LibPath))
		{
			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				PublicAdditionalLibraries.Add(Path.Combine(LibPath, "nats_static.lib"));
			}
			else if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libnats_static.a"));
			}
			else if (Target.Platform == UnrealTargetPlatform.Linux)
			{
				PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libnats_static.a"));
			}
		}
		else
		{
			// nats.c not yet vendored — plugin will compile but NATS calls will be no-ops.
			// Run: Plugins/NatsClient/Source/NatsClient/ThirdParty/fetch_nats.sh
			PublicDefinitions.Add("NATS_CLIENT_NOT_AVAILABLE=1");
		}
	}
}
