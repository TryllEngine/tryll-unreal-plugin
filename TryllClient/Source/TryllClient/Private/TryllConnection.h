// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/ThreadSafeBool.h"
#include "Containers/Queue.h"

#include "TryllError.h"
#include "TryllVisemeEvent.h"
#include "TryllClientObserver.h"
#include "TryllDiagnosticsDemand.h"
#include "FTryllErrorContext.h"
#include "TryllConnectionEvents.h"
#include "TryllPushRouter.h"

#include "messages_generated.h"

#include <atomic>
#include <cstdint>
#include <deque>
#include <mutex>
#include <unordered_map>

class FSocket;
class ISocketSubsystem;

// ── Events (socket worker → game thread) ─────────────────────────────────────
// Request replies are completions on this same FIFO, not event-type arms.
// What remains is connection state plus the pushes the router still delivers.

enum class ETryllConnEventType : uint8
{
	Connected,
	ConnectFailed,
	Disconnected,
	Completion,

	AnswerText,
	TurnComplete,

	NodeEvent,
	TranscriptUpdate,

	TtsAudioFormat,
	TtsAudio,
	TtsViseme,
};

struct FTryllConnEvent
{
	ETryllConnEventType  Type       = ETryllConnEventType::ConnectFailed;
	std::uint64_t        RequestId  = 0;
	std::uint64_t        AgentId    = 0;
	uint32               Code       = 0;
	FString              Message;
	FString              Text;
	bool                 bFlag      = false;
	bool                 bFlag2     = false;
	uint8                TurnStatus = 0;
	uint32               TokensGenerated = 0;
	TArray<TPair<FString, FString>> KvPairs;

	FString              ServerExePath;
	FString              ServerBuildConfig;
	FString              ServerMode;
	FString              ServerVersion;
	FString              ServerRunId;
	bool                 bServerManaged           = false;
	uint32               ServerIdleTimeoutSeconds = 0;

	std::uint64_t        VoiceInputId    = 0;
	uint8                TranscriptKind  = 0;
	uint32               AudioMsConsumed = 0;

	uint32               TtsSampleRate    = 0;
	uint16               TtsChannels      = 0;
	uint16               TtsBitsPerSample = 0;
	TArray<int16>        TtsPcm;
	TArray<FTryllVisemeEvent> TtsVisemes;

	TUniqueFunction<void()> Completion;
};

struct FTryllOutboundFrame
{
	std::uint64_t        RequestId   = 0;
	TArray<uint8>        Bytes;
	::Tryll::MessageBody RequestBody = ::Tryll::MessageBody_NONE;
};

using FTryllPendingHandler =
	TUniqueFunction<bool(const ::Tryll::Message* /*nullable*/, const FTryllErrorContext&)>;

class FTryllSocketWorker;

// ── FTryllConnection ──────────────────────────────────────────────────────────

/**
 * Thread-safe bridge between the game thread and the background socket worker.
 * Call registers a handler before the frame is queued. Completions and push
 * events share one FIFO (EventQueue).
 */
class TRYLLCLIENT_API FTryllConnection final : public TSharedFromThis<FTryllConnection>
{
	friend class FTryllSocketWorker;
	friend class FTryllAcquireDeadSocketSpec;
	friend class FTryllEditorConnectionRecoverySpec;
	friend class FTryllJoinLateReplaySpec;
	friend struct FTryllLicenceGateTestAccess;

public:
	FTryllConnection();
	~FTryllConnection();

	void Start(const FString& Host, int32 Port);
	void Stop();

	[[nodiscard]] bool TryPopEvent(FTryllConnEvent& Out);
	[[nodiscard]] bool TryPopOutbound(FTryllOutboundFrame& Out);

	[[nodiscard]] std::uint64_t AllocateRequestId();
	[[nodiscard]] std::uint64_t GetGeneration() const { return Generation.load(std::memory_order_relaxed); }

	/**
	 * Register Handler under the registry lock, invoke Note, then queue Frame.
	 * Send is fire-and-forget (no registry entry).
	 */
	void Call(
		std::uint64_t RequestId,
		TArray<uint8> Frame,
		const void* Owner,
		FTryllPendingHandler Handler,
		::Tryll::MessageBody RequestBody,
		TUniqueFunction<void(std::uint64_t)> Note);

	void Send(TArray<uint8> Frame);

	/** Worker or caller → game-thread FIFO. Same queue as connection events. */
	void PostToGameThread(TUniqueFunction<void()> Completion);

	/**
	 * Pop the inbound FIFO. Completions run, pushes go through the router, and
	 * connection-state events hit StateHook then the events object. Returns
	 * early if the hook swallows Connected (identity mismatch already stopped
	 * the socket).
	 */
	void PumpEvents();

	/** Discard queued events without delivering them. Used when Stop drops the socket. */
	void DropQueuedEvents();

	/**
	 * Owner side effects for Connected / ConnectFailed / Disconnected. Return
	 * false from Connected to swallow the public broadcast.
	 */
	void SetStateHook(TUniqueFunction<bool(const FTryllConnEvent&)> Hook);

	FTryllPushRouter& GetPushRouter() { return PushRouter; }
	const FTryllPushRouter& GetPushRouter() const { return PushRouter; }
	FTryllConnectionEvents& GetEvents() { return Events; }
	const FTryllConnectionEvents& GetEvents() const { return Events; }

	/** Shared one-shot guard so the editor and a delegating game cannot both send. */
	bool TryBeginCreateSession();
	void MarkSessionReady();
	void AbandonCreateSession();
	void ResetSessionPhase();
	bool IsSessionConfigured() const;
	bool IsCreateSessionInFlight() const;

	/**
	 * True when CreateSession was given a non-empty locale. A text-provider language
	 * change then leaves the session language alone. Shared so PIE and the editor agree.
	 */
	void SetSessionLocaleExplicit(bool bExplicit) { bSessionLocaleExplicit.store(bExplicit); }
	bool IsSessionLocaleExplicit() const { return bSessionLocaleExplicit.load(); }

	/**
	 * Fail every entry owned by Owner. Handlers run before this returns.
	 * A null owner fails nothing.
	 */
	void FailOwnedBy(const void* Owner, const FTryllError& Error);
	void FailRegistered(const FTryllError& Error);
	void FailOneRegistered(std::uint64_t RequestId, const FTryllError& Error);

	/** EditMode seam: run the inbound path without a socket. */
	void TestDeliver(const TArray<uint8>& Payload);
	/** EditMode seam: the disconnect path that fails whatever is still pending. */
	void TestDisconnect(const FString& Message);

	void SetSessionId(std::uint64_t Id) { SessionId.store(Id, std::memory_order_relaxed); }

	[[nodiscard]] bool IsConnected() const { return bConnected.load(std::memory_order_relaxed); }
	[[nodiscard]] std::uint64_t GetSessionId() const { return SessionId.load(std::memory_order_relaxed); }

	FTryllClientObserverHub& GetObserverHub() { return *ObserverHub; }
	const FTryllClientObserverHub& GetObserverHub() const { return *ObserverHub; }

	const TSharedRef<FTryllDiagnosticsDemand>& GetDiagnosticsDemand() const { return DiagnosticsDemand; }

	void EnqueueEvent(FTryllConnEvent&& Event);

private:
	struct FPendingEntry
	{
		FTryllPendingHandler Handler;
		const void*          Owner       = nullptr;
		::Tryll::MessageBody RequestBody = ::Tryll::MessageBody_NONE;
		std::atomic<bool>    bDone{false};

		FPendingEntry() = default;
		FPendingEntry(const FPendingEntry&) = delete;
		FPendingEntry& operator=(const FPendingEntry&) = delete;
	};

	void QueueFrame(FTryllOutboundFrame&& Frame);
	void DispatchRegistered(const ::Tryll::Message* Msg);
	void InvokeHandler(const TSharedPtr<FPendingEntry>& Entry, const ::Tryll::Message* Msg, const FTryllErrorContext& Err);

	TSharedRef<FTryllClientObserverHub> ObserverHub       = MakeShared<FTryllClientObserverHub>();
	TSharedRef<FTryllDiagnosticsDemand> DiagnosticsDemand = MakeShared<FTryllDiagnosticsDemand>();

	FTryllPushRouter PushRouter;
	FTryllConnectionEvents Events;
	TUniqueFunction<bool(const FTryllConnEvent&)> StateHook;

	/** 0 idle, 1 in flight, 2 configured. */
	std::atomic<uint8> SessionPhase{0};
	std::atomic<bool> bSessionLocaleExplicit{false};

	std::atomic<std::uint64_t> NextRequestId{1};
	std::atomic<std::uint64_t> Generation{1};
	std::atomic<bool>          bConnected{false};
	std::atomic<std::uint64_t> SessionId{0};

	TQueue<FTryllConnEvent, EQueueMode::Mpsc> EventQueue;

	std::deque<FTryllOutboundFrame> Outbound;
	std::mutex                      OutboundMutex;

	std::unordered_map<std::uint64_t, TSharedPtr<FPendingEntry>> Registry;
	std::mutex                                       RegistryMutex;

	FThreadSafeBool      bRunWorker{true};
	FRunnableThread*     WorkerThread = nullptr;
	FTryllSocketWorker*  Worker       = nullptr;

	ISocketSubsystem* SocketSubsystem = nullptr;
	FSocket*          Socket          = nullptr;

	FString PendingHost;
	int32   PendingPort = 0;
};
