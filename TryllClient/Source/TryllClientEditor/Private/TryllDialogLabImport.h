// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

struct FTryllDialogLabRunResult;
struct FTryllLabScriptSnapshot;
struct FTryllLabTurnRecord;
struct FTryllLabCheckOutcome;

/**
 * Loads a Dialog Lab JSONL file (format 4, or the older 3) into an immutable run result for
 * read-only display. Does not recreate assets, agents, or runnable config.
 */
struct TRYLLCLIENTEDITOR_API FTryllDialogLabImport
{
	/** The pre-typed-check format, still readable: its step fields convert through the same
	 *  migration as an old asset, and its failures map onto the converted checks. */
	static constexpr int32 LegacyFormatVersion = 3;

	static bool TryLoad(const FString& Text, FTryllDialogLabRunResult& OutRun, FString& OutError);

	/** Variant swatches are not exported, so they are re-assigned from a fixed palette by index. */
	static FLinearColor PaletteColor(int32 Index);

	/**
	 * Format 3 recorded only failures, identified by kind + arg. Ties each one to the converted
	 * check it came from; on a turn whose checks ran (Ok or CheckFail) every check with no
	 * failure passed, because format 3 evaluated all of them. A failure that matches nothing (a
	 * label-only record, or a global pattern on a seeded turn, which format 3 still checked) is
	 * kept with index -1 so the cell stays failed. A port of Unity's MapV3Outcomes.
	 */
	static TArray<FTryllLabCheckOutcome> MapV3Outcomes(
		const FTryllLabScriptSnapshot& Script, int32 TurnIndex, const FTryllLabTurnRecord& Record,
		const TArray<FTryllLabCheckOutcome>& Failures);
};
