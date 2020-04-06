// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class UnrealCV: ModuleRules
	{
		public UnrealCV(ReadOnlyTargetRules  Target): base(Target)
		{
			PrivateIncludePaths.AddRange(
				new string[] {
				}
				);

			PrivateIncludePaths.Add("C:/ProgramData/chocolatey/lib/eigen/include");

            PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

            MinFilesUsingPrecompiledHeaderOverride = 1;
            bFasterWithoutUnity = true;

            PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "RenderCore", "Networking", "Sockets", "Slate", "ImageWrapper"});
            /*
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					// ... add other public dependencies that you statically link with here ...
				}
				);
                */

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					// ... add private dependencies that you statically link with here ...
                    // "Renderer"
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
                    "Renderer"
				}
				);
		}
	}
}