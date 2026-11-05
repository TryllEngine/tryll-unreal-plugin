// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

/**
 * Extracts `{{var.<name>}}` references (per-agent Variables) from a Mustache
 * template source. Independent from TryllMustacheRefs — that scanner is
 * contract-locked byte-for-byte with the C++ server and the Unity editor port
 * for `slot.`/`user_message` extraction (rename propagation); this one is
 * Unreal-only tooling for the newer per-agent Variables feature and must NOT
 * be merged into it (mirrors tryll/clients/unity/Editor/TryllVarRefs.cs).
 *
 * Scope mirrors the server's ScanVarRefs
 * (tryll/server/workflow/src/TemplateValidation.cpp) — the same tag shapes
 * TryllMustacheRefs recognises for `slot.`, so anything kainjow would actually
 * resolve at render time is caught here too:
 *   - `{{var.<name>}}`  — reports `<name>`.
 *   - Padded with whitespace inside the braces (`{{ var.x }}`).
 *   - Triple (unescaped) braces (`{{{var.x}}}`), including combined with
 *     padding (`{{{ var.x }}}`).
 * Every other tag is left unreported.
 */
namespace TryllVarRefs
{
	// Exported so TryllClientTests' automation spec can link against it across
	// the module DLL boundary.
	[[nodiscard]] TRYLLCLIENTEDITOR_API TArray<FString> ExtractRefs(const FString& Tmpl);
}
