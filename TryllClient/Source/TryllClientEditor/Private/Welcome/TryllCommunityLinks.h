// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllDocs.h"

/**
 * Every outside link the Welcome window shows, in one place. The same URLs as
 * Unity's TryllCommunityLinks. An empty URL hides its button, so a channel that
 * does not exist yet costs nothing. Labels are user-visible copy and live in
 * tryll/schema/editor_ui.json (FTryllEditorUiContract).
 */
struct TRYLLCLIENTEDITOR_API FTryllCommunityLinks
{
	static constexpr const TCHAR* Website = TEXT("https://tryllengine.com");
	static constexpr const TCHAR* Docs = TryllDocs::BaseUrl;
	static constexpr const TCHAR* GettingStarted = TryllDocs::GettingStartedUnreal;
	/** What telemetry the server sends; the Welcome window's "Telemetry" button. */
	static constexpr const TCHAR* Telemetry = TryllDocs::Telemetry;
	/** The licence page of this version's docs; the Welcome window's "Read the licence" button. */
	static constexpr const TCHAR* Licence = TryllDocs::Licence;
	/** How to report a bug; the Help & Community tab's "Report a bug" button. */
	static constexpr const TCHAR* ReportBug = TryllDocs::ReportABug;
	static constexpr const TCHAR* GitHub = TEXT("https://github.com/TryllEngine");

	static constexpr const TCHAR* Discord = TEXT("https://discord.com/invite/DTKycjPpzs");
	static constexpr const TCHAR* YouTube = TEXT("https://www.youtube.com/@Tryll_engine");
	static constexpr const TCHAR* LinkedIn = TEXT("https://www.linkedin.com/company/tryllengine/posts/");
	static constexpr const TCHAR* Instagram = TEXT("https://www.instagram.com/tryll_engine");
	static constexpr const TCHAR* TikTok = TEXT("https://www.tiktok.com/@tryll_engine");
	static constexpr const TCHAR* X = TEXT("");

	static constexpr const TCHAR* Email = TEXT("mailto:team@tryllengine.com");
	/** Issue tracker of the public plugin repo ([unreal.export].repo_url in ci/release.toml). */
	static constexpr const TCHAR* ReportIssue = TEXT("https://github.com/TryllEngine/tryll-unreal-plugin/issues");

	/** (label, url) for every social channel, in Unity's order; empty URLs are dropped. */
	static TArray<TPair<FString, FString>> CommunityLinks();

	/** (label, url) for docs and support, in Unity's order; empty URLs are dropped. */
	static TArray<TPair<FString, FString>> HelpLinks();

	/** Opens an outside link in the browser, or hands it to the test seam. */
	static void OpenUrl(const FString& Url);

	/** Test seam: receives the URL instead of the browser. */
	static TFunction<void(const FString&)> DebugOpenUrlForTests;

	/** Opens a local file in the OS default app, or hands it to the test seam. */
	static void OpenFile(const FString& Path);

	/** Test seam: receives the path instead of the OS. */
	static TFunction<void(const FString&)> DebugOpenFileForTests;
};
