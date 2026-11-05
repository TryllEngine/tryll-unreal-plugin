// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllLogsFolder.h"

#include "TryllEditorUiContract.h"
#include "TryllRuntimeSettings.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "Misc/App.h"
#include "Misc/Paths.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <bcrypt.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogTryllLogsFolder, Log, All);

namespace
{
	constexpr int32 SlugMaxChars = 48;
	constexpr int32 HashChars = 16;

	bool IsSlugChar(TCHAR C)
	{
		return (C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9')
			|| C == '.' || C == '_' || C == '-';
	}

	TCHAR AsciiLower(TCHAR C)
	{
		return (C >= 'A' && C <= 'Z') ? static_cast<TCHAR>(C + 32) : C;
	}

	// UE's FPlatformMisc::GetSHA256Signature has no Windows implementation (it asserts),
	// so hash with the OS directly. The server, and so this folder, is Windows-only.
	FString Sha256Hex(const FString& Text)
	{
		FString Hex;
#if PLATFORM_WINDOWS
		const FTCHARToUTF8 Utf8(*Text);
		uint8 Digest[32] = {};
		BCRYPT_ALG_HANDLE Alg = nullptr;
		BCRYPT_HASH_HANDLE Hash = nullptr;
		bool bOk = BCRYPT_SUCCESS(::BCryptOpenAlgorithmProvider(&Alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0));
		bOk = bOk && BCRYPT_SUCCESS(::BCryptCreateHash(Alg, &Hash, nullptr, 0, nullptr, 0, 0));
		bOk = bOk && BCRYPT_SUCCESS(::BCryptHashData(Hash,
			reinterpret_cast<PUCHAR>(const_cast<ANSICHAR*>(Utf8.Get())), static_cast<ULONG>(Utf8.Length()), 0));
		bOk = bOk && BCRYPT_SUCCESS(::BCryptFinishHash(Hash, Digest, sizeof(Digest), 0));
		if (Hash)
			::BCryptDestroyHash(Hash);
		if (Alg)
			::BCryptCloseAlgorithmProvider(Alg, 0);
		if (bOk)
			Hex = BytesToHex(Digest, sizeof(Digest)).ToLower();
#endif
		return Hex;
	}
}

namespace TryllLogsFolder
{
	void Open()
	{
		const FString Logs = LogsDirectory();
		if (IFileManager::Get().DirectoryExists(*Logs))
		{
			FPlatformProcess::ExploreFolder(*Logs);
			return;
		}
		const FString Projects = ProjectsDirectory();
		IFileManager::Get().MakeDirectory(*Projects, /*Tree=*/true);
		FPlatformProcess::ExploreFolder(*Projects);
		UE_LOG(LogTryllLogsFolder, Display, TEXT("%s"),
			*FString(FTryllEditorUiContract::HelpLogsNotCreated).Replace(TEXT("{folder}"), *Projects));
	}

	FString LogsDirectory()
	{
		const UTryllRuntimeSettings* Settings = GetDefault<UTryllRuntimeSettings>();
		const FString Name = Settings ? Settings->ResolveProjectName() : FString(FApp::GetProjectName());
		const FString Dir = CanonicalProjectDir(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()));
		return ProjectsDirectory() / ProjectKey(Name, Dir) / TEXT("logs");
	}

	FString ProjectsDirectory()
	{
		return FPlatformMisc::GetEnvironmentVariable(TEXT("LOCALAPPDATA")) / TEXT("Tryll") / TEXT("projects");
	}

	FString CanonicalProjectDir(const FString& Dir)
	{
		FString Text = FPaths::ConvertRelativePathToFull(Dir);
		Text.ReplaceInline(TEXT("\\"), TEXT("/"));
		while (Text.Len() > 3 && Text.EndsWith(TEXT("/")))
			Text.LeftChopInline(1);
		for (TCHAR& C : Text)
			C = AsciiLower(C);
		return Text;
	}

	FString ProjectKey(const FString& ProjectName, const FString& CanonicalDir)
	{
		FString Slug;
		bool bPendingSeparator = false;
		for (const TCHAR C : ProjectName)
		{
			if (IsSlugChar(C))
			{
				const bool bNeedSeparator = bPendingSeparator && !Slug.IsEmpty();
				if (Slug.Len() + (bNeedSeparator ? 1 : 0) + 1 > SlugMaxChars)
					break;
				if (bNeedSeparator)
					Slug.AppendChar('-');
				bPendingSeparator = false;
				Slug.AppendChar(AsciiLower(C));
			}
			else
			{
				bPendingSeparator = true;
			}
		}
		while (!Slug.IsEmpty() && (Slug.EndsWith(TEXT("-")) || Slug.EndsWith(TEXT(".")) || Slug.EndsWith(TEXT("_"))))
			Slug.LeftChopInline(1);
		if (Slug.IsEmpty())
			Slug = TEXT("project");

		return Slug + TEXT("-") + Sha256Hex(CanonicalDir).Left(HashChars);
	}
}
