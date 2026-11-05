// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

/**
 * Window > Tryll > Open Tryll Logs Folder: opens the folder where the editor's server
 * writes its logs and crash dumps, %LOCALAPPDATA%\Tryll\projects\<key>\logs.
 *
 * The key is recomputed here rather than asked of the server, because the server is
 * usually gone when someone needs its crash dump. It must match
 * tryll/server/runtime/src/ProjectKey.cpp; both are tested against
 * tryll/schema/project-key-vectors.json. When the folder does not exist (no server
 * run yet, or a path the server canonicalised differently, such as a symlink), the
 * parent "projects" folder opens instead.
 */
namespace TryllLogsFolder
{
	/** Opens this project's logs folder, or the parent "projects" folder. */
	TRYLLCLIENTEDITOR_API void Open();

	/** The Client Dev logs folder of this Unreal project. */
	[[nodiscard]] TRYLLCLIENTEDITOR_API FString LogsDirectory();

	/** %LOCALAPPDATA%\Tryll\projects */
	[[nodiscard]] TRYLLCLIENTEDITOR_API FString ProjectsDirectory();

	/**
	 * Absolute, '/' separators, no trailing separator, ASCII case folded: what the
	 * server's CanonicalProjectDir produces for an ordinary Windows path.
	 */
	[[nodiscard]] TRYLLCLIENTEDITOR_API FString CanonicalProjectDir(const FString& Dir);

	/** <slug>-<first 16 hex of SHA-256(dir)>; see ProjectKey.cpp. */
	[[nodiscard]] TRYLLCLIENTEDITOR_API FString ProjectKey(const FString& ProjectName, const FString& CanonicalDir);
}
