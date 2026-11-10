// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Generated/Nodes/TryllDocsBase.gen.h"

/**
 * Published docs URLs used by the Unreal editor. TRYLL_DOCS_BASE_URL is pinned by codegen
 * to the repo VERSION's `/vX.Y/` folder, so links work from a source checkout as well as
 * from a release (the release stamper leaves already-pinned URLs alone).
 */
namespace TryllDocs
{
	inline constexpr TCHAR BaseUrl[] = TRYLL_DOCS_BASE_URL;
	inline constexpr TCHAR VoiceInput[] = TRYLL_DOCS_BASE_URL TEXT("how-to/use-voice-input.html");
	inline constexpr TCHAR GettingStartedUnreal[] = TRYLL_DOCS_BASE_URL TEXT("getting-started/first-inference-unreal.html");
	inline constexpr TCHAR Telemetry[] = TRYLL_DOCS_BASE_URL TEXT("reference/telemetry.html");
	inline constexpr TCHAR ReportABug[] = TRYLL_DOCS_BASE_URL TEXT("how-to/report-a-bug.html");
	/** The licence for this docs version, so it is the text this plugin ships. */
	inline constexpr TCHAR Licence[] = TRYLL_DOCS_BASE_URL TEXT("reference/legal/licence.html");
}
