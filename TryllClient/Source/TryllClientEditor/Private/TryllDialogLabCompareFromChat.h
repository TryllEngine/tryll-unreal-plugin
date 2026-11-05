// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

class UTryllDialogScriptAsset;

/** One captured chat turn. Mirrors the chat panel's own entry shape without depending on it. */
struct FTryllChatHistoryEntry
{
	bool    bIsUser = false;
	FString Text;
};

/**
 * Builds the transient Dialog Script behind Compare-in-Lab: the captured chat history as seeded
 * steps, plus one blank generated continuation so Run has work to do.
 *
 * Slate-free on purpose — the pair-alternation rule is the part worth testing, and a test should
 * not have to stand up a panel to reach it.
 */
struct TRYLLCLIENTEDITOR_API FTryllDialogLabCompareFromChat
{
	static const TCHAR* ScriptName;
	static const TCHAR* ScriptDescription;

	/**
	 * Fills Asset from History. Consumes strict user/assistant pairs and stops at the first entry
	 * that breaks the alternation, so a trailing unanswered user line is never turned into a
	 * half-seeded step (21.2). SeedTurnCount ends up as the number of complete pairs, and exactly
	 * one blank generated step is appended (21.3) — the user fills it in before running, which
	 * §3.5 refuses otherwise.
	 */
	static void ApplyToAsset(
		UTryllDialogScriptAsset* Asset, const TArray<FTryllChatHistoryEntry>& History);
};
