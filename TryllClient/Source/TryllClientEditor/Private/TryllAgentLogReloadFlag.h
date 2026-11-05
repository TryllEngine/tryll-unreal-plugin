// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

/**
 * Session-scoped Agent Log "history discarded by reload" flag. The bool is stored
 * in GEditorPerProjectIni so it survives in-process hot reload; the process id
 * prevents a later editor start from honoring a leftover true.
 */
struct TRYLLCLIENTEDITOR_API FTryllAgentLogReloadFlag
{
	static constexpr const TCHAR* Section = TEXT("/Script/TryllClientEditor.AgentLog");
	static constexpr const TCHAR* DiscardKey = TEXT("bHistoryDiscardedByReload");
	static constexpr const TCHAR* ProcessIdKey = TEXT("ReloadFlagProcessId");

	static bool ShouldHonor(bool bDiscard, uint32 StoredProcessId, uint32 CurrentProcessId);

	/** Write the flag for this editor process. */
	static void Write(bool bDiscard);

	/**
	 * True when a same-process hot reload left a discard. Clears the bool after a
	 * match so a later restart cannot see it.
	 */
	static bool ConsumeIfCurrentProcess();
};
