// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDevCheckout.h"

#include "Interfaces/IPluginManager.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// Named, not anonymous: UBT unity builds merge this file with others in the module.
namespace TryllDevCheckoutPrivate
{
// A plugin inside the repo sits at <repo>/UnrealPlugin/<Project>/Plugins/TryllClient.
constexpr int32 MaxRepoDepth = 6;

TSharedPtr<IPlugin> ClientPlugin()
{
	return IPluginManager::Get().FindPlugin(TEXT("TryllClient"));
}
} // namespace TryllDevCheckoutPrivate

const TCHAR* const FTryllDevCheckout::UnstampedVersion = TEXT("0.0.0");
const TCHAR* const FTryllDevCheckout::RepoLicenceFile = TEXT("docs/legal/LICENSE.md");

bool FTryllDevCheckout::IsUnstamped()
{
	const TSharedPtr<IPlugin> Plugin = TryllDevCheckoutPrivate::ClientPlugin();
	return Plugin.IsValid() && Plugin->GetDescriptor().VersionName == UnstampedVersion;
}

FString FTryllDevCheckout::RepoRoot()
{
	static const FString Root = []()
	{
		const TSharedPtr<IPlugin> Plugin = TryllDevCheckoutPrivate::ClientPlugin();
		return Plugin.IsValid() && IsUnstamped() ? FindRepoRoot(Plugin->GetBaseDir()) : FString();
	}();
	return Root;
}

FString FTryllDevCheckout::RepoVersion()
{
	const FString Root = RepoRoot();
	return Root.IsEmpty() ? FString() : ReadVersionFile(Root);
}

FString FTryllDevCheckout::RepoLicencePath()
{
	const FString Root = RepoRoot();
	if (Root.IsEmpty())
	{
		return FString();
	}
	const FString Path = FPaths::Combine(Root, RepoLicenceFile);
	return FPaths::FileExists(Path) ? Path : FString();
}

FString FTryllDevCheckout::FindRepoRoot(const FString& Start)
{
	FString Dir = Start.IsEmpty() ? FString() : FPaths::ConvertRelativePathToFull(Start);
	for (int32 Depth = 0; Depth <= TryllDevCheckoutPrivate::MaxRepoDepth && !Dir.IsEmpty(); ++Depth)
	{
		if (!ReadVersionFile(Dir).IsEmpty())
		{
			return Dir;
		}
		const FString Parent = FPaths::GetPath(Dir);
		if (Parent == Dir)
		{
			break;
		}
		Dir = Parent;
	}
	return FString();
}

FString FTryllDevCheckout::ReadVersionFile(const FString& Dir)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *FPaths::Combine(Dir, TEXT("VERSION"))))
	{
		return FString();
	}
	Text.TrimStartAndEndInline();
	const FRegexPattern VersionNumber(TEXT("^[0-9]+\\.[0-9]+\\.[0-9]+([\\-+][0-9A-Za-z.\\-+]*)?$"));
	FRegexMatcher Matcher(VersionNumber, Text);
	return Matcher.FindNext() ? Text : FString();
}
