// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

struct FTryllDialogLabRunResult;

/**
 * Writes a Dialog Lab run as versioned JSONL: one header line carrying provenance and the full
 * script, then one line per cell (seeded, generated and skipped alike). Always consumes an
 * immutable run result — never live panel state.
 *
 * Format 4 is shared with the Unity Lab, byte for byte, so the same studio scripts read both
 * producers and either can import the other's files. 4: typed checks — the header carries
 * dialogues[] (each with checks[] / global_checks[] of {type, enabled, note, …}) and each turn
 * line dialogue_index and check_outcomes[] (passes included, identified by scope + index).
 * Import gates on exact equality and also reads 3. A field added later without a bump must
 * default, when absent, to exactly what older files ran with.
 */
struct TRYLLCLIENTEDITOR_API FTryllDialogLabExport
{
	static constexpr int32 FormatVersion = 4;

	static FString ExportToJsonl(const FTryllDialogLabRunResult& Run);

	/** JSON string escaping: quotes, backslash, newline, return, tab, and control chars. */
	static FString EscapeJson(const FString& Value);
};
