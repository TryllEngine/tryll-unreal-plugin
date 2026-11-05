// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllClientModule.h"

// IPluginManager.h itself includes PluginDescriptor.h, which defines FPluginDescriptor
// (and its VersionName field) -- no second include needed.
#include "Interfaces/IPluginManager.h"

DEFINE_LOG_CATEGORY(LogTryll);

void FTryllClientModule::StartupModule()
{
	// Plugin version, first Tryll line in every Unreal log. Read from the .uplugin's
	// VersionName, which the release pipeline stamps from the repo-root VERSION file
	// (ci/scripts/release.py, _stamp_json_version) -- so a shipped build reports its
	// real version while an in-tree dev build reports the placeholder 0.0.0.
	// Logged here rather than in UTryllSubsystem::Initialize so it appears once at
	// plugin load, and crucially even when no connection is ever made: a "cannot
	// connect" ticket is exactly the case where the server identity line is absent.
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("TryllClient"));
	const FString Version = Plugin.IsValid() ? Plugin->GetDescriptor().VersionName : TEXT("unknown");
	UE_LOG(LogTryll, Log, TEXT("TryllClient plugin v%s started"), *Version);
}

void FTryllClientModule::ShutdownModule()
{
	UE_LOG(LogTryll, Log, TEXT("TryllClient module shut down"));
}

IMPLEMENT_MODULE(FTryllClientModule, TryllClient)
