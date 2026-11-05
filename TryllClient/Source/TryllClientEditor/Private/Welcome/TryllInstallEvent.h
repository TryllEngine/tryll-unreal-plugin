// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

/**
 * The plugin's one event: tryll.server.install. The server comes inside the
 * plugin zip, so the event goes out once per plugin version, after the licence
 * is accepted (and at editor startup when it was accepted but the event for this
 * version was not sent yet). Telemetry is part of the Tryll Runtime Licence and
 * always on. Same endpoint, project key and JSON shape as Unity's
 * TryllInstallTelemetry. One try, a short timeout, errors swallowed.
 * Properties are counts and versions only; no paths, names or project content.
 */
struct TRYLLCLIENTEDITOR_API FTryllInstallEvent
{
	static const TCHAR* const EventName;
	static const TCHAR* const Endpoint;
	static const TCHAR* const ProjectKey;

	struct FInfo
	{
		FString Result;
		FString Version;
		FString Variant;
		FString Source;   // zip: the server came with the plugin
		FString Channel;  // release | dev
		int64 Bytes = 0;
		double Seconds = 0.0;
	};

	/** The capture body, the same shape Unity sends with engine "unreal". */
	static FString BuildBody(const FInfo& Info, const FString& DistinctId, const FString& EngineVersion,
		const FDateTime& UtcNow);

	/** How many sent versions the store keeps; the oldest drop off first. */
	static constexpr int32 MaxSentVersions = 16;

	/**
	 * True when the licence is accepted and no event went out for PluginVersion yet.
	 * SentVersions is the stored ';'-joined list (FTryllLicence::GetLastInstallEventVersion),
	 * so projects on different plugin versions each send once, in any order.
	 */
	static bool IsDue(bool bLicenceAccepted, const FString& PluginVersion, const FString& SentVersions);

	/** The stored list with Version added last, keeping the newest MaxSentVersions. */
	static FString AddSentVersion(const FString& SentVersions, const FString& Version);

	/** This plugin's event: result ok, variant full, source zip, bytes and duration 0. */
	static FInfo MakePluginInfo();

	/** TryllClient VersionName, or empty when the plugin is not found. */
	static FString PluginVersion();

	/** "dev" when TryllDeveloperTools.marker is in the plugin folder, else "release". */
	static FString Channel();

	/** Sends the event if IsDue, and records the version first so it goes out once. Game thread. */
	static void SendIfDue();

	/** Listen to FTryllLicence::OnChanged: an accepted licence sends the event. */
	static void Register();
	static void Unregister();

	/** Test seam: receives the JSON body instead of the HTTP post. */
	static TFunction<void(const FString& Body)> DebugSenderForTests;
};
