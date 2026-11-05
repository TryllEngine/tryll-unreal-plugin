// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

/**
 * The person's acceptance of the Tryll Runtime Licence, plus the plugin's own install id.
 *
 * The licence is LICENSE.md in the TryllClient plugin folder; an unstamped in-tree plugin
 * without one reads docs/legal/LICENSE.md from the repo (FTryllDevCheckout). Its version is a hash of
 * the text, so any change to the text asks again. Without the file the version is
 * "evaluation-draft" and Accept still works.
 *
 * Stored user-global with FPlatformMisc stored values (store "Tryll", section
 * "Welcome"), so a choice follows the person across projects and engine versions.
 * Game thread only.
 */
struct TRYLLCLIENT_API FTryllLicence
{
	/** Version used when the plugin has no LICENSE.md. */
	static const TCHAR* const EvaluationDraftVersion;

	/** <TryllClient plugin BaseDir>/LICENSE.md, else the repo's licence in a development checkout, or the test override. */
	static FString LicenceFilePath();
	static bool HasLicenceFile();
	/** The licence text with line endings as "\n". Empty when the file is missing. */
	static FString LoadLicenceText();
	/** "sha1:<16 hex>" of the text (no BOM, LF line endings, no trailing whitespace), or EvaluationDraftVersion. */
	static FString CurrentVersion();
	/** True when the stored accepted version equals CurrentVersion(). */
	static bool IsAccepted();
	/** Accept stores CurrentVersion() and the UTC time; withdraw removes both. Broadcasts OnChanged. */
	static void SetAccepted(bool bAccepted);
	/** Random id made on first use and never derived from the machine. */
	static FString GetInstallId();
	/**
	 * The plugin versions an install event went out for, ';'-joined (an older
	 * single version reads as a list of one), or empty. FTryllInstallEvent owns the format.
	 */
	static FString GetLastInstallEventVersion();
	static void SetLastInstallEventVersion(const FString& Version);
	/** Fires after every SetAccepted. Game thread. */
	static FSimpleMulticastDelegate& OnChanged();

	/** Test seams: in-memory store (never touch the real one) and a substitute licence file. */
	static void DebugUseInMemoryStoreForTests(bool bEnable);
	static bool IsUsingInMemoryStoreForTests();
	/** A path that does not exist stands for a plugin without LICENSE.md. Empty restores the real file. */
	static void DebugSetLicenceFileForTests(const FString& PathOrEmpty);
	/** Back to the real store and the real file, with the in-memory store emptied. */
	static void DebugResetForTests();
};
