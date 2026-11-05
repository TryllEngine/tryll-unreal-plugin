// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllGraphDescription.h"
#include "TryllModelManifest.h"   // ETryllMemoryPlacement
#include "TryllKvCache.h"
#include "TryllMemorySnapshot.h"
#include "TryllModelInfo.h"
#include "TryllRuntimeSettings.h"
#include "TryllThrottle.h"
#include "TryllToolCall.h"
#include "TryllVariableDecl.h"
#include "TryllVoiceInput.h"
#include "TryllIntentClassification.h"
#include "TryllSubsystem.h"

class FTryllAgent;
class FTryllConnection;
class FTryllConnectionEvents;
class FTryllVoiceInput;

// ── Native multicast delegates (non-Blueprint) ─────────────────────────────────
// Slate widgets aren't UObjects and can't bind DECLARE_DYNAMIC_MULTICAST_DELEGATE
// (Blueprint) delegates. UTryllSubsystem's existing Blueprint delegates stay the
// Blueprint-facing surface, populated by subscribing to these and re-broadcasting.

DECLARE_MULTICAST_DELEGATE_OneParam(FTryllOnConnectionChangedNative, bool /*bConnected*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FTryllOnErrorNative, const FString& /*ErrorMessage*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FTryllOnAgentDestroyedNative, uint64 /*AgentId*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FTryllOnCreateSessionCompleteNative, bool /*bSuccess*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTryllOnSetSessionLocaleCompleteNative, const FString& /*Locale*/, bool /*bSuccess*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTryllOnListModelsCompleteNative, const TArray<FTryllModelInfo>& /*Models*/, bool /*bSuccess*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTryllOnGetMemoryConsumptionCompleteNative, const FTryllMemorySnapshot& /*Snapshot*/, bool /*bSuccess*/);
DECLARE_MULTICAST_DELEGATE_FourParams(FTryllOnDownloadProgressNative, const FString& /*ModelName*/, uint64 /*BytesDownloaded*/, uint64 /*TotalBytes*/, float /*Percent*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FTryllOnDownloadCompleteNative, const FString& /*ModelName*/, bool /*bSuccess*/, const FString& /*ErrorMessage*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTryllOnLoadModelCompleteNative, const FString& /*ModelName*/, bool /*bSuccess*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTryllOnUnloadModelCompleteNative, const FString& /*ModelName*/, bool /*bSuccess*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTryllOnDeleteModelCompleteNative, const FString& /*ModelName*/, bool /*bSuccess*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTryllOnCreateStringStorageCompleteNative, const FString& /*Name*/, bool /*bSuccess*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTryllOnDestroyStringStorageCompleteNative, const FString& /*Name*/, bool /*bSuccess*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FTryllOnCreateEmbeddedStringStorageCompleteNative, const FString& /*Name*/, int32 /*RecordCount*/, bool /*bSuccess*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTryllOnDestroyEmbeddedStringStorageCompleteNative, const FString& /*Name*/, bool /*bSuccess*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FTryllOnToolCallNative, uint64 /*AgentId*/, const FString& /*ToolName*/, const FString& /*ArgumentsJson*/);
DECLARE_MULTICAST_DELEGATE_FourParams(FTryllOnToolCallWithIdNative, uint64 /*AgentId*/, const FString& /*CallId*/, const FString& /*ToolName*/, const FString& /*ArgumentsJson*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTryllOnToolCallDetailedNative, uint64 /*AgentId*/, const FTryllToolCall& /*Call*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTryllOnIntentClassifiedNative, uint64 /*AgentId*/, const FTryllIntentClassification& /*Classification*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTryllOnIntentLlmClassifiedNative, uint64 /*AgentId*/, const FTryllIntentLlmClassification& /*Classification*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FTryllOnPausedNative, uint64 /*AgentId*/, const FString& /*NodeName*/, const FString& /*PendingExit*/);
/// PIE rebroadcast channel. UTryllSubsystem binds this and forwards to its
/// Blueprint OnPaused. Its IsBound() state is NOT a pause listener — every
/// delegating subsystem subscribes whether or not a game has bound OnPaused.
DECLARE_MULTICAST_DELEGATE_ThreeParams(FTryllOnPausedForwardNative, uint64 /*AgentId*/, const FString& /*NodeName*/, const FString& /*PendingExit*/);
DECLARE_MULTICAST_DELEGATE_FourParams(FTryllOnNodeEventNative, uint64 /*AgentId*/, const FString& /*NodeName*/, const FString& /*EventType*/, const TArray<FTryllNodeEventKeyValue>& /*KvPairs*/);

/**
 * Runtime-facing surface for the editor-only shared Tryll connection/session.
 * UTryllEditorSubsystem (TryllClientEditor) registers an implementation so
 * UTryllSubsystem (TryllClient) can share the editor connection in PIE without
 * linking the editor module. Per-message calls are not forwarded: the game
 * subsystem uses GetSharedConnection() and its own completions.
 */
class TRYLLCLIENT_API ITryllEditorBridge
{
public:
	virtual ~ITryllEditorBridge() = default;

	virtual void AcquireConnection() = 0;
	virtual void ReleaseConnection() = 0;

	virtual bool IsConnected() const = 0;
	virtual bool IsSessionReady() const = 0;
	virtual int64 GetSessionId() const = 0;

	virtual bool TryLaunch(EServerBuildVariant Variant, int32 KnownPort, int32 IdleShutdownTimeoutSeconds, FString& OutError) = 0;
	virtual int32 GetActualPort() const = 0;

	/**
	 * After TryLaunch replaced a running server, move an open shared connection
	 * onto the new port. No-op when nothing is connected.
	 */
	virtual void AdoptRelaunchedServer() = 0;

	/**
	 * Report whether this consumer's session-facing OnPaused is actually bound.
	 * Used so a PIE subscription is not mistaken for a listener.
	 * Returns an id to pass to UnregisterPauseListenerProbe.
	 */
	virtual int32 RegisterPauseListenerProbe(TFunction<bool()> Probe) = 0;
	virtual void UnregisterPauseListenerProbe(int32 ProbeId) = 0;
	/// Direct OnPausedNative listeners, or any registered probe that returns true.
	virtual bool HasSessionPauseListener() const = 0;

	/**
	 * Report whether this consumer's session-facing OnToolCall / OnToolCallWithId /
	 * OnToolCallDetailed is actually bound. The native tool-call delegates are the
	 * PIE forwarding channels and are always subscribed while delegating, so they
	 * cannot tell on their own. A bound listener makes the pause after a tool-call
	 * batch resumable (it can answer with ResumeWithToolResult(s)). Returns an id to
	 * pass to UnregisterToolCallListenerProbe.
	 */
	virtual int32 RegisterToolCallListenerProbe(TFunction<bool()> Probe) = 0;
	virtual void UnregisterToolCallListenerProbe(int32 ProbeId) = 0;
	/// Any registered tool-call probe that returns true.
	virtual bool HasSessionToolCallListener() const = 0;

	/** The editor's shared connection. Null when the editor has not connected. */
	virtual FTryllConnection* GetSharedConnection() = 0;
	/** Broadcast-class events for that connection. Null when there is no connection. */
	virtual FTryllConnectionEvents* GetConnectionEvents() = 0;
};

class TRYLLCLIENT_API FTryllEditorBridge
{
public:
	static void Register(ITryllEditorBridge* Bridge);
	static void Unregister(ITryllEditorBridge* Bridge);
	static ITryllEditorBridge* Get();
};
