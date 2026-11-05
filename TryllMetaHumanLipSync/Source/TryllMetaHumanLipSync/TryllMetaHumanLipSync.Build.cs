// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

using System.IO;
using System.Text;
using UnrealBuildTool;

public class TryllMetaHumanLipSync : ModuleRules
{
	public TryllMetaHumanLipSync(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"LiveLinkInterface",
			"TryllClient",
			"MetaHumanCoreTech"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"LiveLink",
			"LiveLinkAnimationCore",
			"NNE"
		});

		if (Target.Version.MajorVersion > 5 || (Target.Version.MajorVersion == 5 && Target.Version.MinorVersion >= 8))
		{
			// UE 5.8 moved EAudioDrivenAnimationMood from MetaHumanCoreTech into this module.
			PublicDependencyModuleNames.Add("SpeechAnimationSolver");
		}

		if (Target.Platform == UnrealTargetPlatform.Win64 && Target.Type != TargetType.Editor && Target.Type != TargetType.Server)
		{
			CheckProjectModelIsSetUp(Target);
		}
	}

	private void CheckProjectModelIsSetUp(ReadOnlyTargetRules Target)
	{
		if (Target.ProjectFile == null)
			return;

		const string AssetName = "solar-1-2700_blend-0v65.uasset";
		string AssetPath = Path.Combine(
			Target.ProjectFile.Directory.FullName, "Content", "TryllMetaHumanLipSync", AssetName);

		// Invalidates the cached makefile when the asset changes or disappears.
		ExternalDependencies.Add(AssetPath);

		string Problem = null;
		if (!File.Exists(AssetPath))
		{
			Problem = "the project model asset (Content/TryllMetaHumanLipSync/" + AssetName + ") is missing";
		}
		else if (!FileContainsAscii(AssetPath, "NNERuntimeORTCpu"))
		{
			Problem = "the project model asset (Content/TryllMetaHumanLipSync/" + AssetName + ") has no CPU runtime target";
		}

		if (Problem != null)
		{
			throw new BuildException(
				"Tryll MetaHuman Lip Sync is not set up for this project: " + Problem + ", " +
				"so the packaged game's face will not animate. " +
				"Run Set Up in Project Settings > Plugins > Tryll MetaHuman Lip Sync, " +
				"then build again.");
		}
	}

	private static bool FileContainsAscii(string FilePath, string Needle)
	{
		byte[] Pattern = Encoding.ASCII.GetBytes(Needle);
		byte[] Data = File.ReadAllBytes(FilePath);
		for (int i = 0; i <= Data.Length - Pattern.Length; ++i)
		{
			int j = 0;
			while (j < Pattern.Length && Data[i + j] == Pattern[j])
				++j;
			if (j == Pattern.Length)
				return true;
		}
		return false;
	}
}
