// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllPathUtil.h"

#include "Misc/Paths.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/WindowsHWrapper.h"
#endif

namespace Tryll::Client
{

FString CanonicalizeExePath(const FString& InPath)
{
	const FString Full = FPaths::ConvertRelativePathToFull(InPath);
#if PLATFORM_WINDOWS
	// dwDesiredAccess = 0 → query metadata only; works even on the running exe.
	HANDLE Handle = ::CreateFileW(*Full, 0,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
		OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	if (Handle != INVALID_HANDLE_VALUE)
	{
		TCHAR Buffer[1024];
		const DWORD Len = ::GetFinalPathNameByHandleW(Handle, Buffer, UE_ARRAY_COUNT(Buffer), 0);
		::CloseHandle(Handle);
		if (Len > 0 && Len < UE_ARRAY_COUNT(Buffer))
		{
			FString Result(Buffer);
			const FString ExtPrefix = TEXT("\\\\?\\");
			if (Result.StartsWith(ExtPrefix))
			{
				Result = Result.RightChop(ExtPrefix.Len());
			}
			return Result;
		}
	}
#endif
	return Full;
}

} // namespace Tryll::Client

#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif
