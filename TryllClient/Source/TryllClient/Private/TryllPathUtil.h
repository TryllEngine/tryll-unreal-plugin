// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

namespace Tryll::Client
{

/**
 * Canonical absolute path with symlinks/junctions resolved, matching the
 * server's std::filesystem::weakly_canonical(exe_path). The bundled
 * .../Server path is usually a symlink/junction to the dev build, so without
 * resolving it an identity comparison against the server's own canonical
 * exe_path would trip a false "different server" mismatch.
 *
 * On Windows uses GetFinalPathNameByHandle; falls back to the normalized full
 * path otherwise (the bundled server is Windows-only).
 *
 * Shared by UTryllSubsystem's post-connect identity check and
 * UTryllEditorSubsystem's known-port reuse probe (editor module — hence API export).
 */
TRYLLCLIENT_API [[nodiscard]] FString CanonicalizeExePath(const FString& InPath);

} // namespace Tryll::Client
