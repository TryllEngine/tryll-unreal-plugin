// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllLicence.h"

#include "TryllDevCheckout.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"

// Named, not anonymous: UBT unity builds merge this file with others in the module.
namespace TryllLicencePrivate
{
const TCHAR* const StoreId = TEXT("Tryll");
const TCHAR* const StoreSection = TEXT("Welcome");
const TCHAR* const AcceptedVersionKey = TEXT("LicenceAcceptedVersion");
const TCHAR* const AcceptedAtKey = TEXT("LicenceAcceptedAt");
const TCHAR* const InstallIdKey = TEXT("InstallId");
const TCHAR* const LastInstallEventVersionKey = TEXT("LastInstallEventVersion");

bool bUseMemoryStore = false;
TMap<FString, FString> MemoryStore;
FString LicenceFileOverride;

/** CurrentVersion() for one file state, so a per-frame IsAccepted() does not re-hash. */
struct FVersionCache
{
	FString Path;
	FDateTime ModificationTime;
	int64 Size = -1;
	FString Version;
};
FVersionCache VersionCache;

FString ReadValue(const TCHAR* Key)
{
	FString Value;
	if (bUseMemoryStore)
	{
		if (const FString* Found = MemoryStore.Find(Key))
		{
			Value = *Found;
		}
		return Value;
	}
	FPlatformMisc::GetStoredValue(StoreId, StoreSection, Key, Value);
	return Value;
}

void WriteValue(const TCHAR* Key, const FString& Value)
{
	if (bUseMemoryStore)
	{
		MemoryStore.Add(Key, Value);
		return;
	}
	FPlatformMisc::SetStoredValue(StoreId, StoreSection, Key, Value);
}

void DeleteValue(const TCHAR* Key)
{
	if (bUseMemoryStore)
	{
		MemoryStore.Remove(Key);
		return;
	}
	FPlatformMisc::DeleteStoredValue(StoreId, StoreSection, Key);
}

/** BOM stripped and line endings as "\n". False when the file is missing or unreadable. */
bool TryLoadText(const FString& Path, FString& OutText)
{
	OutText.Reset();
	if (Path.IsEmpty() || !FPaths::FileExists(Path) || !FFileHelper::LoadFileToString(OutText, *Path))
	{
		OutText.Reset();
		return false;
	}
	if (OutText.Len() > 0 && OutText[0] == TCHAR(0xFEFF))
	{
		OutText.RightChopInline(1, EAllowShrinking::No);
	}
	OutText.ReplaceInline(TEXT("\r\n"), TEXT("\n"), ESearchCase::CaseSensitive);
	return true;
}

FString HashVersion(FString Text)
{
	Text.TrimEndInline();
	const FTCHARToUTF8 Utf8(*Text);
	uint8 Digest[FSHA1::DigestSize];
	FSHA1::HashBuffer(Utf8.Get(), static_cast<uint64>(Utf8.Length()), Digest);
	return FString(TEXT("sha1:")) + BytesToHex(Digest, 8).ToLower();
}
} // namespace TryllLicencePrivate

const TCHAR* const FTryllLicence::EvaluationDraftVersion = TEXT("evaluation-draft");

FString FTryllLicence::LicenceFilePath()
{
	using namespace TryllLicencePrivate;
	if (!LicenceFileOverride.IsEmpty())
	{
		return LicenceFileOverride;
	}
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("TryllClient"));
	if (!Plugin.IsValid())
	{
		return FString();
	}
	const FString PluginFile = FPaths::ConvertRelativePathToFull(FPaths::Combine(Plugin->GetBaseDir(), TEXT("LICENSE.md")));
	if (FPaths::FileExists(PluginFile))
	{
		return PluginFile;
	}
	// A development checkout: the file the release copies into the plugin.
	const FString RepoFile = FTryllDevCheckout::RepoLicencePath();
	return RepoFile.IsEmpty() ? PluginFile : RepoFile;
}

bool FTryllLicence::HasLicenceFile()
{
	const FString Path = LicenceFilePath();
	return !Path.IsEmpty() && FPaths::FileExists(Path);
}

FString FTryllLicence::LoadLicenceText()
{
	FString Text;
	TryllLicencePrivate::TryLoadText(LicenceFilePath(), Text);
	return Text;
}

FString FTryllLicence::CurrentVersion()
{
	using namespace TryllLicencePrivate;
	const FString Path = LicenceFilePath();
	const FFileStatData Stat = Path.IsEmpty() ? FFileStatData() : IFileManager::Get().GetStatData(*Path);
	if (!Stat.bIsValid || Stat.bIsDirectory)
	{
		return EvaluationDraftVersion;
	}
	if (VersionCache.Path == Path
		&& VersionCache.ModificationTime == Stat.ModificationTime
		&& VersionCache.Size == Stat.FileSize)
	{
		return VersionCache.Version;
	}

	FString Text;
	if (!TryLoadText(Path, Text))
	{
		return EvaluationDraftVersion;
	}
	VersionCache.Path = Path;
	VersionCache.ModificationTime = Stat.ModificationTime;
	VersionCache.Size = Stat.FileSize;
	VersionCache.Version = HashVersion(MoveTemp(Text));
	return VersionCache.Version;
}

bool FTryllLicence::IsAccepted()
{
	const FString Stored = TryllLicencePrivate::ReadValue(TryllLicencePrivate::AcceptedVersionKey);
	return !Stored.IsEmpty() && Stored == CurrentVersion();
}

void FTryllLicence::SetAccepted(bool bAccepted)
{
	using namespace TryllLicencePrivate;
	if (bAccepted)
	{
		WriteValue(AcceptedVersionKey, CurrentVersion());
		WriteValue(AcceptedAtKey, FDateTime::UtcNow().ToIso8601());
	}
	else
	{
		DeleteValue(AcceptedVersionKey);
		DeleteValue(AcceptedAtKey);
	}
	OnChanged().Broadcast();
}

FString FTryllLicence::GetInstallId()
{
	using namespace TryllLicencePrivate;
	FString Id = ReadValue(InstallIdKey);
	if (Id.IsEmpty())
	{
		Id = FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower();
		WriteValue(InstallIdKey, Id);
	}
	return Id;
}

FString FTryllLicence::GetLastInstallEventVersion()
{
	return TryllLicencePrivate::ReadValue(TryllLicencePrivate::LastInstallEventVersionKey);
}

void FTryllLicence::SetLastInstallEventVersion(const FString& Version)
{
	TryllLicencePrivate::WriteValue(TryllLicencePrivate::LastInstallEventVersionKey, Version);
}

FSimpleMulticastDelegate& FTryllLicence::OnChanged()
{
	static FSimpleMulticastDelegate Changed;
	return Changed;
}

void FTryllLicence::DebugUseInMemoryStoreForTests(bool bEnable)
{
	TryllLicencePrivate::bUseMemoryStore = bEnable;
	TryllLicencePrivate::MemoryStore.Reset();
}

bool FTryllLicence::IsUsingInMemoryStoreForTests()
{
	return TryllLicencePrivate::bUseMemoryStore;
}

void FTryllLicence::DebugSetLicenceFileForTests(const FString& PathOrEmpty)
{
	TryllLicencePrivate::LicenceFileOverride = PathOrEmpty;
	TryllLicencePrivate::VersionCache = TryllLicencePrivate::FVersionCache();
}

void FTryllLicence::DebugResetForTests()
{
	DebugUseInMemoryStoreForTests(false);
	DebugSetLicenceFileForTests(FString());
}
