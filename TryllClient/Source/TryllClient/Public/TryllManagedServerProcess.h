// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"

/**
 * Plain (non-UObject) helper that spawns tryll_server.exe as a managed child,
 * draining its stdout to the log until released. Shared by
 * UTryllEditorSubsystem and UTryllSubsystem's packaged-build path —
 * those differ in when/which port to spawn on, not in how.
 */
class TRYLLCLIENT_API FTryllManagedServerProcess
{
public:
	FTryllManagedServerProcess() = default;
	~FTryllManagedServerProcess();

	FTryllManagedServerProcess(const FTryllManagedServerProcess&) = delete;
	FTryllManagedServerProcess& operator=(const FTryllManagedServerProcess&) = delete;

	/** Environment variable: "tryll-dev" or "client-dev". Wins over the developer override. */
	static const TCHAR* const ServerModeEnvVar;
	static const TCHAR* const TryllDevMode;
	static const TCHAR* const ClientDevMode;

	/** Canonical mode for Value, or empty when it is not a mode a plugin may launch. */
	static FString NormalizeLaunchMode(const FString& Value);

	/**
	 * Mode for the next spawn: ServerModeEnvVar, then the developer override, then
	 * client-dev. tryll-dev spawns also pass --telemetry-level off.
	 */
	static FString ResolveLaunchMode();

	/**
	 * Set by the monorepo-only editor developer menu (compiled only when
	 * TRYLL_DEVELOPER_TOOLS=1). Empty clears it. Released plugins never call this.
	 */
	static void SetDeveloperModeOverride(const FString& Mode);
	static const FString& GetDeveloperModeOverride() { return DeveloperModeOverride; }

	/**
	 * Logged when the licence gate stops a launch. Same text as
	 * FTryllEditorUiContract::ServerNeedsLicence, which this module cannot see;
	 * a unit test keeps the two equal.
	 */
	static const TCHAR* const ServerNeedsLicenceMessage;

	/**
	 * Licence gate for a server the plugin starts. Blocks only when the licence is
	 * not accepted and someone could accept it: never for client-prod (players),
	 * tryll-dev, or a non-interactive run.
	 */
	static bool ShouldBlockLaunch(const FString& Mode, bool bLicenceAccepted, bool bNonInteractive);

	/** Nobody can click Accept: a commandlet, -unattended, or an automation test. */
	static bool IsNonInteractiveRun();

	/** ShouldBlockLaunch for this process. Always false in packaged builds. */
	static bool IsLaunchBlockedByLicence();

	/** Severity of one server stdout line, read from its `[time] [level] message` prefix. */
	enum class ELineSeverity : uint8
	{
		Normal,  ///< info/debug/trace, or a line without the prefix (continuations, banners)
		Warning, ///< [warning]
		Error,   ///< [error] or [critical]
	};
	static ELineSeverity ClassifyLine(const FString& Line);

	bool IsRunning();

	/** True after Spawn saw the child accept TCP. False while still warming. */
	bool IsObservedReady() const { return bObservedReady; }

	/**
	 * Spawns ExePath with "--port Port --idle-shutdown-timeout Seconds
	 * --llama-backend LlamaCppBackend" and waits up to ReadinessTimeoutSeconds
	 * for its TCP port to start accepting.
	 *
	 * Returns true when the launch should proceed to connecting — either the port is already
	 * accepting, OR the readiness wait elapsed but the process is *still alive* (a cold start
	 * still warming up). In the latter case the server is deliberately left running, not killed:
	 * killing an almost-ready server discards the warmup and forces a cold re-spawn — the root
	 * cause of "first launch hangs, relaunch works". The caller connects anyway and the
	 * connection worker retries until the server is ready.
	 *
	 * Returns false only for a genuine launch failure: the process could not be spawned, or it
	 * exited on its own before the port opened. OutError is then populated with a clear reason,
	 * and bOutPortConflict is set to true iff the process exited with Tryll::ServerExitPortInUse
	 * (3), so callers can special-case a bind collision (retry on a different port).
	 */
	bool Spawn(const FString& ExePath, int32 Port, int32 IdleShutdownTimeoutSeconds,
		double ReadinessTimeoutSeconds, const FString& LlamaCppBackend,
		FString& OutError, bool& bOutPortConflict);

	/** Releases the handle without killing the process (normal teardown). */
	void ReleaseHandle();

	/** Force-terminate then release (test/CI cleanup only). */
	void TerminateAndRelease();

private:
	friend class UTryllEditorSubsystem;

	bool DrainOutput(float DeltaTime);

	static FString DeveloperModeOverride;

	/// Editor-test-only extra CLI args (fault injection). Not a public Spawn parameter.
	FString DebugExtraCliArgs;

	/// Editor-test-only launch isolation: absolute --storage-root / --save-root in
	/// place of the project-derived folders, and no custom catalog or
	/// Tryll/models.d, so the host project's state never reaches a test server.
	FString DebugStorageRoot;
	FString DebugSaveRoot;
	bool bDebugNoUserCatalogs = false;

	FProcHandle ServerProcess;
	bool        bObservedReady = false;
	void*       StdoutRead  = nullptr;
	void*       StdoutWrite = nullptr;
	FTSTicker::FDelegateHandle DrainTickerHandle;
	FString     LogLineBuffer;
};
