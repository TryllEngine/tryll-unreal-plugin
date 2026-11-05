// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "IPIEAuthorizer.h"
#include "Runtime/Launch/Resources/Version.h"

class UTryllEditorSubsystem;
class FTryllModelDownloadGuard;
class SNotificationItem;

/**
 * feat-011 PIE guard, gated through the engine's IPIEAuthorizer extension point
 * (Editor/UnrealEd/Public/IPIEAuthorizer.h) rather than FEditorDelegates::BeginPIE.
 * PlayLevel.cpp checks every registered IPIEAuthorizer between PreBeginPIE and BeginPIE;
 * if any denies, the engine calls CancelRequestPlaySession() and returns before the PIE
 * world is ever created. That means a denied attempt never spins up the world at all —
 * unlike the original BeginPIE + RequestEndPlayMap approach (which had to end a session
 * that had already started, and which turned out to make the engine reenter BeginPIE
 * reentrantly), this one is a clean, engine-supported veto.
 *
 * Routes through UTryllEditorSubsystem — the same shared connection/session/agent-map used
 * by the chat panel and model manager panel — rather than opening a private connection and
 * launching its own server process. A second independent connection here would race the
 * shared subsystem's own in-flight ListModels/DownloadModel requests against this guard's.
 *
 * RequestPIEPermission() denies (returns false) while a check/download is in flight, and
 * once it succeeds, sets bVerifiedReady and calls RequestPlaySession() again — which will
 * hit this same authorizer and be allowed straight through, since bVerifiedReady is now true.
 *
 * The verified-ready state is cached for the life of the editor process (there is no
 * domain-reload equivalent in Unreal) — Invalidate() must be called whenever the manifest
 * changes or a model's files change outside this guard's own download.
 */
class FTryllPIEModelGuard : public IPIEAuthorizer
{
public:
	static void Register();
	static void Unregister();

	/** Call after a manifest edit or model deletion so the next Play re-checks. */
	static void Invalidate() { bVerifiedReady = false; }

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 8)
	// UE 5.8+: the bool/FString overload is deprecated+final. The silent query must never
	// block or start work; the blocking request runs the model check.
	virtual TValueOrError<bool, FText> IsPIEAuthorizedInternal(bool bIsSimulateInEditor) const override;
	virtual TValueOrError<bool, FText> RequestPIEPermissionInternal(bool bIsSimulateInEditor) const override;
#else
	virtual bool RequestPIEPermission(bool bIsSimulateInEditor, FString& OutReason) const override;
#endif

private:
	bool RequestPIEPermissionImpl(FString& OutReason) const;

	static void RunGuardThenResume();
	static void Cleanup();

	// Editor toast shown for the life of a check so a denied Play doesn't look like a
	// silent cancel. ShowOrUpdate creates (or refreshes) a pending notification;
	// Dismiss expires it (CompletionState: None=0 / Pending=1 / Success=2 / Fail=3 —
	// matches SNotificationItem::ECompletionState). Idempotent when nothing is shown.
	static void ShowOrUpdateProgressNotification(const FText& Text);
	static void DismissProgressNotification(int32 CompletionState, const FText& FinalText = FText::GetEmpty());

	// Disarms the connect/session watchdog (below). Safe to call when unarmed.
	static void DisarmWatchdog();

	// Subscribed once to the shared UTryllEditorSubsystem's native delegates (they outlive
	// any single guard run, so re-subscribing on every Play attempt would stack duplicates).
	// Both no-op immediately if !bCheckInProgress: once verified (or between Play attempts),
	// a reconnect triggered by someone else (e.g. PIE's own UTryllSubsystem re-acquiring
	// after ReleaseHeldConnection() below) must not kick off a whole new check-and-release
	// cycle — that self-sustaining loop was confirmed live via debugger before this guard
	// existed. Intermediate ConnectFailed while HasConnectRetriesRemaining() is also ignored
	// so accept-late server warm-up can finish inside the retry budget / watchdog.
	static void HandleConnectionChanged(bool bConnected);
	static void HandleCreateSessionComplete(bool bSuccess);
	static void HandleError(const FString& ErrorMessage);
	static void OnModelsReady(bool bSuccess, FString Error);

	// On success, the connection acquisition made in RunGuardThenResume() is deliberately
	// held past OnModelsReady() rather than released immediately — PIE's own UTryllSubsystem
	// is about to acquire its own ref via Initialize(), and releasing first would force it to
	// reconnect from scratch. PostPIEStarted fires only after UTryllSubsystem::Initialize()
	// has already run (confirmed against PlayLevel.cpp: CreateNewPlayInEditorInstance ->
	// OnLoginPIEComplete_Deferred -> CreateInnerProcessPIEGameInstance ->
	// GameInstance->InitializeForPlayInEditor(), all synchronous, before StartPlayInEditorSession
	// reaches PostPIEStarted.Broadcast()), so releasing there is safe. CancelPIE/EndPIE are the
	// fallback release triggers for when PIE never actually starts (denied by another
	// IPIEAuthorizer, or fails to initialize) so the held ref can't leak indefinitely.
	static void OnPIEStartedOrEnded(bool bIsSimulating);
	static void OnPIECancelled();
	static void ReleaseHeldConnection();

	static bool bVerifiedReady;
	static bool bCheckInProgress;
	static bool bSubscribedToSubsystem;
	static bool bAcquiredConnection;

	/// Set by HandleError() and consumed by whichever paired completion handler fires
	/// immediately after it — see STryllChatPanel::LastErrorMessage for the same idiom.
	static FString LastErrorMessage;

	static FTSTicker::FDelegateHandle TickerHandle;
	// One-shot watchdog covering only the connect + CreateSession stage. Armed in
	// RunGuardThenResume, disarmed the moment a session outcome arrives
	// (HandleCreateSessionComplete) so it is never live during the model-download
	// stage — that stage can legitimately run for minutes and shows its own
	// cancelable progress dialog. Guarantees a wedged pre-session server surfaces a
	// clear error instead of an indefinite Play veto.
	static FTSTicker::FDelegateHandle WatchdogTickerHandle;
	static FDelegateHandle PostPIEStartedHandle;
	static FDelegateHandle CancelPIEHandle;
	static FDelegateHandle EndPIEHandle;

	static UTryllEditorSubsystem* EditorSubsystem;
	static TSharedPtr<FTryllModelDownloadGuard> ModelGuard;
	static TWeakPtr<SNotificationItem> ProgressNotification;

	static FTryllPIEModelGuard Instance;
};
