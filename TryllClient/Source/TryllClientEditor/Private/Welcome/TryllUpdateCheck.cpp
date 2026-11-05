// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "Welcome/TryllUpdateCheck.h"

#include "Welcome/TryllCommunityWidgets.h"

#include "Dom/JsonObject.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

TFunction<void(const FString&, TFunction<void(const FString&)>)> FTryllUpdateCheck::DebugLatestVersionForTests;

// Named, not anonymous: UBT unity builds merge this file with others in the module.
namespace TryllUpdateCheckPrivate
{
/** "1.2.3-rc.1+abc" -> {1,2,3}, "rc.1". False when there are not exactly three numbers. */
bool ParseVersion(const FString& Version, int32 (&Parts)[3], FString& Pre)
{
	Pre.Reset();
	FString Core = Version;
	Core.RemoveFromStart(TEXT("v"));
	int32 Index = INDEX_NONE;
	if (Core.FindChar(TEXT('+'), Index))
	{
		Core.LeftInline(Index);
	}
	if (Core.FindChar(TEXT('-'), Index))
	{
		Pre = Core.Mid(Index + 1);
		Core.LeftInline(Index);
	}
	TArray<FString> Fields;
	Core.ParseIntoArray(Fields, TEXT("."), /*InCullEmpty=*/false);
	if (Fields.Num() != 3)
	{
		return false;
	}
	for (int32 i = 0; i < 3; ++i)
	{
		if (Fields[i].IsEmpty() || !Fields[i].IsNumeric())
		{
			return false;
		}
		Parts[i] = FCString::Atoi(*Fields[i]);
	}
	return true;
}
} // namespace TryllUpdateCheckPrivate

FString FTryllUpdateCheck::ParseTagUrl(const FString& Url)
{
	static const FString Marker = TEXT("/releases/tag/");
	const int32 At = Url.Find(Marker, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	if (At == INDEX_NONE)
	{
		return FString();
	}
	FString Tag = Url.Mid(At + Marker.Len());
	Tag.RemoveFromStart(TEXT("v"));
	if (Tag.IsEmpty() || !FChar::IsDigit(Tag[0]))
	{
		return FString();
	}
	for (const TCHAR C : Tag)
	{
		if (!(FChar::IsAlnum(C) || C == TEXT('.') || C == TEXT('-') || C == TEXT('+')))
		{
			return FString();
		}
	}
	return Tag;
}

bool FTryllUpdateCheck::IsNewer(const FString& Candidate, const FString& Current)
{
	int32 A[3] = {}, B[3] = {};
	FString PreA, PreB;
	if (!TryllUpdateCheckPrivate::ParseVersion(Candidate, A, PreA)
		|| !TryllUpdateCheckPrivate::ParseVersion(Current, B, PreB))
	{
		return false;
	}
	for (int32 i = 0; i < 3; ++i)
	{
		if (A[i] != B[i])
		{
			return A[i] > B[i];
		}
	}
	if (PreA == PreB)
	{
		return false;
	}
	if (PreA.IsEmpty())
	{
		return true;   // 1.2.0 > 1.2.0-rc.1
	}
	if (PreB.IsEmpty())
	{
		return false;
	}
	return FCString::Strcmp(*PreA, *PreB) > 0;
}

FString FTryllUpdateCheck::ReleasesRepo()
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("TryllClient"));
	if (!Plugin.IsValid())
	{
		return FString();
	}
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *FPaths::Combine(Plugin->GetBaseDir(), ReleaseInfoFile)))
	{
		return FString();
	}
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	FString Repo;
	if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
	{
		Root->TryGetStringField(TEXT("releases_repo"), Repo);
	}
	return Repo;
}

void FTryllUpdateCheck::LatestVersionAsync(const FString& RepoUrl, TFunction<void(const FString&)> OnDone)
{
	if (DebugLatestVersionForTests)
	{
		DebugLatestVersionForTests(RepoUrl, MoveTemp(OnDone));
		return;
	}
	if (RepoUrl.IsEmpty())
	{
		OnDone(FString());
		return;
	}
	FString Url = RepoUrl;
	while (Url.EndsWith(TEXT("/")))
	{
		Url.LeftChopInline(1);
	}
	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(TEXT("HEAD"));
	Request->SetURL(Url + TEXT("/releases/latest"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("TryllUnreal/") + TryllCommunityWidgets::ClientVersion());
	Request->SetTimeout(10.0f);
	// Completion runs on the game thread. The HTTP module follows the redirect; the
	// effective URL is the release tag page.
	Request->OnProcessRequestComplete().BindLambda(
		[OnDone = MoveTemp(OnDone)](FHttpRequestPtr /*Req*/, FHttpResponsePtr Response, bool bConnected)
		{
			const bool bOk = bConnected && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode());
			OnDone(bOk ? ParseTagUrl(Response->GetEffectiveURL()) : FString());
		});
	Request->ProcessRequest();
}
