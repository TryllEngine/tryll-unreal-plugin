// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

using UnrealBuildTool;

public class TryllMCP : ModuleRules
{
	public TryllMCP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			// UE 5.8 Experimental. This module lives in its own plugin so that the
			// dependency lands only on projects that opt into MCP authoring.
			"ToolsetRegistry",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Json",
			"JsonUtilities",
			"UnrealEd",
			"AssetRegistry",
			"TryllClient",
			"TryllClientEditor",
		});
	}
}
