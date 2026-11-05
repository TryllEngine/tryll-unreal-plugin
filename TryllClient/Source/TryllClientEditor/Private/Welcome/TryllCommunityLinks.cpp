// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "Welcome/TryllCommunityLinks.h"

#include "TryllEditorUiContract.h"

#include "HAL/PlatformProcess.h"

TFunction<void(const FString&)> FTryllCommunityLinks::DebugOpenUrlForTests;

namespace TryllCommunityLinksPrivate
{
template <int32 N>
TArray<TPair<FString, FString>> Filled(const TPair<const TCHAR*, const TCHAR*> (&All)[N])
{
	TArray<TPair<FString, FString>> Result;
	for (const TPair<const TCHAR*, const TCHAR*>& Link : All)
	{
		if (Link.Value != nullptr && *Link.Value != TEXT('\0'))
		{
			Result.Emplace(Link.Key, Link.Value);
		}
	}
	return Result;
}
} // namespace TryllCommunityLinksPrivate

TArray<TPair<FString, FString>> FTryllCommunityLinks::CommunityLinks()
{
	const TPair<const TCHAR*, const TCHAR*> All[] =
	{
		{ FTryllEditorUiContract::LinkDiscord, Discord },
		{ FTryllEditorUiContract::LinkYoutube, YouTube },
		{ FTryllEditorUiContract::LinkLinkedin, LinkedIn },
		{ FTryllEditorUiContract::LinkInstagram, Instagram },
		{ FTryllEditorUiContract::LinkTiktok, TikTok },
		{ FTryllEditorUiContract::LinkX, X },
		{ FTryllEditorUiContract::LinkGithub, GitHub },
		{ FTryllEditorUiContract::LinkWebsite, Website },
	};
	return TryllCommunityLinksPrivate::Filled(All);
}

TArray<TPair<FString, FString>> FTryllCommunityLinks::HelpLinks()
{
	// Discord is the Help tab's main button and a community chip, so it is not repeated here.
	const TPair<const TCHAR*, const TCHAR*> All[] =
	{
		{ FTryllEditorUiContract::LinkDocs, Docs },
		{ FTryllEditorUiContract::WelcomeGetStarted, GettingStarted },
		{ FTryllEditorUiContract::LinkEmail, Email },
		{ FTryllEditorUiContract::LinkReportIssue, ReportIssue },
	};
	return TryllCommunityLinksPrivate::Filled(All);
}

void FTryllCommunityLinks::OpenUrl(const FString& Url)
{
	if (DebugOpenUrlForTests)
	{
		DebugOpenUrlForTests(Url);
		return;
	}
	FPlatformProcess::LaunchURL(*Url, nullptr, nullptr);
}
