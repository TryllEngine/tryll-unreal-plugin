// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

/**
 * Facts a release stamps into the plugin, read from the source tree when the plugin
 * runs from a development checkout instead. The release pipeline sets the .uplugin
 * VersionName from the repo-root VERSION file and copies docs/legal/LICENSE.md into the
 * plugin root (ci/release.toml [paths].licence_file); an in-tree plugin has neither, so
 * it reads them from the repo. A released plugin never looks outside its own folder.
 * The same rules as Unity's TryllVersionLookup. Plugin-internal.
 */
struct TRYLLCLIENT_API FTryllDevCheckout
{
	/** The .uplugin VersionName of a plugin no release has stamped. */
	static const TCHAR* const UnstampedVersion;

	/** The licence source, relative to the repo root. */
	static const TCHAR* const RepoLicenceFile;

	/** True when the TryllClient plugin's VersionName is still UnstampedVersion. */
	static bool IsUnstamped();

	/**
	 * The repo root above the unstamped TryllClient plugin, or empty for a released
	 * plugin or one outside a checkout. Read once per process.
	 */
	static FString RepoRoot();

	/** The repo-root VERSION (a version number), or empty when RepoRoot() is. */
	static FString RepoVersion();

	/** <repo>/docs/legal/LICENSE.md when RepoRoot() has one, else empty. */
	static FString RepoLicencePath();

	/** Walks up from Start to the first folder whose VERSION holds a version number; empty when none. */
	static FString FindRepoRoot(const FString& Start);

	/** The version number in Dir/VERSION, or empty when it is missing or holds something else. */
	static FString ReadVersionFile(const FString& Dir);
};
