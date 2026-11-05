// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

using System.IO;
using UnrealBuildTool;

public class TryllClientEditor : ModuleRules
{
	public TryllClientEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"TryllClient",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"InputCore",
			"Json", // Dialog Lab: parses per-turn debug_info diagnostics.
			"Slate",
			"SlateCore",
			"EditorStyle",
			"UnrealEd",
			"EditorSubsystem", // UTryllEditorSubsystem : UEditorSubsystem
			"WorkspaceMenuStructure",
			"PropertyEditor",
			"ToolMenus",
			"Projects",
			"Sockets",
			"Networking",
			"AudioCaptureCore",
			"DesktopPlatform",
			// Visual workflow graph editor.
			"GraphEditor",   // SGraphEditor, SGraphNode, FGraphPanelNodeFactory, connection drawing
			"AssetTools",    // IAssetTypeActions registration for UTryllWorkflowAsset
			"KismetWidgets", // SLevelOfDetailBranchNode and shared graph node widgets
			"ApplicationCore",
			"HTTP",      // Welcome: the tryll.server.install event
			"MainFrame", // Welcome: open the tab once the main frame exists
		});

		PrivateIncludePaths.Add(
			Path.Combine(ModuleDirectory, "..", "TryllClient", "Private"));

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// SHA-256 for the logs-folder project key (TryllLogsFolder.cpp).
			PublicSystemLibraries.Add("bcrypt.lib");
		}

		// The marker exists only in the monorepo; ci/release.toml drops it together
		// with Private/Developer/, so a shipped plugin compiles no developer menu.
		string DeveloperMarker = Path.Combine(PluginDirectory, "TryllDeveloperTools.marker");
		bool bDeveloperTools = File.Exists(DeveloperMarker);
		if (bDeveloperTools)
		{
			ExternalDependencies.Add(DeveloperMarker);
		}
		PrivateDefinitions.Add("TRYLL_DEVELOPER_TOOLS=" + (bDeveloperTools ? "1" : "0"));
	}
}
