// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

/**
 * Extracts and rewrites `{{slot.<name>}}` / `{{user_message}}` references in a
 * Mustache template source. This is one of three independent implementations
 * that must agree byte-for-byte on the shared test vectors in
 * `tryll/schema/mustache-refs-vectors.json` — the other two are the C++
 * server (tryll/server/workflow/src/MustacheRefs.cpp) and the Unity (C#) editor port.
 *
 * Scope: recognises the tag forms the kainjow Mustache renderer actually accepts:
 *   - `{{slot.<name>}}`  — reports `<name>`.
 *   - `{{user_message}}` — reports the literal "user_message".
 *   - Either form padded with whitespace inside the braces (`{{ slot.x }}`,
 *     `{{ user_message }}`).
 *   - Either form using triple (unescaped) braces (`{{{slot.x}}}`,
 *     `{{{ user_message }}}`).
 * Every other tag (`{{knowledge_<source>}}`, `{{#instructions}}...{{/instructions}}`,
 * section/inverted/partial tags) is left untouched and never reported or rewritten.
 */
namespace TryllMustacheRefs
{
	// Exported (not just visible within TryllClientEditor) so TryllClientTests'
	// vector-driven automation spec can link against it across the module DLL boundary.
	[[nodiscard]] TRYLLCLIENTEDITOR_API TArray<FString> ExtractRefs(const FString& Tmpl);

	[[nodiscard]] TRYLLCLIENTEDITOR_API FString Rename(const FString& Tmpl, const FString& From, const FString& To);

	// One scan. A name written by one pair is not matched as another pair's source.
	[[nodiscard]] TRYLLCLIENTEDITOR_API FString Rename(
		const FString& Tmpl, const TMap<FString, FString>& Renames);
}
