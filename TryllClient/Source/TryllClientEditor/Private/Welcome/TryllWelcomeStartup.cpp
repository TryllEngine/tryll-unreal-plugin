// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "Welcome/TryllWelcomeStartup.h"

#include "Welcome/TryllInstallEvent.h"
#include "TryllClientEditorModule.h"
#include "TryllLicence.h"
#include "TryllManagedServerProcess.h"

#include "Containers/Ticker.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Interfaces/IMainFrameModule.h"
#include "Misc/ConfigCacheIni.h"
#include "Modules/ModuleManager.h"

// Named, not anonymous: UBT unity builds merge this file with others in the module.
namespace TryllWelcomeStartupPrivate
{
const TCHAR* const ShowAtStartupKey = TEXT("bShowAtStartup");
const TCHAR* const LastShownVersionKey = TEXT("LastShownVersion");

/** Decided once per editor session. */
bool bCheckedThisSession = false;
FDelegateHandle MainFrameHandle;
FTSTicker::FDelegateHandle OpenTickerHandle;
} // namespace TryllWelcomeStartupPrivate

bool FTryllWelcomeStartup::ShouldOpen(
	bool bVersionChanged, bool bLicenceAccepted, bool bShowAtStartup, bool bNonInteractive)
{
	if (bNonInteractive)
	{
		return false;
	}
	return bVersionChanged || !bLicenceAccepted || bShowAtStartup;
}

bool FTryllWelcomeStartup::GetShowAtStartup()
{
	bool bShow = false;
	if (GConfig != nullptr)
	{
		GConfig->GetBool(IniSection, TryllWelcomeStartupPrivate::ShowAtStartupKey, bShow, GEditorPerProjectIni);
	}
	return bShow;
}

void FTryllWelcomeStartup::SetShowAtStartup(bool bShow)
{
	if (GConfig == nullptr)
	{
		return;
	}
	GConfig->SetBool(IniSection, TryllWelcomeStartupPrivate::ShowAtStartupKey, bShow, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

FString FTryllWelcomeStartup::GetLastShownVersion()
{
	FString Version;
	if (GConfig != nullptr)
	{
		GConfig->GetString(IniSection, TryllWelcomeStartupPrivate::LastShownVersionKey, Version, GEditorPerProjectIni);
	}
	return Version;
}

void FTryllWelcomeStartup::SetLastShownVersion(const FString& Version)
{
	if (GConfig == nullptr)
	{
		return;
	}
	GConfig->SetString(IniSection, TryllWelcomeStartupPrivate::LastShownVersionKey, *Version, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

void FTryllWelcomeStartup::Register()
{
	using namespace TryllWelcomeStartupPrivate;
	// Nobody can click Accept in these runs, and CI must not open tabs or send events.
	if (FTryllManagedServerProcess::IsNonInteractiveRun() || bCheckedThisSession || MainFrameHandle.IsValid())
	{
		return;
	}
	IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
	if (MainFrame.IsWindowInitialized())
	{
		ScheduleOpen();
		return;
	}
	MainFrameHandle = MainFrame.OnMainFrameCreationFinished().AddStatic(&FTryllWelcomeStartup::HandleMainFrameCreated);
}

void FTryllWelcomeStartup::Unregister()
{
	using namespace TryllWelcomeStartupPrivate;
	if (MainFrameHandle.IsValid())
	{
		if (IMainFrameModule* MainFrame = FModuleManager::GetModulePtr<IMainFrameModule>(TEXT("MainFrame")))
		{
			MainFrame->OnMainFrameCreationFinished().Remove(MainFrameHandle);
		}
		MainFrameHandle.Reset();
	}
	if (OpenTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(OpenTickerHandle);
		OpenTickerHandle.Reset();
	}
}

void FTryllWelcomeStartup::HandleMainFrameCreated(TSharedPtr<SWindow> /*RootWindow*/, bool bIsRunningStartupDialog)
{
	// The project browser is not the editor yet; the real main frame follows it.
	if (bIsRunningStartupDialog)
	{
		return;
	}
	ScheduleOpen();
}

void FTryllWelcomeStartup::ScheduleOpen()
{
	using namespace TryllWelcomeStartupPrivate;
	if (OpenTickerHandle.IsValid())
	{
		return;
	}
	OpenTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([](float) -> bool
		{
			TryllWelcomeStartupPrivate::OpenTickerHandle.Reset();
			MaybeOpen();
			return false;
		}),
		0.0f);
}

void FTryllWelcomeStartup::MaybeOpen()
{
	using namespace TryllWelcomeStartupPrivate;
	if (bCheckedThisSession || FTryllManagedServerProcess::IsNonInteractiveRun())
	{
		return;
	}
	bCheckedThisSession = true;

	// A licence accepted earlier (another project, another engine) still owes
	// this plugin version its install event.
	FTryllInstallEvent::SendIfDue();

	const FString Version = FTryllInstallEvent::PluginVersion();
	if (!ShouldOpen(
			GetLastShownVersion() != Version,
			FTryllLicence::IsAccepted(),
			GetShowAtStartup(),
			FTryllManagedServerProcess::IsNonInteractiveRun()))
	{
		return;
	}
	if (!FSlateApplication::IsInitialized()
		|| !FGlobalTabmanager::Get()->HasTabSpawner(FTryllClientEditorModule::WelcomeTabName))
	{
		return;
	}
	SetLastShownVersion(Version);
	FGlobalTabmanager::Get()->TryInvokeTab(FTryllClientEditorModule::WelcomeTabName);
}
