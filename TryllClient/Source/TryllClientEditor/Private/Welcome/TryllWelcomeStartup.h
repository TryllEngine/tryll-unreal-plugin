// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

class SWindow;

/**
 * Opens the Tryll Welcome tab once per editor session, after the main frame
 * exists, when this project needs it:
 * - the plugin version changed since the tab last opened at startup in this
 *   project (first install, or an update);
 * - the current licence is not accepted;
 * - the person ticked "Show this window at startup".
 * Never in commandlets, -unattended runs or automation tests. Mirrors Unity's
 * TryllWelcomeStartup. Settings are per project (GEditorPerProjectIni).
 */
struct TRYLLCLIENTEDITOR_API FTryllWelcomeStartup
{
	static constexpr const TCHAR* IniSection = TEXT("TryllWelcome");

	/** The startup rule, pure so tests can walk its truth table. */
	static bool ShouldOpen(bool bVersionChanged, bool bLicenceAccepted, bool bShowAtStartup, bool bNonInteractive);

	/** Per-project choice ("Show this window at startup"). Default off. */
	static bool GetShowAtStartup();
	static void SetShowAtStartup(bool bShow);

	/** Plugin version the tab last opened for at startup in this project, or empty. */
	static FString GetLastShownVersion();

	/** Hook the main frame. Also sends a pending install event at startup. */
	static void Register();
	static void Unregister();

private:
	static void SetLastShownVersion(const FString& Version);
	static void HandleMainFrameCreated(TSharedPtr<SWindow> RootWindow, bool bIsRunningStartupDialog);
	/** Next tick: the restored layout is in place, and invoking a tab is safe. */
	static void ScheduleOpen();
	static void MaybeOpen();
};
