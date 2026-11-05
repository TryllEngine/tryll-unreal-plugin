// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

using UnrealBuildTool;

public class TryllMetaHumanLipSyncEditor : ModuleRules
{
	public TryllMetaHumanLipSyncEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"NNE",
			"PropertyEditor",
			"Slate",
			"SlateCore",
			"UnrealEd",
			"TryllMetaHumanLipSync"
		});
	}
}
