// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

using UnrealBuildTool;
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using Microsoft.Extensions.Logging;
using Microsoft.Win32.SafeHandles;
using EpicGames.Core;

public class TryllClient : ModuleRules
{
	public TryllClient(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		// UE 5.x modules require C++20; the plugin's own code stays C++17-style
		// (no coroutines, no std::format, etc.).
		CppStandard = CppStandardVersion.Cpp20;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"DeveloperSettings",
			"Engine",
			"Projects",
			"Sockets",
			"Networking"
		});

		// AudioCaptureCore — microphone capture for UTryllVoiceInputComponent (STT).
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AudioCaptureCore",
			// Text Table picker lists string-table assets that are not loaded yet.
			"AssetRegistry"
		});

		// Editor targets: UnrealEd for GEditor when routing PIE through the bridge.
		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd",
				"EditorSubsystem",
			});
		}

		string ModuleDir   = ResolveLinks(ModuleDirectory);
		string PluginRoot  = Path.GetFullPath(Path.Combine(ModuleDir, "..", ".."));
		// Phase C1 moved the plugin from client-unreal/ to clients/unreal/, adding one
		// directory level, so ServerRoot (= tryll/) now needs two steps up from PluginRoot.
		string ServerRoot  = Path.GetFullPath(Path.Combine(PluginRoot, "..", ".."));

		// messages_generated.h — produced by the server CMake build, gitignored.
		string GeneratedDir       = Path.Combine(PluginRoot, "Generated");
		// Protocol.h, FbbHelpers.h, ErrorCodes.h, … — header-only shared common/ headers.
		// Prefer the plugin-local bundled copy (ThirdParty/TryllCommon/include, populated at
		// build time by TryllFlatBuffers.cmake and gitignored) so the shipped plugin compiles
		// standalone. Fall back to the monorepo common/include for an in-tree build that has
		// not run codegen yet. Both layouts compile.
		string BundledCommonInclude  = Path.GetFullPath(Path.Combine(ModuleDir, "..", "ThirdParty", "TryllCommon", "include"));
		string MonorepoCommonInclude = Path.Combine(ServerRoot, "common", "include");
		string CommonIncludeDir = Directory.Exists(Path.Combine(BundledCommonInclude, "tryll"))
			? BundledCommonInclude
			: MonorepoCommonInclude;
		// FlatBuffers header-only runtime — committed in ThirdParty.
		string FlatbuffersInclude = Path.Combine(ModuleDir, "..", "ThirdParty", "flatbuffers", "include");

		// The module's own subdirectories use the UNRESOLVED ModuleDirectory -- the same
		// spelling UBT uses for the default Public/ path and for the .cpp files. With the
		// resolved path, "TryllNodeParamsFactory.h" (via Public/Generated) and
		// "Generated/TryllNodeParamsFactory.h" (via Public) reach one header by two paths,
		// #pragma once treats them as two files, and a unity blob holding both includes
		// fails with "generated.h already included" (td-042).
		PublicIncludePaths.Add(GeneratedDir);
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public", "Generated"));
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public", "Generated", "Nodes"));
		PublicIncludePaths.Add(CommonIncludeDir);
		PublicIncludePaths.Add(FlatbuffersInclude);

		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));
		// Generated protocol codec (ClientCodec.gen.h / .cpp) emitted by tryll_codegen.py.
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private", "Generated"));

		if (Target.Platform == UnrealTargetPlatform.Win64 && Target.Type != TargetType.Editor)
		{
			StageBundledServer(Target);
		}
	}

	// Relative (to the project's storage root) path of the models directory.
	// Client-dev default: <StorageRoot>/models (config key models_root).
	private static readonly string ModelsDirName = "models";
	// Tryll Dev / legacy server caches that must never ship inside Server/.
	private static readonly string AppDataRel   = Path.Combine("data", ".app-data");
	private static readonly string DevModelsRel = Path.Combine("data", ".models");

	// llama-cuda profile in tryll/cmake/TryllDependencyVersions.json plus ggml-cuda.dll.
	// Also skip leftover ORT CUDA extras if a stale payload still has them.
	private static readonly HashSet<string> LlamaCudaDlls = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
	{
		"ggml-cuda.dll",
		"cudart64_12.dll",
		"cublas64_12.dll",
		"cublasLt64_12.dll",
	};
	private static readonly HashSet<string> OrtCudaExtraDlls = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
	{
		"onnxruntime_providers_cuda.dll",
		"cufft64_11.dll",
		"cufftw64_11.dll",
		"cudnn64_9.dll",
		"cudnn_ops64_9.dll",
		"cudnn_cnn64_9.dll",
		"cudnn_adv64_9.dll",
		"cudnn_graph64_9.dll",
		"cudnn_heuristic64_9.dll",
		"cudnn_engines_precompiled64_9.dll",
		"cudnn_engines_runtime_compiled64_9.dll",
	};

	/// <summary>
	/// Stages the bundled server next to the packaged game as <c>$(ProjectDir)/Server/</c>,
	/// and stages the project's TryllStorage folder (NonUFS) with Production models only.
	///
	/// Variant selection mirrors <c>UTryllSubsystem::ResolveServerExePath</c> (editor): a variant
	/// counts as available only when <c>tryll_server.exe</c> exists; a single available variant
	/// is always used; when both are present the project's <c>ServerBuildVariant</c> setting wins.
	///
	/// Models marked Production in the project's TryllModelManifest are staged into
	/// <c>TryllStorage/models/</c> with a filtered <c>downloads.json</c>, so the
	/// packaged game loads them offline. Missing Production models fail the build.
	/// A StorageDataFolder under Content/ fails the build (the server cannot read .pak).
	/// </summary>
	private void StageBundledServer(ReadOnlyTargetRules Target)
	{
		TryllProjectSettings Settings = LoadProjectSettings(Target);
		string ServerRoot = Path.Combine(PluginDirectory, "Binaries", "ThirdParty", "TryllServer");
		string ServerSource = ResolveServerVariantDir(ServerRoot, Settings.ServerBuildVariant);
		bool IncludeCuda = Settings.LlamaCppBackend == "Auto";
		if (!string.IsNullOrEmpty(ServerSource) && IncludeCuda
			&& !File.Exists(Path.Combine(ServerSource, "ggml-cuda.dll")))
		{
			throw new BuildException(
				"Tryll: Llama.cpp backend is Auto but ggml-cuda.dll is missing from the " +
				"bundled server payload. Build the server with TRYLL_CUDA=PREBUILT, or set " +
				"Project Settings → Tryll Client → Llama Cpp Backend to Vulkan.");
		}
		if (string.IsNullOrEmpty(ServerSource))
		{
			string Msg =
				"Tryll: no bundled server variant with tryll_server.exe found under " +
				"'Binaries/ThirdParty/TryllServer/{Default,Release}/'. The packaged build will " +
				"have no server to launch. Copy the server build there, or set AutoLaunchServer = " +
				"false and manage the server yourself.";
			if (Settings.bAutoLaunchServer)
				throw new BuildException(Msg);
			Logger.LogWarning("{Message}", Msg);
			return;
		}

		if (!TryResolveClientProd(ServerSource, out string ProdExe, out string ProdManifest, out string ProdError))
		{
			if (Settings.bAutoLaunchServer)
				throw new BuildException(ProdError);
			Logger.LogWarning("{Message}", ProdError);
			return;
		}

		if (Target.ProjectFile != null && IsStorageUnderContent(Target, Settings.StorageDataFolder))
		{
			throw new BuildException(
				"Tryll: StorageDataFolder ('" + Settings.StorageDataFolder + "') resolves under " +
				"Content/. The Tryll server cannot read Unreal .pak archives. Move it outside " +
				"Content/ (default: TryllStorage next to the .uproject) via Project Settings → " +
				"Plugins → Tryll Client → Storage Data Folder, then build again.");
		}

		// 1. Stage the server payload; skip data/.app-data/ and data/.models/ (models live under TryllStorage).
		// EnumerateFiles returns children under the path as passed (junctions are transparent),
		// so derive the relative path from the unresolved prefix.
		string SourceFull = Path.GetFullPath(ServerSource).TrimEnd(
			Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar) + Path.DirectorySeparatorChar;
		foreach (string File_ in Directory.EnumerateFiles(ServerSource, "*", SearchOption.AllDirectories))
		{
			string Rel  = Path.GetFullPath(File_).Substring(SourceFull.Length);
			string Name = Path.GetFileName(File_);
			if (Name == ".gitignore" || Name == "README.md")
				continue;
			if (IsFlavorExecutableOrManifest(Name))
				continue;
			if (!IncludeCuda && (LlamaCudaDlls.Contains(Name) || OrtCudaExtraDlls.Contains(Name)))
				continue;
			if (IsUnderRelative(Rel, AppDataRel) || IsUnderRelative(Rel, DevModelsRel))
				continue; // models are staged from the project storage root

			// Stage server-config.json from a production-logging copy (see StageProductionServerConfig).
			string Source = File_;
			if (Rel.Equals(Path.Combine("data", "server-config.json"), StringComparison.OrdinalIgnoreCase))
				Source = StageProductionServerConfig(File_);

			RuntimeDependencies.Add(
				"$(ProjectDir)/Server/" + Rel.Replace('\\', '/'),
				Source,
				StagedFileType.NonUFS);
		}

		RuntimeDependencies.Add("$(ProjectDir)/Server/tryll_server.exe", ProdExe, StagedFileType.NonUFS);
		RuntimeDependencies.Add(
			"$(ProjectDir)/Server/tryll_server.build.json", ProdManifest, StagedFileType.NonUFS);

		StageUserModelsCatalog(Target);

		string StorageRoot = ResolveStorageRootAbsolute(Target, Settings.StorageDataFolder);
		BakeDerivedFiles(Target, Settings, ServerSource, StorageRoot);
		StageStorageDirectory(Target, ServerSource, StorageRoot);
		StageManifestModels(Target, ServerSource, StorageRoot, Settings.ProductionModelNames);
	}

	/// <summary>
	/// Copies the project's optional custom models catalog into
	/// Server/data/user-catalog/ and rewrites relative chat_template_file values
	/// so those files sit beside the copy. The reserved folder cannot overwrite
	/// the shipped models.json. No-op when the setting is empty, the path leaves
	/// the project, or the file is missing.
	/// </summary>
	private void StageUserModelsCatalog(ReadOnlyTargetRules Target)
	{
		string Full = ResolveUserModelsCatalogFile(Target);
		if (string.IsNullOrEmpty(Full) || Target.ProjectFile == null)
			return;

		string GeneratedDir = Path.Combine(PluginDirectory, "Intermediate", "TryllUserCatalog");
		if (Directory.Exists(GeneratedDir))
			Directory.Delete(GeneratedDir, true);
		Directory.CreateDirectory(GeneratedDir);

		string ProjectRoot = Path.GetFullPath(Target.ProjectFile.Directory.FullName);
		string Json = RewriteCatalogChatTemplates(Full, GeneratedDir, ProjectRoot);
		string FileName = Path.GetFileName(Full);
		string OutJson = Path.Combine(GeneratedDir, FileName);
		File.WriteAllText(OutJson, Json, new System.Text.UTF8Encoding(false));

		const string DestRoot = "$(ProjectDir)/Server/data/user-catalog/";
		RuntimeDependencies.Add(DestRoot + FileName, OutJson, StagedFileType.NonUFS);

		string SidecarDir = Path.Combine(GeneratedDir, "sidecars");
		if (Directory.Exists(SidecarDir))
		{
			foreach (string Side in Directory.GetFiles(SidecarDir))
			{
				RuntimeDependencies.Add(
					DestRoot + "sidecars/" + Path.GetFileName(Side),
					Side,
					StagedFileType.NonUFS);
			}
		}
		Logger.LogInformation("Tryll: staged custom models catalog at Server/data/user-catalog/{File}", FileName);
	}

	/// <summary>
	/// Copy relative chat templates next to the generated catalog and point that
	/// JSON at the copies. Absolute template paths are left as-is. The project's
	/// catalog file is not modified.
	/// </summary>
	private string RewriteCatalogChatTemplates(string CatalogPath, string GeneratedDir, string ProjectRoot)
	{
		string Json = File.ReadAllText(CatalogPath);
		string CatalogDir = Path.GetDirectoryName(CatalogPath) ?? string.Empty;
		string RootPrefix = Path.GetFullPath(ProjectRoot)
			.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar)
			+ Path.DirectorySeparatorChar;
		int N = 0;
		return Regex.Replace(
			Json,
			"\"chat_template_file\"\\s*:\\s*\"([^\"]*)\"",
			Match =>
			{
				string Value = Match.Groups[1].Value;
				if (string.IsNullOrWhiteSpace(Value) || Path.IsPathRooted(Value))
					return Match.Value;

				string Src = Path.GetFullPath(Path.Combine(
					CatalogDir, Value.Replace('/', Path.DirectorySeparatorChar)));
				if (!Src.StartsWith(RootPrefix, StringComparison.OrdinalIgnoreCase) || !File.Exists(Src))
				{
					Logger.LogWarning(
						"Tryll: chat_template_file '{Path}' in '{Catalog}' is missing or outside the project; left unchanged.",
						Value, CatalogPath);
					return Match.Value;
				}

				string DestRel = "sidecars/" + N + "_" + Path.GetFileName(Src);
				++N;
				string DestFile = Path.Combine(GeneratedDir, DestRel.Replace('/', Path.DirectorySeparatorChar));
				string DestParent = Path.GetDirectoryName(DestFile);
				if (!string.IsNullOrEmpty(DestParent))
					Directory.CreateDirectory(DestParent);
				File.Copy(Src, DestFile, true);
				return "\"chat_template_file\": \"" + DestRel + "\"";
			},
			RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);
	}

	/// <summary>
	/// Absolute custom-catalog file from Game ini, or null when unset, outside
	/// the project, or missing on disk.
	/// </summary>
	private static string ResolveUserModelsCatalogFile(ReadOnlyTargetRules Target)
	{
		if (Target.ProjectFile == null)
			return null;

		ConfigHierarchy Config = ConfigCache.ReadHierarchy(
			ConfigHierarchyType.Game, Target.ProjectFile.Directory, Target.Platform);
		string Rel;
		if (!Config.GetString("/Script/TryllClient.TryllRuntimeSettings", "UserModelsCatalogPath", out Rel)
			|| string.IsNullOrWhiteSpace(Rel))
			return null;

		string Root = Path.GetFullPath(Target.ProjectFile.Directory.FullName)
			.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar)
			+ Path.DirectorySeparatorChar;
		string Full = Path.IsPathRooted(Rel.Trim())
			? Path.GetFullPath(Rel.Trim())
			: Path.GetFullPath(Path.Combine(Target.ProjectFile.Directory.FullName, Rel.Trim()));
		if (!Full.StartsWith(Root, StringComparison.OrdinalIgnoreCase) || !File.Exists(Full))
			return null;
		return Full;
	}

	private static bool IsFlavorExecutableOrManifest(string Name)
	{
		return Name.Equals("tryll_server.exe", StringComparison.OrdinalIgnoreCase)
			|| Name.Equals("tryll_server_client_prod.exe", StringComparison.OrdinalIgnoreCase)
			|| Name.Equals("tryll-build.json", StringComparison.OrdinalIgnoreCase)
			|| Name.Equals("tryll_server.build.json", StringComparison.OrdinalIgnoreCase)
			|| Name.Equals("tryll_server_client_prod.build.json", StringComparison.OrdinalIgnoreCase);
	}

	private static bool TryResolveClientProd(
		string Source, out string Exe, out string Manifest, out string Error)
	{
		Exe = null;
		Manifest = null;
		string RenamedExe = Path.Combine(Source, "tryll_server_client_prod.exe");
		string RenamedManifest = Path.Combine(Source, "tryll_server_client_prod.build.json");
		string DirectExe = Path.Combine(Source, "tryll_server.exe");
		string DirectManifest = Path.Combine(Source, "tryll-build.json");
		if (File.Exists(RenamedExe))
		{
			Exe = RenamedExe;
			Manifest = RenamedManifest;
		}
		else
		{
			Exe = DirectExe;
			Manifest = DirectManifest;
		}

		if (!File.Exists(Exe) || !File.Exists(Manifest))
		{
			Error = "Tryll: player builds require a ClientProd server "
				+ "(tryll_server_client_prod.exe plus tryll_server_client_prod.build.json, "
				+ "or tryll_server.exe whose tryll-build.json says ClientProd).";
			return false;
		}

		string Json = File.ReadAllText(Manifest);
		Match Config = Regex.Match(Json, "\"build_config\"\\s*:\\s*\"([^\"]*)\"");
		if (!Config.Success || !string.Equals(Config.Groups[1].Value, "ClientProd", StringComparison.Ordinal))
		{
			Error = "Tryll: refusing to stage '" + Exe + "': its build manifest is not ClientProd.";
			return false;
		}

		Error = null;
		return true;
	}

	private static string DisableTelemetry(string Cfg)
	{
		if (string.IsNullOrEmpty(Cfg))
			return Cfg;
		int Key = Cfg.IndexOf("\"telemetry\"", StringComparison.Ordinal);
		if (Key < 0)
			return Cfg;
		int Brace = Cfg.IndexOf('{', Key);
		if (Brace < 0)
			return Cfg;
		int Depth = 0;
		int End = Brace;
		for (; End < Cfg.Length; ++End)
		{
			if (Cfg[End] == '{')
				Depth++;
			else if (Cfg[End] == '}')
			{
				Depth--;
				if (Depth == 0)
				{
					End++;
					break;
				}
			}
		}
		return Cfg.Substring(0, Key)
			+ "\"telemetry\": { \"enabled\": false, \"level\": \"off\" }"
			+ Cfg.Substring(End);
	}

	/// <summary>
	/// Writes a production-logging copy of server-config.json (log_mode=production, log_level=info)
	/// to the plugin's Intermediate dir and returns its path. Telemetry is forced off.
	/// </summary>
	private string StageProductionServerConfig(string SourceConfig)
	{
		string Cfg = File.ReadAllText(SourceConfig);
		Cfg = Regex.Replace(Cfg, "\"log_mode\"\\s*:\\s*\"[^\"]*\"",  "\"log_mode\": \"production\"");
		Cfg = Regex.Replace(Cfg, "\"log_level\"\\s*:\\s*\"[^\"]*\"", "\"log_level\": \"info\"");
		Cfg = DisableTelemetry(Cfg);

		string GeneratedDir = Path.Combine(PluginDirectory, "Intermediate", "TryllStagedModels");
		Directory.CreateDirectory(GeneratedDir);
		string Out = Path.Combine(GeneratedDir, "server-config.json");
		File.WriteAllText(Out, Cfg);
		return Out;
	}

	private void StageManifestModels(
		ReadOnlyTargetRules Target,
		string ServerSource,
		string StorageRoot,
		HashSet<string> ProductionNames)
	{
		if (Target.ProjectFile == null)
			return; // No consuming project → no manifest.

		HashSet<string> Pending = new HashSet<string>(ProductionNames);
		if (Pending.Count == 0)
			return; // Pure auto-download game, or nothing registered.

		string SrcModelsDir = Path.Combine(StorageRoot, ModelsDirName);
		var Staged = new List<StagedModel>();
		var StagedAssets = new List<StagedAsset>();

		// 2a. Downloaded models — match the source downloads.json by name.
		string DownloadsJson = Path.Combine(SrcModelsDir, "downloads.json");
		if (File.Exists(DownloadsJson))
		{
			JsonObject Root = JsonObject.Read(new FileReference(DownloadsJson));
			JsonObject[] Entries;
			if (!Root.TryGetObjectArrayField("downloads", out Entries))
				Entries = new JsonObject[0];

			foreach (JsonObject E in Entries)
			{
				string Name;
				if (!E.TryGetStringField("name", out Name) || !Pending.Contains(Name))
					continue;

				string Folder, EngineName;
				string[] Files;
				E.TryGetStringField("folder", out Folder);
				E.TryGetStringField("engine", out EngineName);
				if (!E.TryGetStringArrayField("files", out Files))
					Files = new string[0];

				string SrcFolder = Path.Combine(SrcModelsDir, Folder ?? "");
				if (string.IsNullOrEmpty(Folder) || !Directory.Exists(SrcFolder))
					continue; // files absent on disk; leave pending for the missing-model check

				long Total = 0;
				foreach (string F in Files)
				{
					string SrcFile = Path.Combine(SrcFolder, F);
					if (File.Exists(SrcFile))
						Total += new FileInfo(SrcFile).Length;
					StageNonUfs(Target, "models/" + Folder + "/" + F, SrcFile);
				}
				StageBakedVocab(Target, SrcFolder, Folder, Files);
				Staged.Add(new StagedModel { Name = Name, Engine = EngineName, Folder = Folder, Files = Files, TotalBytes = Total });
				Pending.Remove(Name);
			}
		}

		// 2b. Local models (models.json, then the optional project catalog).
		if (Pending.Count > 0)
		{
			var Catalogs = new List<string> { Path.Combine(ServerSource, "data", "models.json") };
			string UserCatalog = ResolveUserModelsCatalogFile(Target);
			if (!string.IsNullOrEmpty(UserCatalog))
				Catalogs.Add(UserCatalog);
			foreach (string ModelsCatalog in Catalogs)
			{
			if (File.Exists(ModelsCatalog))
			{
				JsonObject Root = JsonObject.Read(new FileReference(ModelsCatalog));
				JsonObject[] Models;
				if (!Root.TryGetObjectArrayField("models", out Models))
					Models = new JsonObject[0];

				foreach (JsonObject M in Models)
				{
					string Name;
					if (!M.TryGetStringField("name", out Name) || !Pending.Contains(Name))
						continue;
					JsonObject[] Variants;
					if (!M.TryGetObjectArrayField("variants", out Variants))
						continue;

					foreach (JsonObject V in Variants)
					{
						string LocalPath, EngineName, Repo;
						string[] Files;
						if (!V.TryGetStringField("local_path", out LocalPath) || string.IsNullOrEmpty(LocalPath))
							continue;
						string ResolvedLocal = Path.IsPathRooted(LocalPath)
							? Path.GetFullPath(LocalPath)
							: Path.GetFullPath(Path.Combine(StorageRoot, LocalPath));
						if (!Directory.Exists(ResolvedLocal))
							continue;
						if (!V.TryGetStringArrayField("files", out Files) || Files.Length == 0)
							continue;
						V.TryGetStringField("engine", out EngineName);
						V.TryGetStringField("huggingface_repo", out Repo);

						string Folder = !string.IsNullOrEmpty(Repo) ? RepoToFolder(Repo) : SanitizeFolder(Name);
						long Total = 0;
						bool Ok = true;
						foreach (string F in Files)
						{
							string SrcFile = Path.Combine(ResolvedLocal, F);
							if (!File.Exists(SrcFile)) { Ok = false; break; }
							Total += new FileInfo(SrcFile).Length;
							StageNonUfs(Target, "models/" + Folder + "/" + F, SrcFile);
						}
						if (!Ok)
							continue;
						StageBakedVocab(Target, ResolvedLocal, Folder, Files);

						Staged.Add(new StagedModel { Name = Name, Engine = EngineName, Folder = Folder, Files = Files, TotalBytes = Total });
						Pending.Remove(Name);
						break;
					}
				}
			}
			}
		}

		if (Pending.Count > 0)
		{
			throw new BuildException(
				"Tryll: these models are marked Production in the Tryll Model Manifest " +
				"(Config/DefaultGame.ini) but are not downloaded/available on this machine, so they " +
				"cannot be staged into the build:\n  - " + string.Join("\n  - ", Pending) +
				"\nDownload them in the Tryll Model Manager (or fix their local_path in models.json), " +
				"or unmark them as Production, then build again. Models are expected under " +
				SrcModelsDir + ".");
		}

		StageReachableAssets(Target, ServerSource, SrcModelsDir, ProductionNames, StagedAssets);

		// 3. Write the filtered downloads.json and stage it.
		string GeneratedDir  = Path.Combine(PluginDirectory, "Intermediate", "TryllStagedModels");
		Directory.CreateDirectory(GeneratedDir);
		string GeneratedJson = Path.Combine(GeneratedDir, "downloads.json");
		File.WriteAllText(GeneratedJson, BuildDownloadsJson(Staged, StagedAssets));
		StageNonUfs(Target, "models/downloads.json", GeneratedJson);
	}

	/// <summary>
	/// Stages non-model files from the project's storage root as NonUFS under
	/// <c>$(ProjectDir)/TryllStorage/</c>. Models are handled separately by
	/// <see cref="StageManifestModels"/> so only Production weights ship.
	/// </summary>
	private void StageStorageDirectory(ReadOnlyTargetRules Target, string ServerSource, string StorageRoot)
	{
		if (string.IsNullOrEmpty(StorageRoot) || !Directory.Exists(StorageRoot))
			return;

		string RootFull = Path.GetFullPath(StorageRoot).TrimEnd(
			Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar)
			+ Path.DirectorySeparatorChar;
		string ModelsPrefix = ModelsDirName + Path.DirectorySeparatorChar;
		HashSet<string> LocalPathPrefixes = LocalPathPrefixesInsideStorage(Target, ServerSource, StorageRoot);

		foreach (string File_ in Directory.EnumerateFiles(StorageRoot, "*", SearchOption.AllDirectories))
		{
			string Rel = Path.GetFullPath(File_).Substring(RootFull.Length);
			if (Rel.StartsWith(ModelsPrefix, StringComparison.OrdinalIgnoreCase)
				|| Rel.Equals(ModelsDirName, StringComparison.OrdinalIgnoreCase))
				continue; // Production models staged by StageManifestModels
			if (Rel.EndsWith(".part", StringComparison.OrdinalIgnoreCase)
				|| Rel.EndsWith(".tmp", StringComparison.OrdinalIgnoreCase))
				continue;
			string RelSlash = Rel.Replace('\\', '/');
			bool UnderLocalPath = false;
			foreach (string Prefix in LocalPathPrefixes)
			{
				if (RelSlash.StartsWith(Prefix, StringComparison.OrdinalIgnoreCase))
				{
					UnderLocalPath = true;
					break;
				}
			}
			if (UnderLocalPath)
				continue; // catalog local_path folders; Production ones stage via the manifest

			StageNonUfs(Target, Rel, File_);
		}
	}

	/// <summary>
	/// Storage-relative prefixes (with a trailing slash) for every catalog
	/// <c>local_path</c> inside the storage root. Relative values resolve against
	/// that root, matching the server.
	/// </summary>
	private HashSet<string> LocalPathPrefixesInsideStorage(
		ReadOnlyTargetRules Target, string ServerSource, string StorageRoot)
	{
		var Prefixes = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
		if (string.IsNullOrEmpty(StorageRoot) || string.IsNullOrEmpty(ServerSource))
			return Prefixes;

		string RootFull = Path.GetFullPath(StorageRoot).TrimEnd(
			Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar)
			+ Path.DirectorySeparatorChar;
		var Catalogs = new List<string> { Path.Combine(ServerSource, "data", "models.json") };
		string UserCatalog = ResolveUserModelsCatalogFile(Target);
		if (!string.IsNullOrEmpty(UserCatalog))
			Catalogs.Add(UserCatalog);

		foreach (string ModelsCatalog in Catalogs)
		{
			if (!File.Exists(ModelsCatalog))
				continue;
			JsonObject Root = JsonObject.Read(new FileReference(ModelsCatalog));
			JsonObject[] Models;
			if (!Root.TryGetObjectArrayField("models", out Models))
				continue;
			foreach (JsonObject M in Models)
			{
				JsonObject[] Variants;
				if (!M.TryGetObjectArrayField("variants", out Variants))
					continue;
				foreach (JsonObject V in Variants)
				{
					string LocalPath;
					if (!V.TryGetStringField("local_path", out LocalPath) || string.IsNullOrEmpty(LocalPath))
						continue;
					string Resolved = Path.IsPathRooted(LocalPath)
						? Path.GetFullPath(LocalPath)
						: Path.GetFullPath(Path.Combine(StorageRoot, LocalPath));
					if (!Resolved.StartsWith(RootFull, StringComparison.OrdinalIgnoreCase))
						continue;
					string Rel = Resolved.Substring(RootFull.Length).Replace('\\', '/').TrimEnd('/');
					if (!string.IsNullOrEmpty(Rel))
						Prefixes.Add(Rel + "/");
				}
			}
		}
		return Prefixes;
	}

	/// <summary>
	/// Stages the <c>bpe.vocab</c> and <c>bpe.vocab.json</c> that <c>tryll_server --bake</c>
	/// wrote beside a speech model's tokens.txt. They are not in the model's file list,
	/// and Client Prod cannot derive them.
	/// </summary>
	private void StageBakedVocab(ReadOnlyTargetRules Target, string SrcFolder, string Folder, string[] Files)
	{
		var Seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
		foreach (string F in Files)
		{
			string Dir = Path.GetDirectoryName(F) ?? "";
			foreach (string Baked in new[] { "bpe.vocab", "bpe.vocab.json" })
			{
				string Rel = Path.Combine(Dir, Baked);
				string Src = Path.Combine(SrcFolder, Rel);
				if (Seen.Add(Rel) && File.Exists(Src))
					StageNonUfs(Target, "models/" + Folder + "/" + Rel.Replace('\\', '/'), Src);
			}
		}
	}

	/// <summary>
	/// Runs <c>tryll_server --bake</c> (the editor's Client Dev server) on the project storage,
	/// so every knowledge base's <c>.usearch</c>, <c>.bm25</c> and <c>.bake.json</c>, and every
	/// speech model's <c>bpe.vocab</c>, exist before staging enumerates the files. A packaged
	/// game (Client Prod) only loads them, so a failed bake fails the build. Skipped while
	/// generating project files.
	/// </summary>
	private void BakeDerivedFiles(
		ReadOnlyTargetRules Target, TryllProjectSettings Settings, string ServerSource, string StorageRoot)
	{
		// ProjectFileGenerator is internal to UBT, so read its switch off the command line.
		bool GeneratingProjectFiles = Array.Exists(Environment.GetCommandLineArgs(), Arg =>
			Arg.Equals("-ProjectFiles", StringComparison.OrdinalIgnoreCase)
			|| Arg.Equals("-GenerateProjectFiles", StringComparison.OrdinalIgnoreCase));
		if (GeneratingProjectFiles || Target.ProjectFile == null)
			return;

		string ProjectDir = Target.ProjectFile.Directory.FullName;
		ConfigHierarchy Config = ConfigCache.ReadHierarchy(
			ConfigHierarchyType.Game, Target.ProjectFile.Directory, Target.Platform);
		string ProjectName;
		if (!Config.GetString("/Script/TryllClient.TryllRuntimeSettings", "GameName", out ProjectName)
			|| string.IsNullOrWhiteSpace(ProjectName))
			ProjectName = Target.ProjectFile.GetFileNameWithoutExtension();
		// Client Dev requires a project id; it only keys telemetry, which the bake turns off.
		string ProjectId;
		if (!Config.GetString("/Script/TryllClient.TryllRuntimeSettings", "ProjectId", out ProjectId)
			|| !Regex.IsMatch(ProjectId ?? "", "^[0-9a-fA-F]{32}$"))
			ProjectId = Guid.NewGuid().ToString("N");

		var Args = new StringBuilder("--bake --port 0 --mode client-dev");
		Args.Append(" --llama-backend ").Append(Settings.LlamaCppBackend == "Auto" ? "auto" : "vulkan");
		Args.Append(" --project-name \"").Append(ProjectName.Trim()).Append('"');
		Args.Append(" --project-dir \"").Append(ProjectDir).Append('"');
		Args.Append(" --project-id ").Append(ProjectId.ToLowerInvariant());
		Args.Append(" --storage-root \"").Append(StorageRoot).Append('"');
		string UserCatalog = ResolveUserModelsCatalogFile(Target);
		if (!string.IsNullOrEmpty(UserCatalog))
			Args.Append(" --user-models-path \"").Append(UserCatalog).Append('"');
		string UserModelsDir = Path.Combine(ProjectDir, "Tryll", "models.d");
		if (Directory.Exists(UserModelsDir))
			Args.Append(" --user-models-dir \"").Append(UserModelsDir).Append('"');

		string Exe = Path.Combine(ServerSource, "tryll_server.exe");
		var Output = new StringBuilder();
		var Psi = new System.Diagnostics.ProcessStartInfo
		{
			FileName               = Exe,
			Arguments              = Args.ToString(),
			WorkingDirectory       = ServerSource,
			UseShellExecute        = false,
			CreateNoWindow         = true,
			RedirectStandardOutput = true,
			RedirectStandardError  = true,
		};
		int ExitCode;
		using (var Proc = new System.Diagnostics.Process { StartInfo = Psi })
		{
			Proc.OutputDataReceived += (Sender, E) => { if (E.Data != null) lock (Output) Output.AppendLine(E.Data); };
			Proc.ErrorDataReceived  += (Sender, E) => { if (E.Data != null) lock (Output) Output.AppendLine(E.Data); };
			Proc.Start();
			Proc.BeginOutputReadLine();
			Proc.BeginErrorReadLine();
			Proc.WaitForExit();
			ExitCode = Proc.ExitCode;
		}
		if (ExitCode != 0)
		{
			throw new BuildException(
				"Tryll: baking knowledge base indexes and speech vocabularies failed (exit code " +
				ExitCode + "). A packaged game cannot build them, so the build stops.\n" + Output);
		}
		Logger.LogInformation("Tryll: baked derived files for the build:\n{Output}", Output.ToString());
	}

	/// A file that already lives at the staged path cannot be both the source and
	/// the target of a copy (UBT rejects that). Declare it in place instead.
	private void StageNonUfs(ReadOnlyTargetRules Target, string RelativeUnderStorage, string SourceFile)
	{
		string Rel = RelativeUnderStorage.Replace('\\', '/');
		string Dest = "$(ProjectDir)/TryllStorage/" + Rel;
		string DestFull = Path.GetFullPath(Path.Combine(
			Target.ProjectFile.Directory.FullName, "TryllStorage", RelativeUnderStorage));
		string SrcFull = Path.GetFullPath(SourceFile);
		if (string.Equals(DestFull, SrcFull, StringComparison.OrdinalIgnoreCase))
			RuntimeDependencies.Add(Dest, StagedFileType.NonUFS);
		else
			RuntimeDependencies.Add(Dest, SourceFile, StagedFileType.NonUFS);
	}

	private static string ResolveStorageRootAbsolute(ReadOnlyTargetRules Target, string Configured)
	{
		if (Target.ProjectFile == null)
			return null;
		string Rel = string.IsNullOrWhiteSpace(Configured) ? "TryllStorage" : Configured.Trim();
		if (Path.IsPathRooted(Rel))
			return Path.GetFullPath(Rel);
		return Path.GetFullPath(Path.Combine(Target.ProjectFile.Directory.FullName, Rel));
	}

	/// True when a payload-relative path is Folder itself or anything under it.
	private static bool IsUnderRelative(string Rel, string Folder)
	{
		return Rel.Equals(Folder, StringComparison.OrdinalIgnoreCase)
			|| Rel.StartsWith(Folder + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase);
	}

	private static bool IsStorageUnderContent(ReadOnlyTargetRules Target, string Configured)
	{
		if (Target.ProjectFile == null)
			return false;
		string Storage = ResolveStorageRootAbsolute(Target, Configured);
		if (string.IsNullOrEmpty(Storage))
			return false;
		string Content = Path.GetFullPath(Path.Combine(Target.ProjectFile.Directory.FullName, "Content"))
			.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
		string StorageNorm = Path.GetFullPath(Storage)
			.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
		return StorageNorm.Equals(Content, StringComparison.OrdinalIgnoreCase)
			|| StorageNorm.StartsWith(Content + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase);
	}

	private struct TryllProjectSettings
	{
		public bool bAutoLaunchServer;
		public string ServerBuildVariant; // "Default" | "Release"
		public string LlamaCppBackend;    // "Vulkan" | "Auto"
		public string StorageDataFolder;  // project-relative, default TryllStorage
		public HashSet<string> ProductionModelNames;
	}

	/// <summary>
	/// Reads Tryll runtime + manifest settings via UBT's ConfigHierarchy (Game ini stack,
	/// including platform overrides). Falls back to safe defaults when no project is present.
	/// </summary>
	private static TryllProjectSettings LoadProjectSettings(ReadOnlyTargetRules Target)
	{
		var Settings = new TryllProjectSettings
		{
			bAutoLaunchServer = true,
			ServerBuildVariant = "Default",
			LlamaCppBackend = "Vulkan",
			StorageDataFolder = "TryllStorage",
			ProductionModelNames = new HashSet<string>(),
		};
		if (Target.ProjectFile == null)
			return Settings;

		ConfigHierarchy Config = ConfigCache.ReadHierarchy(
			ConfigHierarchyType.Game, Target.ProjectFile.Directory, Target.Platform);

		bool AutoLaunch;
		if (Config.GetBool("/Script/TryllClient.TryllRuntimeSettings", "bAutoLaunchServer", out AutoLaunch))
			Settings.bAutoLaunchServer = AutoLaunch;

		string Variant;
		if (Config.GetString("/Script/TryllClient.TryllRuntimeSettings", "ServerBuildVariant", out Variant)
			&& !string.IsNullOrEmpty(Variant))
		{
			// Accept bare "Release"/"Default" or enum-qualified "EServerBuildVariant::Release".
			if (Variant.EndsWith("Release", StringComparison.OrdinalIgnoreCase))
				Settings.ServerBuildVariant = "Release";
			else
				Settings.ServerBuildVariant = "Default";
		}

		string Backend;
		if (Config.GetString("/Script/TryllClient.TryllRuntimeSettings", "LlamaCppBackend", out Backend)
			&& !string.IsNullOrEmpty(Backend))
		{
			Settings.LlamaCppBackend = Backend.EndsWith("Auto", StringComparison.OrdinalIgnoreCase)
				? "Auto"
				: "Vulkan";
		}

		string Storage;
		if (Config.GetString("/Script/TryllClient.TryllRuntimeSettings", "StorageDataFolder", out Storage)
			&& !string.IsNullOrWhiteSpace(Storage))
		{
			Settings.StorageDataFolder = Storage.Trim();
		}

		IReadOnlyList<string> Entries;
		if (Config.TryGetValues("/Script/TryllClient.TryllModelManifest", "Entries", out Entries))
		{
			foreach (string Entry in Entries)
			{
				// (Name="...",ModelType="...",Registration=Production) — or ETryllModelRegistration::Production
				Match Name = Regex.Match(Entry, "Name=\"([^\"]*)\"");
				Match Reg  = Regex.Match(Entry, "Registration=([A-Za-z_:]+)");
				if (Name.Success && Reg.Success
					&& Reg.Groups[1].Value.EndsWith("Production", StringComparison.OrdinalIgnoreCase)
					&& !string.IsNullOrEmpty(Name.Groups[1].Value))
				{
					Settings.ProductionModelNames.Add(Name.Groups[1].Value);
				}
			}
		}
		return Settings;
	}

	/// <summary>
	/// Picks <c>TryllServer/{Default|Release}</c> the same way
	/// <c>UTryllSubsystem::ResolveServerExePath</c> does for the editor: require the exe,
	/// honor Preferred only when both variants exist, otherwise use the single available one.
	/// Returns null when nothing is available.
	/// </summary>
	private static string ResolveServerVariantDir(string ServerRoot, string Preferred)
	{
		if (string.IsNullOrEmpty(ServerRoot) || !Directory.Exists(ServerRoot))
			return null;

		var Available = new List<string>();
		foreach (string Variant in new[] { "Default", "Release" })
		{
			string Dir = Path.Combine(ServerRoot, Variant);
			string Exe = Path.Combine(Dir, "tryll_server.exe");
			if (File.Exists(Exe))
				Available.Add(Variant);
		}
		if (Available.Count == 0)
			return null;
		if (Available.Count == 1 || !Available.Contains(Preferred))
			return Path.Combine(ServerRoot, Available[0]);
		return Path.Combine(ServerRoot, Preferred);
	}

	/// <summary>HuggingFace repo slug → download folder name (convention: '/' → '--').</summary>
	private static string RepoToFolder(string Repo) => Repo.Replace("/", "--");

	private static string SanitizeFolder(string Name)
	{
		foreach (char C in Path.GetInvalidFileNameChars())
			Name = Name.Replace(C, '-');
		return Name;
	}

	private struct StagedModel
	{
		public string   Name;
		public string   Engine;
		public string   Folder;
		public string[] Files;
		public long     TotalBytes;
	}

	private struct StagedAsset
	{
		public string   Id;
		public string   Folder;
		public string[] Files;
		public long     TotalBytes;
	}

	private void StageReachableAssets(
		ReadOnlyTargetRules Target,
		string ServerSource,
		string SrcModelsDir,
		HashSet<string> ProductionNames,
		List<StagedAsset> StagedAssets)
	{
		string ModelsCatalog = Path.Combine(ServerSource, "data", "models.json");
		HashSet<string> Needed = File.Exists(ModelsCatalog)
			? CollectReachableAssetIds(ModelsCatalog, ProductionNames)
			: new HashSet<string>();
		string UserCatalog = ResolveUserModelsCatalogFile(Target);
		if (!string.IsNullOrEmpty(UserCatalog))
		{
			foreach (string Id in CollectReachableAssetIds(UserCatalog, ProductionNames))
				Needed.Add(Id);
		}
		if (Needed.Count == 0)
			return;

		string DownloadsJson = Path.Combine(SrcModelsDir, "downloads.json");
		if (!File.Exists(DownloadsJson))
		{
			throw new BuildException(
				"Production models require shared assets, but downloads.json is missing: " +
				string.Join(", ", Needed));
		}

		JsonObject Root = JsonObject.Read(new FileReference(DownloadsJson));
		JsonObject[] Entries;
		if (!Root.TryGetObjectArrayField("assets", out Entries))
		{
			throw new BuildException(
				"Production models require shared assets, but downloads.json has no assets: " +
				string.Join(", ", Needed));
		}

		foreach (JsonObject E in Entries)
		{
			string Id, Folder;
			string[] Files;
			if (!E.TryGetStringField("id", out Id) || !Needed.Contains(Id))
				continue;
			E.TryGetStringField("folder", out Folder);
			if (!E.TryGetStringArrayField("files", out Files) || Files == null || Files.Length == 0)
				continue;

			string SrcFolder = Path.Combine(SrcModelsDir, Folder ?? "");
			if (string.IsNullOrEmpty(Folder) || !Directory.Exists(SrcFolder))
				continue;

			bool MissingFile = false;
			long Total = 0;
			foreach (string F in Files)
			{
				string SrcFile = Path.Combine(SrcFolder, F);
				if (!File.Exists(SrcFile))
				{
					MissingFile = true;
					break;
				}
				Total += new FileInfo(SrcFile).Length;
				StageNonUfs(Target, "models/" + Folder + "/" + F, SrcFile);
			}
			if (MissingFile)
				continue;
			StagedAssets.Add(new StagedAsset { Id = Id, Folder = Folder, Files = Files, TotalBytes = Total });
			Needed.Remove(Id);
		}

		if (Needed.Count > 0)
		{
			throw new BuildException(
				"Production models require shared assets that were not staged: " +
				string.Join(", ", Needed));
		}
	}

	private static HashSet<string> CollectReachableAssetIds(string ModelsCatalog, HashSet<string> ProductionNames)
	{
		var Needed = new HashSet<string>();
		JsonObject Root = JsonObject.Read(new FileReference(ModelsCatalog));
		JsonObject[] Models;
		if (!Root.TryGetObjectArrayField("models", out Models))
			return Needed;

		foreach (JsonObject M in Models)
		{
			string Name;
			if (!M.TryGetStringField("name", out Name) || !ProductionNames.Contains(Name))
				continue;
			JsonObject[] Variants;
			if (!M.TryGetObjectArrayField("variants", out Variants))
				continue;
			foreach (JsonObject V in Variants)
			{
				JsonObject Deps;
				if (!V.TryGetObjectField("dependencies", out Deps) || Deps == null)
					continue;
				foreach (string Key in Deps.KeyNames)
				{
					string AssetId;
					if (Deps.TryGetStringField(Key, out AssetId) && !string.IsNullOrEmpty(AssetId))
						Needed.Add(AssetId);
				}
			}
		}
		return Needed;
	}

	private static string BuildDownloadsJson(List<StagedModel> Models, List<StagedAsset> Assets)
	{
		var Sb = new StringBuilder();
		Sb.Append("{\n  \"downloads\": [\n");
		for (int i = 0; i < Models.Count; ++i)
		{
			StagedModel M = Models[i];
			var Files = new List<string>();
			foreach (string F in M.Files)
				Files.Add("\"" + Esc(F) + "\"");

			Sb.Append("    {\n");
			Sb.Append("      \"name\": \""   + Esc(M.Name)   + "\",\n");
			Sb.Append("      \"engine\": \"" + Esc(M.Engine) + "\",\n");
			Sb.Append("      \"folder\": \"" + Esc(M.Folder) + "\",\n");
			Sb.Append("      \"files\": [" + string.Join(", ", Files) + "],\n");
			Sb.Append("      \"total_bytes\": " + M.TotalBytes + "\n");
			Sb.Append("    }" + (i + 1 < Models.Count ? "," : "") + "\n");
		}
		Sb.Append("  ],\n  \"assets\": [\n");
		for (int i = 0; i < Assets.Count; ++i)
		{
			StagedAsset A = Assets[i];
			var Files = new List<string>();
			foreach (string F in A.Files)
				Files.Add("\"" + Esc(F) + "\"");

			Sb.Append("    {\n");
			Sb.Append("      \"id\": \""     + Esc(A.Id)     + "\",\n");
			Sb.Append("      \"folder\": \"" + Esc(A.Folder) + "\",\n");
			Sb.Append("      \"files\": [" + string.Join(", ", Files) + "],\n");
			Sb.Append("      \"total_bytes\": " + A.TotalBytes + "\n");
			Sb.Append("    }" + (i + 1 < Assets.Count ? "," : "") + "\n");
		}
		Sb.Append("  ]\n}\n");
		return Sb.ToString();
	}

	private static string Esc(string S) => (S ?? "").Replace("\\", "\\\\").Replace("\"", "\\\"");

	[DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
	private static extern SafeFileHandle CreateFileW(
		string lpFileName,
		uint dwDesiredAccess,
		uint dwShareMode,
		IntPtr lpSecurityAttributes,
		uint dwCreationDisposition,
		uint dwFlagsAndAttributes,
		IntPtr hTemplateFile);

	[DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
	private static extern uint GetFinalPathNameByHandleW(
		SafeFileHandle hFile,
		[Out] char[] lpszFilePath,
		uint cchFilePath,
		uint dwFlags);

	private const uint FILE_SHARE_ALL             = 0x7;
	private const uint OPEN_EXISTING              = 3;
	private const uint FILE_FLAG_BACKUP_SEMANTICS = 0x02000000;

	private static string ResolveLinks(string InputPath)
	{
		string Full = Path.GetFullPath(InputPath);
		if (!RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
		{
			return Full;
		}
		try
		{
			using (SafeFileHandle Handle = CreateFileW(
				Full, 0, FILE_SHARE_ALL, IntPtr.Zero, OPEN_EXISTING,
				FILE_FLAG_BACKUP_SEMANTICS, IntPtr.Zero))
			{
				if (Handle.IsInvalid) return Full;
				char[] Buffer = new char[2048];
				uint Length = GetFinalPathNameByHandleW(Handle, Buffer, (uint)Buffer.Length, 0);
				if (Length == 0 || Length >= Buffer.Length) return Full;
				string Result = new string(Buffer, 0, (int)Length);
				if (Result.StartsWith(@"\\?\UNC\"))      Result = @"\\" + Result.Substring(8);
				else if (Result.StartsWith(@"\\?\"))     Result = Result.Substring(4);
				return Result;
			}
		}
		catch
		{
			return Full;
		}
	}
}
