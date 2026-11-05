// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllRuntimeSettings.h"

#include "TryllClientModule.h"
#include "TryllEditorBridge.h"
#include "TryllModelManifest.h"
#include "TryllSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

void UTryllRuntimeSettings::RelaunchRunningServer(const FString& Reason)
{
	const UTryllRuntimeSettings* Settings = GetDefault<UTryllRuntimeSettings>();
	if (!Settings || !Settings->bAutoLaunchServer)
	{
		return;
	}

#if WITH_EDITOR
	if (ITryllEditorBridge* Bridge = FTryllEditorBridge::Get())
	{
		// No launch yet this editor session: the next auto-launch reads the
		// path we just saved. A live port means a warm server to replace.
		if (Bridge->GetActualPort() <= 0)
		{
			return;
		}

		const bool bWasConnected = Bridge->IsConnected();
		FString Error;
		if (!Bridge->TryLaunch(
			Settings->ServerBuildVariant,
			Settings->ServerPort,
			/*IdleShutdownTimeoutSeconds=*/60,
			Error))
		{
			if (!Error.IsEmpty())
			{
				UE_LOG(LogTryll, Warning,
					TEXT("[Tryll] %s, but the server did not restart: %s"),
					*Reason, *Error);
			}
			return;
		}

		if (bWasConnected)
		{
			Bridge->AdoptRelaunchedServer();
		}

		if (GEngine)
		{
			const int32 Port = Bridge->GetActualPort();
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UGameInstance* Game = Context.OwningGameInstance;
				if (!Game)
				{
					continue;
				}
				if (UTryllSubsystem* Subsystem = Game->GetSubsystem<UTryllSubsystem>())
				{
					Subsystem->ServerPort = Port;
				}
			}
		}
		return;
	}
#endif

	if (!GEngine)
	{
		return;
	}
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UGameInstance* Game = Context.OwningGameInstance;
		if (!Game)
		{
			continue;
		}
		if (UTryllSubsystem* Subsystem = Game->GetSubsystem<UTryllSubsystem>())
		{
			Subsystem->RelaunchServerForUserCatalog();
		}
	}
}

bool UTryllRuntimeSettings::TryNormalizeUserModelsCatalogPath(
	const FString& ProjectDir,
	const FString& Configured,
	FString& OutRelative,
	FString& OutError)
{
	OutRelative.Reset();
	OutError.Reset();

	const FString Trimmed = Configured.TrimStartAndEnd();
	if (Trimmed.IsEmpty())
	{
		return true;
	}

	auto ToSlash = [](FString Path)
	{
		Path.ReplaceInline(TEXT("\\"), TEXT("/"));
		return Path;
	};

	FString Root = ToSlash(FPaths::ConvertRelativePathToFull(ProjectDir));
	while (Root.EndsWith(TEXT("/")))
	{
		Root.LeftChopInline(1);
	}
	const FString RootPrefix = Root + TEXT("/");

	const FString Full = ToSlash(FPaths::IsRelative(Trimmed)
		? FPaths::ConvertRelativePathToFull(FPaths::ConvertRelativePathToFull(ProjectDir) / Trimmed)
		: FPaths::ConvertRelativePathToFull(Trimmed));

	if (!Full.StartsWith(RootPrefix, ESearchCase::IgnoreCase))
	{
		OutError = TEXT("Custom models catalog must be inside the project. ")
			TEXT("Use a path relative to the project folder, not a path outside it.");
		return false;
	}

	OutRelative = Full.RightChop(RootPrefix.Len());
	if (OutRelative.IsEmpty() || OutRelative.EndsWith(TEXT("/")))
	{
		OutError = TEXT("Custom models catalog must be a file inside the project, not the project folder itself.");
		OutRelative.Reset();
		return false;
	}
	return true;
}

bool UTryllRuntimeSettings::SetUserModelsCatalogPath(const FString& RelativePath, FString& OutError)
{
	FString Normalized;
	if (!TryNormalizeUserModelsCatalogPath(FPaths::ProjectDir(), RelativePath, Normalized, OutError))
	{
		return false;
	}

	UTryllRuntimeSettings* Settings = GetMutableDefault<UTryllRuntimeSettings>();
	if (Settings->UserModelsCatalogPath == Normalized)
	{
		return true;
	}
	Settings->UserModelsCatalogPath = Normalized;
	Settings->SaveConfig();
	RelaunchRunningServer(TEXT("Custom models catalog was saved"));
	return true;
}

FString UTryllRuntimeSettings::ResolveUserModelsCatalogForLaunch() const
{
	FString Relative;
	FString Error;
	if (!TryNormalizeUserModelsCatalogPath(FPaths::ProjectDir(), UserModelsCatalogPath, Relative, Error))
	{
		UE_LOG(LogTryll, Error, TEXT("[Tryll] %s"), *Error);
		return FString();
	}
	if (Relative.IsEmpty())
	{
		return FString();
	}

#if !WITH_EDITOR
	// The cook writes the saved catalog here and rewrites relative chat
	// templates beside it. A different loose file inside the project is used
	// when this staged name is not present.
	const FString Staged = FPaths::ConvertRelativePathToFull(FPaths::Combine(
		FPaths::ProjectDir(), TEXT("Server"), TEXT("data"), TEXT("user-catalog"),
		FPaths::GetCleanFilename(Relative)));
	if (FPaths::FileExists(Staged))
	{
		return Staged;
	}
#endif
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / Relative);
}

FString UTryllRuntimeSettings::ResolveSaveDataFolderForLaunch() const
{
	if (!SaveDataFolder.IsEmpty())
	{
		return ResolveStorageDataFolderForServer(SaveDataFolder);
	}
#if WITH_EDITOR
	return FString();
#else
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
#endif
}

FString UTryllRuntimeSettings::ResolveStorageDataFolderForServer() const
{
#if !WITH_EDITOR
	// Packaged builds always stage storage to ProjectDir/TryllStorage (Build.cs),
	// regardless of the configured relative name from the editor project.
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("TryllStorage"));
#else
	return ResolveStorageDataFolderForServer(StorageDataFolder);
#endif
}

FString UTryllRuntimeSettings::ResolveProjectName() const
{
	const FString Trimmed = GameName.TrimStartAndEnd();
	if (!Trimmed.IsEmpty())
	{
		return Trimmed;
	}
	return FString(FApp::GetProjectName());
}

FString UTryllRuntimeSettings::EnsureProjectId()
{
#if WITH_EDITOR
	if (!IsRunningCommandlet())
	{
		bool bValid = ProjectId.Len() == 32;
		for (const TCHAR c : ProjectId)
		{
			if (!FChar::IsHexDigit(c))
			{
				bValid = false;
				break;
			}
		}
		if (!bValid)
		{
			ProjectId = FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower();
			TryUpdateDefaultConfigFile();
		}
	}
#endif
	return ProjectId.ToLower();
}

FString UTryllRuntimeSettings::ResolveStorageDataFolderForServer(const FString& ConfiguredFolder)
{
	if (ConfiguredFolder.IsEmpty())
	{
		return FString();
	}

	if (FPaths::IsRelative(ConfiguredFolder))
	{
		// Always ProjectDir — BaseDir() is the exe folder (Binaries/Win64/), which
		// is wrong for packaged storage staged next to the .uproject.
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / ConfiguredFolder);
	}

	return FPaths::ConvertRelativePathToFull(ConfiguredFolder);
}

bool UTryllRuntimeSettings::IsStorageUnderContent(const FString& ConfiguredFolder)
{
	if (ConfiguredFolder.IsEmpty())
	{
		return false;
	}

	auto ToSlash = [](FString Path)
	{
		Path.ReplaceInline(TEXT("\\"), TEXT("/"));
		return Path;
	};

	FString ContentRoot = ToSlash(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()));
	while (ContentRoot.EndsWith(TEXT("/")))
	{
		ContentRoot.LeftChopInline(1);
	}
	const FString ContentPrefix = ContentRoot + TEXT("/");

	const FString Resolved = ToSlash(ResolveStorageDataFolderForServer(ConfiguredFolder));
	if (Resolved.IsEmpty())
	{
		return false;
	}

#if PLATFORM_WINDOWS
	return Resolved.Equals(ContentRoot, ESearchCase::IgnoreCase)
		|| Resolved.StartsWith(ContentPrefix, ESearchCase::IgnoreCase);
#else
	return Resolved.Equals(ContentRoot) || Resolved.StartsWith(ContentPrefix);
#endif
}

bool UTryllRuntimeSettings::MoveStorageToTryllStorage(FString& OutError)
{
	OutError.Reset();

	UTryllRuntimeSettings* Settings = GetMutableDefault<UTryllRuntimeSettings>();
	if (!Settings)
	{
		OutError = TEXT("Tryll runtime settings are unavailable.");
		return false;
	}

	const FString DestRel = TEXT("TryllStorage");
	const FString DestAbs = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / DestRel);
	const FString SrcAbs = ResolveStorageDataFolderForServer(Settings->StorageDataFolder);

	auto ToSlash = [](FString Path)
	{
		Path.ReplaceInline(TEXT("\\"), TEXT("/"));
		while (Path.EndsWith(TEXT("/")))
		{
			Path.LeftChopInline(1);
		}
		return Path;
	};

	const FString SrcNorm = ToSlash(SrcAbs);
	const FString DestNorm = ToSlash(DestAbs);
#if PLATFORM_WINDOWS
	const bool bAlreadyThere = SrcNorm.Equals(DestNorm, ESearchCase::IgnoreCase);
#else
	const bool bAlreadyThere = SrcNorm.Equals(DestNorm);
#endif

	if (!bAlreadyThere)
	{
		if (SrcNorm.IsEmpty() || !FPaths::DirectoryExists(SrcAbs))
		{
			OutError = FString::Printf(
				TEXT("Storage folder '%s' does not exist on disk, so it cannot be moved."),
				Settings->StorageDataFolder.IsEmpty() ? TEXT("(empty)") : *Settings->StorageDataFolder);
			return false;
		}

		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		const FString ProjectNorm = ToSlash(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()));
		const FString ContentNorm = ToSlash(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()));
		const bool bInsideProject = SrcNorm.StartsWith(ProjectNorm + TEXT("/"));
		if (!bInsideProject
			|| SrcNorm.Equals(ProjectNorm, ESearchCase::IgnoreCase)
			|| SrcNorm.Equals(ContentNorm, ESearchCase::IgnoreCase)
			|| DestNorm.StartsWith(SrcNorm + TEXT("/"), ESearchCase::IgnoreCase))
		{
			OutError = TEXT("Refusing to move that folder. Point storage at the Tryll folder under Content, not Content itself, the project root, or a path outside the project.");
			return false;
		}

		if (FPaths::DirectoryExists(DestAbs))
		{
			OutError = FString::Printf(
				TEXT("'%s' already exists. Move or rename it, then try again."),
				*DestAbs);
			return false;
		}

		if (!PlatformFile.CreateDirectoryTree(*FPaths::GetPath(DestAbs)))
		{
			OutError = FString::Printf(TEXT("Could not create parent folder for '%s'."), *DestAbs);
			return false;
		}

		if (!PlatformFile.CopyDirectoryTree(*DestAbs, *SrcAbs, /*bOverwriteAllExisting=*/false))
		{
			if (FPaths::DirectoryExists(DestAbs))
			{
				PlatformFile.DeleteDirectoryRecursively(*DestAbs);
			}
			OutError = FString::Printf(
				TEXT("Could not copy '%s' to '%s'."), *SrcAbs, *DestAbs);
			return false;
		}
		if (!PlatformFile.DeleteDirectoryRecursively(*SrcAbs))
		{
			UE_LOG(LogTryll, Warning,
				TEXT("[Tryll] Copied storage to '%s' but could not remove the old folder '%s'."),
				*DestAbs, *SrcAbs);
		}
	}

	Settings->StorageDataFolder = DestRel;
	Settings->SaveConfig();
	RelaunchRunningServer(TEXT("Custom models catalog was saved"));
	return true;
}

FString UTryllRuntimeSettings::GetConfiguredStorageDataFolder()
{
	FString FromIni;
	if (GConfig->GetString(
		TEXT("/Script/TryllClient.TryllRuntimeSettings"),
		TEXT("StorageDataFolder"),
		FromIni,
		GGameIni)
		&& !FromIni.IsEmpty())
	{
		return FromIni;
	}

	if (const UTryllRuntimeSettings* Settings = GetDefault<UTryllRuntimeSettings>())
	{
		return Settings->StorageDataFolder;
	}

	return FString();
}

FString UTryllRuntimeSettings::GetStorageRootAbsolute()
{
	const FString Resolved = ResolveStorageDataFolderForServer(GetConfiguredStorageDataFolder());
	if (!Resolved.IsEmpty())
	{
		return Resolved;
	}

	return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
}

bool UTryllRuntimeSettings::MakePathRelativeToStorageRoot(
	const FString& AbsoluteFilePath,
	FString& OutRelativePath)
{
	const FString Root = ResolveStorageDataFolderForServer(GetConfiguredStorageDataFolder());
	if (Root.IsEmpty())
	{
		return false;
	}

	FString RootNorm = FPaths::ConvertRelativePathToFull(Root);
	FString FileNorm = FPaths::ConvertRelativePathToFull(AbsoluteFilePath);
	RootNorm.ReplaceInline(TEXT("\\"), TEXT("/"));
	FileNorm.ReplaceInline(TEXT("\\"), TEXT("/"));
	if (!RootNorm.EndsWith(TEXT("/")))
	{
		RootNorm += TEXT("/");
	}

#if PLATFORM_WINDOWS
	const bool bUnderRoot = FileNorm.StartsWith(RootNorm, ESearchCase::IgnoreCase);
#else
	const bool bUnderRoot = FileNorm.StartsWith(RootNorm);
#endif
	if (!bUnderRoot)
	{
		return false;
	}

	OutRelativePath = FileNorm.Mid(RootNorm.Len());
	OutRelativePath.ReplaceInline(TEXT("\\"), TEXT("/"));
	return !OutRelativePath.StartsWith(TEXT(".."));
}

FString UTryllRuntimeSettings::NormalizeStorageRelativePath(const FString& RelativePath)
{
	if (RelativePath.IsEmpty())
	{
		return RelativePath;
	}

	const FString Configured = GetConfiguredStorageDataFolder();
	if (Configured.IsEmpty())
	{
		return RelativePath;
	}

	FString Path = RelativePath;
	Path.ReplaceInline(TEXT("\\"), TEXT("/"));

	FString ConfigNorm = Configured;
	ConfigNorm.ReplaceInline(TEXT("\\"), TEXT("/"));
	ConfigNorm.TrimStartAndEndInline();
	while (ConfigNorm.StartsWith(TEXT("/")))
	{
		ConfigNorm.RightChopInline(1);
	}

	if (!ConfigNorm.IsEmpty()
		&& (Path.Equals(ConfigNorm, ESearchCase::IgnoreCase)
			|| Path.StartsWith(ConfigNorm + TEXT("/"), ESearchCase::IgnoreCase)))
	{
		Path = Path.Mid(ConfigNorm.Len());
		while (Path.StartsWith(TEXT("/")))
		{
			Path.RightChopInline(1);
		}
	}

	const FString StorageLeaf = FPaths::GetPathLeaf(ConfigNorm);
	if (!StorageLeaf.IsEmpty())
	{
		const FString LeafPrefix = StorageLeaf + TEXT("/");
		if (Path.StartsWith(LeafPrefix, ESearchCase::IgnoreCase))
		{
			Path = Path.Mid(LeafPrefix.Len());
		}
	}

	return Path;
}

TArray<FString> UTryllRuntimeSettings::GetLocaleOptions() const
{
	// Empty first: "no session language" is a meaningful choice (pre-locale
	// behaviour), not a missing value.
	TArray<FString> Options;
	Options.Add(FString());

	const UTryllModelManifest* Manifest = UTryllModelManifest::GetActive();
	if (Manifest)
	{
		Options.Append(Manifest->DeclaredLanguages());
	}

	// A value set by hand (or left over after unregistering the model that
	// declared it) must never vanish from the dropdown and be silently rewritten.
	if (!Locale.IsEmpty() && !Options.Contains(Locale))
	{
		Options.Add(Locale);
	}
	return Options;
}

// ── Manifest-derived engine set (protocol v13) ───────────────────────────────

namespace
{
/// Wire ModelRole ordinal for a catalog model-type string. A VAD model is loaded
/// as an STT dependency and has no declaration of its own, so it maps onto Stt.
uint8 RoleOrdinalFor(const FString& ModelType)
{
	if (ModelType == TEXT("embedding")) return 1;
	if (ModelType == TEXT("stt"))       return 2;
	if (ModelType == TEXT("tts"))       return 3;
	if (ModelType == TEXT("vad"))       return 2;
	return 0;   // "language"
}
} // namespace

TArray<TPair<uint8, uint8>> UTryllRuntimeSettings::BuildEngineDeclarations()
{
	TArray<TPair<uint8, uint8>> Result;

	const UTryllModelManifest* Manifest = UTryllModelManifest::GetActive();
	if (!Manifest)
	{
		return Result;
	}

	// Several models legitimately share one (role, engine) pair. Deduping is now
	// only tidiness — the server logs a duplicate and ignores it — but sending the
	// same pair five times would make the pre-warm log unreadable.
	TSet<TPair<uint8, uint8>> Seen;
	for (const FTryllModelManifestEntry& Entry : Manifest->Entries)
	{
		if (Entry.Registration == ETryllModelRegistration::None)
		{
			continue;
		}
		const TPair<uint8, uint8> Pair{
			RoleOrdinalFor(Entry.ModelType), static_cast<uint8>(Entry.Engine)};
		if (!Seen.Contains(Pair))
		{
			Seen.Add(Pair);
			Result.Add(Pair);
		}
	}
	return Result;
}

TArray<int32> UTryllRuntimeSettings::BuildEngineDeclarationsFlat()
{
	// Blueprint has no TPair, so the pairs are flattened to [role, engine, ...].
	TArray<int32> Flat;
	for (const TPair<uint8, uint8>& Pair : BuildEngineDeclarations())
	{
		Flat.Add(Pair.Key);
		Flat.Add(Pair.Value);
	}
	return Flat;
}

TArray<TPair<FString, uint8>> UTryllRuntimeSettings::BuildHotwordHints()
{
	TArray<TPair<FString, uint8>> Result;

	const UTryllModelManifest* Manifest = UTryllModelManifest::GetActive();
	if (!Manifest)
	{
		return Result;
	}

	for (const FTryllModelManifestEntry& Entry : Manifest->Entries)
	{
		if (Entry.Registration == ETryllModelRegistration::None)
		{
			continue;
		}
		// Sparse by design: Default is the absence of an opinion, and sending it
		// would be indistinguishable from an explicit "follow the catalog".
		if (Entry.Hotwords == ETryllHotwordBias::Default)
		{
			continue;
		}
		Result.Add(TPair<FString, uint8>(Entry.Name, static_cast<uint8>(Entry.Hotwords)));
	}
	return Result;
}

TArray<TPair<FString, uint8>> UTryllRuntimeSettings::BuildPlacementHints()
{
	TArray<TPair<FString, uint8>> Result;

	const UTryllModelManifest* Manifest = UTryllModelManifest::GetActive();
	if (!Manifest)
	{
		return Result;
	}

	for (const FTryllModelManifestEntry& Entry : Manifest->Entries)
	{
		if (Entry.Registration == ETryllModelRegistration::None)
		{
			continue;
		}
		// Sparse by design: a model left on Auto expresses no opinion.
		if (Entry.Placement == ETryllMemoryPlacement::Auto)
		{
			continue;
		}
		// VAD entries are download/config metadata, not models the server caches — a
		// detector is built per STT session inside the recognizer, and the server pins
		// it to the CPU deliberately. A hint for one would be a value nothing reads, so
		// the Model Manager does not offer the control and this does not send it.
		// Filtered here as well as hidden there, because a manifest authored before the
		// control was hidden can still carry a stale VAD placement.
		if (Entry.ModelType.Equals(TEXT("vad"), ESearchCase::IgnoreCase))
		{
			continue;
		}
		Result.Add({Entry.Name, static_cast<uint8>(Entry.Placement)});
	}
	return Result;
}
