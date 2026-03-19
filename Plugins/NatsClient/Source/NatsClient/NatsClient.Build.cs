// Copyright Epic Games, Inc. All Rights Reserved.

using System;
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
		bool bHasInclude = Directory.Exists(IncludePath);
		bool bHasLibrary = false;

		if (bHasInclude)
		{
			PublicIncludePaths.Add(IncludePath);
		}

		if (Directory.Exists(LibPath))
		{
			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				string LibraryPath = Path.Combine(LibPath, "nats_static.lib");
				if (File.Exists(LibraryPath))
				{
					PublicAdditionalLibraries.Add(LibraryPath);
					bHasLibrary = true;
				}
			}
			else if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				string LibraryPath = Path.Combine(LibPath, "libnats_static.a");
				if (File.Exists(LibraryPath))
				{
					PublicAdditionalLibraries.Add(LibraryPath);
					bHasLibrary = true;
				}
			}
			else if (Target.Platform == UnrealTargetPlatform.Linux)
			{
				string LibraryPath = Path.Combine(LibPath, "libnats_static.a");
				if (File.Exists(LibraryPath))
				{
					PublicAdditionalLibraries.Add(LibraryPath);
					bHasLibrary = true;
				}
			}
		}

		bool bNatsAvailable = bHasInclude && bHasLibrary;
		bool bNatsRequired = string.Equals(
			Environment.GetEnvironmentVariable("NATS_CLIENT_REQUIRED"),
			"1",
			StringComparison.OrdinalIgnoreCase);

		if (bNatsRequired && !bNatsAvailable)
		{
			throw new BuildException(
				"NatsClient: NATS is required (NATS_CLIENT_REQUIRED=1) but vendor artifacts are missing for platform '{0}'. " +
				"Expected include at '{1}' and static library under '{2}'.",
				Target.Platform,
				IncludePath,
				LibPath);
		}

		if (!bNatsAvailable)
		{
			// nats.c not yet vendored — plugin will compile but NATS calls will be no-ops.
			// Run: Plugins/NatsClient/Source/NatsClient/ThirdParty/fetch_nats.sh
			PublicDefinitions.Add("NATS_CLIENT_NOT_AVAILABLE=1");
		}
		else
		{
			PublicDefinitions.Add("NATS_CLIENT_NOT_AVAILABLE=0");
		}
	}
}
