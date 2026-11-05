// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

/**
 * Finds the newest release of the public plugin repo without the GitHub REST API (60
 * anonymous calls an hour per IP): <repo>/releases/latest redirects to
 * .../releases/tag/vX.Y.Z, and the tag in the final URL is the answer. The request carries
 * no data about the user or project. The Unreal port of Unity's TryllUpdateCheck.
 *
 * The repo comes from TryllRelease.json in the TryllClient plugin folder, which the release
 * pipeline (stage R) writes; a development checkout has none, so updates show as unavailable.
 */
struct TRYLLCLIENTEDITOR_API FTryllUpdateCheck
{
	static constexpr const TCHAR* ReleaseInfoFile = TEXT("TryllRelease.json");

	/** The version in a ".../releases/tag/vX.Y.Z" URL, without the leading 'v', or empty. */
	static FString ParseTagUrl(const FString& Url);

	/**
	 * True when Candidate is a newer release than Current. Compares major.minor.patch;
	 * a pre-release (-rc.1) sorts before its release. Unparseable input is never newer.
	 */
	static bool IsNewer(const FString& Candidate, const FString& Current);

	/** The public plugin repo URL from TryllRelease.json, or empty. */
	static FString ReleasesRepo();

	/** Looks up the newest release; OnDone gets its version, or empty on any failure. Game thread. */
	static void LatestVersionAsync(const FString& RepoUrl, TFunction<void(const FString&)> OnDone);

	/** Test seam: replaces the HTTP lookup. */
	static TFunction<void(const FString&, TFunction<void(const FString&)>)> DebugLatestVersionForTests;
};
