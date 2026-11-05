// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllConnection.h"
#include "TryllRuntimeSettings.h"
#include "ClientCodec.h"
#include "ClientWire.h"
#include "FTryllErrorContext.h"
#include "Win32Undef.h"
#include "TryllUtf8.h"
#include "TryllClientModule.h"

#include "messages_generated.h"
#include "tryll/Protocol.h"
#include "CodegenFingerprint.gen.h"

#include "SocketSubsystem.h"
#include "Sockets.h"
#include "IPAddress.h"

#include <flatbuffers/verifier.h>
#include <vector>

using Tryll::Client::Utf8ToFString;

// ExtractRequestId is generated from the schema; replaces the hand-written copy above.
#include "Generated/RequestId.gen.h"
#include "Generated/RpcTraits.gen.h"
#include "Generated/TryllMessageDescriptors.gen.h"
#include "TryllObservationCurated.h"

// ── FTryllSocketWorker ────────────────────────────────────────────────────────

class FTryllSocketWorker final : public FRunnable
{
public:
	FTryllSocketWorker(TSharedPtr<FTryllConnection> InOwner, FString InHost, int32 InPort,
		double InConnectionReadyTimeoutSeconds)
		: Owner(MoveTemp(InOwner))
		, Host(MoveTemp(InHost))
		, Port(InPort)
		, ConnectionReadyTimeoutSeconds(InConnectionReadyTimeoutSeconds)
	{
	}

	virtual bool   Init() override { return true; }
	virtual uint32 Run()  override;
	virtual void   Stop() override { bStop.AtomicSet(true); }
	virtual void   Exit() override {}

	void HandleIncoming(const TArray<uint8>& Payload);

private:
	TSharedPtr<FTryllConnection> Owner;
	FString                      Host;
	int32                        Port = 0;
	double                       ConnectionReadyTimeoutSeconds = 10.0;
	ISocketSubsystem*            SocketSubsystem = nullptr;
	FSocket*                     Socket = nullptr;
	FThreadSafeBool              bStop{false};

	void Enqueue(FTryllConnEvent&& E) const
	{
		if (Owner.IsValid())
		{
			Owner->EnqueueEvent(MoveTemp(E));
		}
	}

	void FailAllPending(uint32 Code, const FString& Msg);

	void OnIncomingAnswerText(const ::Tryll::Message* Msg);
	void OnIncomingTurnComplete(const ::Tryll::Message* Msg);
	void OnIncomingDownloadProgress(const ::Tryll::Message* Msg);
	void OnIncomingTtsAudioFormatFrame(const ::Tryll::Message* Msg);
	void OnIncomingTtsAudioFrame(const ::Tryll::Message* Msg);
	void OnIncomingTtsVisemeFrame(const ::Tryll::Message* Msg);
	void OnIncomingNodeEvent(const ::Tryll::Message* Msg);
	void OnIncomingWireTranscriptUpdate(const ::Tryll::Message* Msg);
	void OnIncomingConnectionReady(const ::Tryll::Message* Msg);
};

// ── Fan-out handlers (driven by Dispatch.gen.inc) ─────────────────────────────

void FTryllSocketWorker::OnIncomingAnswerText(const ::Tryll::Message* Msg)
{
	const auto* AT = Msg->body_as<::Tryll::AnswerText>();
	if (!AT) return;
	FTryllConnEvent E;
	E.Type      = ETryllConnEventType::AnswerText;
	E.RequestId = AT->request_id();
	E.AgentId   = AT->agent_id();
	E.Text      = Utf8ToFString(AT->text());
	E.Message   = Utf8ToFString(AT->node_name());
	E.bFlag     = AT->is_delta();
	E.bFlag2    = AT->is_final();
	Enqueue(MoveTemp(E));
	// HandleIncoming falls through to pending correlation.
}

void FTryllSocketWorker::OnIncomingTurnComplete(const ::Tryll::Message* Msg)
{
	const auto* TC = Msg->body_as<::Tryll::TurnComplete>();
	if (!TC) return;
	FTryllConnEvent E;
	E.Type            = ETryllConnEventType::TurnComplete;
	E.RequestId       = TC->request_id();
	E.AgentId         = TC->agent_id();
	E.TurnStatus      = static_cast<uint8>(TC->status());
	E.Text            = Utf8ToFString(TC->debug_info());
	E.TokensGenerated = TC->tokens_generated();
	Enqueue(MoveTemp(E));
	// HandleIncoming falls through to pending correlation.
}

void FTryllSocketWorker::OnIncomingDownloadProgress(const ::Tryll::Message* Msg)
{
	// DownloadProgress is handled entirely by the pending lambda registered
	// during DownloadModel; no per-agent fan-out event here.
	// HandleIncoming falls through to pending correlation.
	(void)Msg;
}

void FTryllSocketWorker::OnIncomingNodeEvent(const ::Tryll::Message* Msg)
{
	const auto* Ev = Msg->body_as<::Tryll::NodeEvent>();
	if (!Ev) return;
	FTryllConnEvent E;
	E.Type      = ETryllConnEventType::NodeEvent;
	E.RequestId = Ev->request_id();
	E.AgentId   = Ev->agent_id();
	E.Text      = Utf8ToFString(Ev->node_name());
	E.Message   = Utf8ToFString(Ev->event_type());
	if (const auto* Kvs = Ev->kv_pairs())
	{
		E.KvPairs.Reserve(Kvs->size());
		for (uint32 i = 0; i < Kvs->size(); ++i)
		{
			const auto* P = Kvs->Get(i);
			if (!P) continue;
			E.KvPairs.Emplace(Utf8ToFString(P->key()), Utf8ToFString(P->value()));
		}
	}
	Enqueue(MoveTemp(E));
	// HandleIncoming returns after NOTIF dispatch (no pending correlation).
}

void FTryllSocketWorker::OnIncomingWireTranscriptUpdate(const ::Tryll::Message* Msg)
{
	const auto* Upd = Msg->body_as<::Tryll::WireTranscriptUpdate>();
	if (!Upd) return;
	FTryllConnEvent E;
	E.Type            = ETryllConnEventType::TranscriptUpdate;
	E.VoiceInputId    = Upd->voice_input_id();
	E.Text            = Utf8ToFString(Upd->text());
	E.TranscriptKind  = static_cast<uint8>(Upd->kind());
	E.AudioMsConsumed = Upd->audio_ms_consumed();
	Enqueue(MoveTemp(E));
	// HandleIncoming returns after NOTIF dispatch (no pending correlation).
}

void FTryllSocketWorker::OnIncomingTtsAudioFormatFrame(const ::Tryll::Message* Msg)
{
	// codec:"manual" Streaming frame; routed here by the generated Dispatch.gen.inc.
	const auto* F = Msg->body_as<::Tryll::TtsAudioFormatFrame>();
	if (!F) return;
	const auto* Fmt = F->format();
	if (!Fmt) return;
	FTryllConnEvent E;
	E.Type             = ETryllConnEventType::TtsAudioFormat;
	E.RequestId        = F->request_id();
	E.AgentId          = F->agent_id();
	E.TtsSampleRate    = Fmt->sample_rate();
	E.TtsChannels      = Fmt->channels();
	E.TtsBitsPerSample = Fmt->bits_per_sample();
	E.Message          = Utf8ToFString(F->node_name());
	Enqueue(MoveTemp(E));
}

void FTryllSocketWorker::OnIncomingTtsAudioFrame(const ::Tryll::Message* Msg)
{
	// codec:"manual" Streaming frame; routed here by the generated Dispatch.gen.inc.
	const auto* F = Msg->body_as<::Tryll::TtsAudioFrame>();
	if (!F) return;
	const auto* PcmVec = F->pcm();

	FTryllConnEvent E;
	E.Type      = ETryllConnEventType::TtsAudio;
	E.RequestId = F->request_id();
	E.AgentId   = F->agent_id();
	E.Message   = Utf8ToFString(F->node_name());

	if (PcmVec && PcmVec->size() > 0)
	{
		// PCM is int16 LE in raw bytes; copy into the event so the worker
		// frees the FlatBuffer before the game thread consumes the chunk.
		const int32 ByteCount   = static_cast<int32>(PcmVec->size());
		const int32 SampleCount = ByteCount / 2;
		E.TtsPcm.SetNumUninitialized(SampleCount);
		FMemory::Memcpy(E.TtsPcm.GetData(), PcmVec->data(), SampleCount * sizeof(int16));
	}
	Enqueue(MoveTemp(E));
}

void FTryllSocketWorker::OnIncomingTtsVisemeFrame(const ::Tryll::Message* Msg)
{
	// Streaming frame (one per synthesized sentence); routed here by the
	// generated Dispatch.gen.inc. Non-terminal: it never completes a pending
	// SendMessage.
	const auto* F = Msg->body_as<::Tryll::TtsVisemeFrame>();
	if (!F) return;

	FTryllConnEvent E;
	E.Type      = ETryllConnEventType::TtsViseme;
	E.RequestId = F->request_id();
	E.AgentId   = F->agent_id();
	E.Message   = Utf8ToFString(F->node_name());

	// Copied out so the worker frees the FlatBuffer before the game thread
	// consumes the track.
	if (const auto* List = F->events())
	{
		if (const auto* Events = List->events())
		{
			E.TtsVisemes.Reserve(static_cast<int32>(Events->size()));
			for (const auto* V : *Events)
			{
				if (!V) continue;
				FTryllVisemeEvent& Out = E.TtsVisemes.AddDefaulted_GetRef();
				Out.SampleOffset = static_cast<int64>(V->sample_offset());
				// A viseme this client doesn't know (newer server) holds as rest,
				// which keeps the rest of the timeline intact.
				const uint8 Raw  = static_cast<uint8>(V->viseme());
				Out.Viseme       = Raw <= static_cast<uint8>(ETryllViseme::ou)
					? static_cast<ETryllViseme>(Raw) : ETryllViseme::sil;
				Out.Weight       = V->weight();
			}
		}
	}
	Enqueue(MoveTemp(E));
}

void FTryllSocketWorker::OnIncomingConnectionReady(const ::Tryll::Message* /*Msg*/)
{
	// ConnectionReady is consumed at startup in Run(); later arrivals are silently ignored.
}

// ── FailAllPending ────────────────────────────────────────────────────────────

void FTryllSocketWorker::FailAllPending(uint32 Code, const FString& Msg)
{
	if (Owner.IsValid())
	{
		Owner->FailRegistered(FTryllError(Code, Msg));
	}
}

// ── HandleIncoming ────────────────────────────────────────────────────────────

void FTryllSocketWorker::HandleIncoming(const TArray<uint8>& Payload)
{
	flatbuffers::Verifier V(Payload.GetData(), static_cast<size_t>(Payload.Num()));
	if (!::Tryll::VerifyMessageBuffer(V))
	{
		UE_LOG(LogTryll, Warning, TEXT("TryllConnection: malformed server message, verifier failed"));
		FailAllPending(static_cast<uint32>(ETryllErrorCode::MalformedMessage), TEXT("Malformed server message"));
		bStop.AtomicSet(true);
		return;
	}

	if (Owner.IsValid())
	{
		Owner->GetObserverHub().ObserveWire(
			Payload.GetData(), Payload.Num(), ETryllObservationDirection::Inbound);
	}

	const auto* Msg = ::Tryll::GetMessage(Payload.GetData());
	if (!Msg)
	{
		return;
	}

	// STREAM: fan-out, then the registry. NOTIF: fan-out and return (request id 0).
	bool bHandledByFanOut = false;
#define TRYLL_MSG_STREAM(Name, Handler) \
	case ::Tryll::MessageBody_##Name: { Handler(Msg); bHandledByFanOut = true; break; }
#define TRYLL_MSG_NOTIF(Name, Handler) \
	case ::Tryll::MessageBody_##Name: { Handler(Msg); return; }
	switch (Msg->body_type())
	{
#include "Generated/Dispatch.gen.inc"
	}
#undef TRYLL_MSG_STREAM
#undef TRYLL_MSG_NOTIF

	if (!Owner.IsValid())
	{
		return;
	}

	const uint64 ReqId = Msg->body_type() == ::Tryll::MessageBody_ErrorResponse
		? Msg->body_as<::Tryll::ErrorResponse>()->request_id()
		: ::Tryll::ExtractRequestId(Msg);
	if (ReqId == 0)
	{
		if (!bHandledByFanOut)
		{
			UE_LOG(LogTryll, Warning, TEXT("TryllConnection: unexpected message body_type=%d"),
				static_cast<int>(Msg->body_type()));
		}
		return;
	}

	Owner->DispatchRegistered(Msg);
}

// ── Run ───────────────────────────────────────────────────────────────────────

uint32 FTryllSocketWorker::Run()
{
	SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		FTryllConnEvent E;
		E.Type    = ETryllConnEventType::ConnectFailed;
		E.Message = TEXT("Socket subsystem unavailable");
		Enqueue(MoveTemp(E));
		return 0;
	}

	Socket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("TryllClient"), false);
	if (!Socket)
	{
		FTryllConnEvent E;
		E.Type    = ETryllConnEventType::ConnectFailed;
		E.Message = TEXT("CreateSocket failed");
		Enqueue(MoveTemp(E));
		return 0;
	}

	if (Owner.IsValid())
	{
		Owner->SocketSubsystem = SocketSubsystem;
		Owner->Socket          = Socket;
	}

	// Resolve host address.
	const FString PortStr = FString::FromInt(Port);
	FAddressInfoResult GAI = SocketSubsystem->GetAddressInfo(
		*Host, *PortStr,
		EAddressInfoFlags::Default,
		NAME_None,
		ESocketType::SOCKTYPE_Streaming);

	TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();
	bool bHaveAddr = false;
	if (GAI.ReturnCode == SE_NO_ERROR && GAI.Results.Num() > 0)
	{
		for (const FAddressInfoResultData& D : GAI.Results)
		{
			if (D.SocketConfiguration == ESocketType::SOCKTYPE_Streaming)
			{
				Addr      = D.Address;
				bHaveAddr = true;
				break;
			}
		}
		if (!bHaveAddr)
		{
			Addr      = GAI.Results[0].Address;
			bHaveAddr = true;
		}
	}

	if (!bHaveAddr)
	{
		bool bIsValid = false;
		Addr->SetIp(*Host, bIsValid);
		Addr->SetPort(Port);
		if (!bIsValid && Host.Equals(TEXT("localhost"), ESearchCase::IgnoreCase))
		{
			Addr->SetIp(TEXT("127.0.0.1"), bIsValid);
			Addr->SetPort(Port);
		}
		bHaveAddr = bIsValid;
	}

	if (!bHaveAddr || !Socket->Connect(*Addr))
	{
		FTryllConnEvent E;
		E.Type    = ETryllConnEventType::ConnectFailed;
		E.Message = TEXT("Connect failed");
		Enqueue(MoveTemp(E));
		SocketSubsystem->DestroySocket(Socket);
		Socket = nullptr;
		if (Owner.IsValid())
		{
			Owner->Socket          = nullptr;
			Owner->SocketSubsystem = nullptr;
		}
		return 0;
	}

	// Read ConnectionReady (connection-level hello; no session_id — the session is
	// created later via CreateSession, which returns the session_id).
	//
	// Bound this read explicitly: the socket is blocking, so without a wait a server
	// that accepts the TCP connection but has not finished starting (cold-start
	// "ConnectionReady-late") would hang this worker thread forever inside ReadFrame,
	// emitting no event — the indefinite-hang bug. On timeout we emit ConnectFailed,
	// which consumes one connect attempt and re-drives Connect via the retry budget,
	// so the worker self-heals as the server warms. (Mirrors ProbeAndVerify's bounded
	// Wait-then-ReadFrame pattern.)
	if (!Socket->Wait(ESocketWaitConditions::WaitForRead,
		FTimespan::FromSeconds(ConnectionReadyTimeoutSeconds)))
	{
		FTryllConnEvent E;
		E.Type    = ETryllConnEventType::ConnectFailed;
		E.Message = TEXT("Timed out waiting for ConnectionReady");
		Enqueue(MoveTemp(E));
		SocketSubsystem->DestroySocket(Socket);
		Socket = nullptr;
		if (Owner.IsValid())
		{
			Owner->Socket          = nullptr;
			Owner->SocketSubsystem = nullptr;
		}
		return 0;
	}

	TArray<uint8> FirstFrame;
	if (!Tryll::Client::ReadFrame(Socket, FirstFrame))
	{
		FTryllConnEvent E;
		E.Type    = ETryllConnEventType::ConnectFailed;
		E.Message = TEXT("Connection closed before ConnectionReady");
		Enqueue(MoveTemp(E));
		SocketSubsystem->DestroySocket(Socket);
		Socket = nullptr;
		if (Owner.IsValid())
		{
			Owner->Socket          = nullptr;
			Owner->SocketSubsystem = nullptr;
		}
		return 0;
	}

	flatbuffers::Verifier Verifier(FirstFrame.GetData(), static_cast<size_t>(FirstFrame.Num()));
	if (!::Tryll::VerifyMessageBuffer(Verifier))
	{
		FTryllConnEvent E;
		E.Type    = ETryllConnEventType::ConnectFailed;
		E.Message = TEXT("ConnectionReady verify failed");
		Enqueue(MoveTemp(E));
		SocketSubsystem->DestroySocket(Socket);
		Socket = nullptr;
		if (Owner.IsValid())
		{
			Owner->Socket          = nullptr;
			Owner->SocketSubsystem = nullptr;
		}
		return 0;
	}

	if (Owner.IsValid())
	{
		Owner->GetObserverHub().ObserveWire(
			FirstFrame.GetData(), FirstFrame.Num(), ETryllObservationDirection::Inbound);
	}

	const auto* FirstMsg = ::Tryll::GetMessage(FirstFrame.GetData());
	if (!FirstMsg || FirstMsg->body_type() != ::Tryll::MessageBody_ConnectionReady)
	{
		FTryllConnEvent E;
		E.Type    = ETryllConnEventType::ConnectFailed;
		E.Message = TEXT("Expected ConnectionReady");
		Enqueue(MoveTemp(E));
		SocketSubsystem->DestroySocket(Socket);
		Socket = nullptr;
		if (Owner.IsValid())
		{
			Owner->Socket          = nullptr;
			Owner->SocketSubsystem = nullptr;
		}
		return 0;
	}

	const auto* Ready = FirstMsg->body_as<::Tryll::ConnectionReady>();
	if (!Ready)
	{
		FTryllConnEvent E;
		E.Type    = ETryllConnEventType::ConnectFailed;
		E.Message = TEXT("Null ConnectionReady body");
		Enqueue(MoveTemp(E));
		SocketSubsystem->DestroySocket(Socket);
		Socket = nullptr;
		if (Owner.IsValid())
		{
			Owner->Socket          = nullptr;
			Owner->SocketSubsystem = nullptr;
		}
		return 0;
	}

	if (Ready->protocol_version() != ::Tryll::ProtocolVersion)
	{
		FTryllConnEvent E;
		E.Type = ETryllConnEventType::ConnectFailed;
		E.Message = FString::Printf(
			TEXT("Protocol version mismatch: server=%u, client=%u"),
			Ready->protocol_version(),
			::Tryll::ProtocolVersion);
		Enqueue(MoveTemp(E));
		SocketSubsystem->DestroySocket(Socket);
		Socket = nullptr;
		if (Owner.IsValid())
		{
			Owner->Socket          = nullptr;
			Owner->SocketSubsystem = nullptr;
		}
		return 0;
	}

	if (const auto* Fp = Ready->codegen_fingerprint())
	{
		if (std::string_view(Fp->c_str()) != ::Tryll::Generated::kCodegenFingerprint)
		{
			UE_LOG(LogTryll, Warning,
				TEXT("Codegen fingerprint mismatch: server=%s, client=%s"),
				UTF8_TO_TCHAR(Fp->c_str()),
				UTF8_TO_TCHAR(::Tryll::Generated::kCodegenFingerprint));
		}
	}

	if (Owner.IsValid())
	{
		// No session_id yet — it is assigned by CreateSession (see the
		// CreateSessionResponse handler) and stored on Owner->SessionId there.
		Owner->bConnected = true;
	}

	{
		FTryllConnEvent E;
		E.Type = ETryllConnEventType::Connected;
		// Server identity from ConnectionReady — verified on the game thread in
		// the subsystem.  No session_id here; it arrives in CreateSessionResponse.
		if (const auto* P = Ready->exe_path())     E.ServerExePath     = UTF8_TO_TCHAR(P->c_str());
		if (const auto* P = Ready->build_config()) E.ServerBuildConfig = UTF8_TO_TCHAR(P->c_str());
		if (const auto* P = Ready->mode())         E.ServerMode        = UTF8_TO_TCHAR(P->c_str());
		if (const auto* P = Ready->version())      E.ServerVersion     = UTF8_TO_TCHAR(P->c_str());
		if (const auto* P = Ready->run_id())       E.ServerRunId       = UTF8_TO_TCHAR(P->c_str());
		E.bServerManaged           = Ready->managed();
		E.ServerIdleTimeoutSeconds = Ready->idle_timeout_seconds();
		Enqueue(MoveTemp(E));
	}

	// Main poll loop.
	bool bEmittedTerminal = false;
	while (!bStop && Owner.IsValid() && Owner->bRunWorker)
	{
		// Drain frames already encoded on the game thread.
		FTryllOutboundFrame Outbound;
		while (Owner.IsValid() && Owner->TryPopOutbound(Outbound))
		{
			if (!Owner->bRunWorker)
			{
				break;
			}
			if (Outbound.Bytes.Num() == 0)
			{
				continue;
			}

			const Tryll::Client::ETryllWriteFrameResult WriteResult =
				Tryll::Client::WriteFrame(Socket, Outbound.Bytes);
			if (WriteResult == Tryll::Client::ETryllWriteFrameResult::TooLarge)
			{
				const bool bImport = Outbound.RequestBody == ::Tryll::MessageBody_ImportDialogRequest;
				if (Owner.IsValid())
				{
					Owner->FailOneRegistered(
						Outbound.RequestId,
						FTryllError(
							static_cast<uint32>(bImport
								? ETryllErrorCode::DialogSnapshotTooLarge
								: ETryllErrorCode::FrameTooLarge),
							bImport
								? TEXT("Encoded dialog snapshot exceeds MaxDialogSnapshotBytes")
								: TEXT("Outbound frame exceeds MaxFramePayloadBytes")));
				}
			}
			else if (WriteResult == Tryll::Client::ETryllWriteFrameResult::IoError)
			{
				FailAllPending(static_cast<uint32>(ETryllErrorCode::ConnectionDropped), TEXT("Write failed"));
				if (Owner.IsValid())
				{
					Owner->bConnected = false;
					FTryllConnEvent E;
					E.Type    = ETryllConnEventType::Disconnected;
					E.Message = TEXT("Write failed");
					Enqueue(MoveTemp(E));
				}
				bEmittedTerminal = true;
				bStop.AtomicSet(true);
				break;
			}
		}


		if (bStop || !Owner.IsValid() || !Owner->bRunWorker)
		{
			break;
		}

		// Wait up to 32 ms for a readable frame.
		const bool bReady = Socket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromMilliseconds(32));
		if (!bReady)
		{
			continue;
		}

		TArray<uint8> Payload;
		if (!Tryll::Client::ReadFrame(Socket, Payload))
		{
			FailAllPending(static_cast<uint32>(ETryllErrorCode::ConnectionDropped), TEXT("Connection closed"));
			if (Owner.IsValid())
			{
				Owner->bConnected = false;
				FTryllConnEvent E;
				E.Type = ETryllConnEventType::Disconnected;
				Enqueue(MoveTemp(E));
			}
			bEmittedTerminal = true;
			break;
		}

		HandleIncoming(Payload);
	}

	if (!bEmittedTerminal)
	{
		FailAllPending(static_cast<uint32>(ETryllErrorCode::ConnectionDropped), TEXT("Connection dropped"));
		if (Owner.IsValid() && Owner->bConnected)
		{
			FTryllConnEvent E;
			E.Type    = ETryllConnEventType::Disconnected;
			E.Message = TEXT("Connection dropped");
			Enqueue(MoveTemp(E));
		}
	}

	if (Owner.IsValid())
	{
		Owner->bConnected = false;
	}

	if (SocketSubsystem && Socket)
	{
		SocketSubsystem->DestroySocket(Socket);
		Socket = nullptr;
	}
	if (Owner.IsValid())
	{
		Owner->Socket          = nullptr;
		Owner->SocketSubsystem = nullptr;
	}

	return 0;
}

// ── FTryllConnection ──────────────────────────────────────────────────────────

FTryllConnection::FTryllConnection() = default;

FTryllConnection::~FTryllConnection()
{
	Stop();
}

void FTryllConnection::Start(const FString& InHost, int32 InPort)
{
	Stop();

	bRunWorker  = true;
	bConnected  = false;
	SessionId   = 0;
	NextRequestId = 1;
	ObserverHub->BeginConnectionEpoch();
	PendingHost = InHost;
	PendingPort = InPort;

	// Frames the retiring worker did not write belong to the failed epoch.
	{
		std::lock_guard<std::mutex> Lock(OutboundMutex);
		Outbound.clear();
	}
	Generation.fetch_add(1, std::memory_order_relaxed);

	// Run completions the retiring worker already posted, and drop stale
	// connection events so they cannot apply to the new socket. Snapshot
	// first: a completion may enqueue work for the new generation.
	std::vector<FTryllConnEvent> PreviousEpoch;
	{
		FTryllConnEvent Discarded;
		while (EventQueue.Dequeue(Discarded))
		{
			PreviousEpoch.push_back(MoveTemp(Discarded));
		}
	}
	for (FTryllConnEvent& Ev : PreviousEpoch)
	{
		if (Ev.Type == ETryllConnEventType::Completion && Ev.Completion)
		{
			Ev.Completion();
		}
	}

	TSharedPtr<FTryllConnection> Self = AsShared();
	const double ConnectionReadyTimeoutSeconds =
		GetDefault<UTryllRuntimeSettings>()->ConnectionReadyTimeoutSeconds;
	Worker       = new FTryllSocketWorker(Self, PendingHost, PendingPort, ConnectionReadyTimeoutSeconds);
	WorkerThread = FRunnableThread::Create(Worker, TEXT("TryllSocket"), 0, TPri_Normal);
}

void FTryllConnection::Stop()
{
	bRunWorker = false;

	if (Socket)
	{
		Socket->Close();
	}

	if (WorkerThread)
	{
		WorkerThread->WaitForCompletion();
		delete WorkerThread;
		WorkerThread = nullptr;
	}

	delete Worker;
	Worker = nullptr;

	Socket          = nullptr;
	SocketSubsystem = nullptr;
	bConnected      = false;

	// Whatever the retiring worker did not already fail (including a Call on
	// a connection whose worker never started) fails here, on the caller.
	FailRegistered(FTryllError(ETryllErrorCode::ConnectionDropped, TEXT("Connection dropped")));
}

void FTryllConnection::EnqueueEvent(FTryllConnEvent&& Event)
{
	EventQueue.Enqueue(MoveTemp(Event));
}

bool FTryllConnection::TryPopEvent(FTryllConnEvent& Out)
{
	if (!EventQueue.Dequeue(Out))
	{
		return false;
	}
	// Completions are noted by the wrapper that posted them. Observing them
	// here would record the reply twice.
	// Wire pushes and replies are journaled by the inbound frame tap.
	if (ObserverHub->HasSubscribers()
		&& (Out.Type == ETryllConnEventType::Connected
			|| Out.Type == ETryllConnEventType::ConnectFailed
			|| Out.Type == ETryllConnEventType::Disconnected))
	{
		ObserverHub->ObserveEvent(Out);
	}
	return true;
}

void FTryllConnection::QueueFrame(FTryllOutboundFrame&& Frame)
{
	if (Frame.Bytes.Num() > 0)
	{
		ObserverHub->ObserveWire(
			Frame.Bytes.GetData(), Frame.Bytes.Num(), ETryllObservationDirection::Outbound);
	}
	std::lock_guard<std::mutex> Lock(OutboundMutex);
	Outbound.push_back(MoveTemp(Frame));
}

bool FTryllConnection::TryPopOutbound(FTryllOutboundFrame& Out)
{
	std::lock_guard<std::mutex> Lock(OutboundMutex);
	if (Outbound.empty())
	{
		return false;
	}
	Out = MoveTemp(Outbound.front());
	Outbound.pop_front();
	return true;
}

std::uint64_t FTryllConnection::AllocateRequestId()
{
	return NextRequestId.fetch_add(1, std::memory_order_relaxed);
}

void FTryllConnection::Call(
	std::uint64_t RequestId,
	TArray<uint8> Frame,
	const void* InOwner,
	FTryllPendingHandler Handler,
	::Tryll::MessageBody RequestBody,
	TUniqueFunction<void(std::uint64_t)> Note)
{
	{
		std::lock_guard<std::mutex> Lock(RegistryMutex);
		TSharedPtr<FPendingEntry> Entry = MakeShared<FPendingEntry>();
		Entry->Handler = MoveTemp(Handler);
		Entry->Owner = InOwner;
		Entry->RequestBody = RequestBody;
		Registry.insert_or_assign(RequestId, Entry);
	}
	if (Note)
	{
		Note(RequestId);
	}
	FTryllOutboundFrame Out;
	Out.RequestId = RequestId;
	Out.Bytes = MoveTemp(Frame);
	Out.RequestBody = RequestBody;
	QueueFrame(MoveTemp(Out));
}

void FTryllConnection::Send(TArray<uint8> Frame)
{
	if (Frame.Num() == 0)
	{
		return;
	}
	FTryllOutboundFrame Out;
	Out.Bytes = MoveTemp(Frame);
	QueueFrame(MoveTemp(Out));
}

void FTryllConnection::PostToGameThread(TUniqueFunction<void()> Completion)
{
	FTryllConnEvent E;
	E.Type = ETryllConnEventType::Completion;
	E.Completion = MoveTemp(Completion);
	EnqueueEvent(MoveTemp(E));
}

void FTryllConnection::InvokeHandler(
	const TSharedPtr<FPendingEntry>& Entry, const ::Tryll::Message* Msg, const FTryllErrorContext& Err)
{
	if (Entry && Entry->Handler)
	{
		Entry->Handler(Msg, Err);
	}
}

static void TryllNoteTransportFailure(FTryllConnection& Conn, uint64 RequestId, const FTryllError& Error)
{
	if (!Conn.GetObserverHub().HasSubscribers())
	{
		return;
	}
	const bool bUnexpected = Error.Code == static_cast<int32>(ETryllErrorCode::MalformedMessage);
	const ::Tryll::LogErrorOrigin Origin = bUnexpected
		? ::Tryll::LogErrorOrigin_unexpected_reply
		: ::Tryll::LogErrorOrigin_transport;
	Conn.GetObserverHub().ObserveLocal(
		FTryllClientObserverHub::MakeError(
			static_cast<uint32>(Error.Code), FString::FromInt(Error.Code), Error.Message, Origin),
		RequestId,
		0,
		true,
		Error.Message,
		static_cast<uint32>(Error.Code));
}

void FTryllConnection::FailOwnedBy(const void* InOwner, const FTryllError& Error)
{
	if (!InOwner)
	{
		return;
	}
	std::vector<TSharedPtr<FPendingEntry>> Doomed;
	std::vector<uint64> RequestIds;
	{
		std::lock_guard<std::mutex> Lock(RegistryMutex);
		for (auto It = Registry.begin(); It != Registry.end();)
		{
			if (It->second && It->second->Owner == InOwner)
			{
				bool bExpected = false;
				if (It->second->bDone.compare_exchange_strong(bExpected, true))
				{
					Doomed.push_back(It->second);
					RequestIds.push_back(It->first);
				}
				It = Registry.erase(It);
			}
			else
			{
				++It;
			}
		}
	}
	FTryllErrorContext Err;
	Err.Code = static_cast<uint32>(Error.Code);
	Err.Message = Error.Message;
	for (uint64 RequestId : RequestIds)
	{
		TryllNoteTransportFailure(*this, RequestId, Error);
	}
	for (const TSharedPtr<FPendingEntry>& Entry : Doomed)
	{
		InvokeHandler(Entry, nullptr, Err);
	}
}

void FTryllConnection::FailRegistered(const FTryllError& Error)
{
	std::vector<TSharedPtr<FPendingEntry>> Doomed;
	std::vector<uint64> RequestIds;
	{
		std::lock_guard<std::mutex> Lock(RegistryMutex);
		Doomed.reserve(Registry.size());
		RequestIds.reserve(Registry.size());
		for (auto& Pair : Registry)
		{
			if (!Pair.second)
			{
				continue;
			}
			bool bExpected = false;
			if (Pair.second->bDone.compare_exchange_strong(bExpected, true))
			{
				Doomed.push_back(Pair.second);
				RequestIds.push_back(Pair.first);
			}
		}
		Registry.clear();
	}
	for (uint64 RequestId : RequestIds)
	{
		TryllNoteTransportFailure(*this, RequestId, Error);
	}
	FTryllErrorContext Err;
	Err.Code = static_cast<uint32>(Error.Code);
	Err.Message = Error.Message;
	for (const TSharedPtr<FPendingEntry>& Entry : Doomed)
	{
		InvokeHandler(Entry, nullptr, Err);
	}
}

void FTryllConnection::FailOneRegistered(std::uint64_t RequestId, const FTryllError& Error)
{
	TSharedPtr<FPendingEntry> Entry;
	{
		std::lock_guard<std::mutex> Lock(RegistryMutex);
		auto It = Registry.find(RequestId);
		if (It == Registry.end() || !It->second)
		{
			return;
		}
		bool bExpected = false;
		if (!It->second->bDone.compare_exchange_strong(bExpected, true))
		{
			Registry.erase(It);
			return;
		}
		Entry = It->second;
		Registry.erase(It);
	}
	TryllNoteTransportFailure(*this, RequestId, Error);
	FTryllErrorContext Err;
	Err.Code = static_cast<uint32>(Error.Code);
	Err.Message = Error.Message;
	InvokeHandler(Entry, nullptr, Err);
}

void FTryllConnection::TestDeliver(const TArray<uint8>& Payload)
{
	if (Worker)
	{
		Worker->HandleIncoming(Payload);
		return;
	}
	FTryllSocketWorker Temp(AsShared(), PendingHost, PendingPort, 10.0);
	Temp.HandleIncoming(Payload);
}

void FTryllConnection::TestDisconnect(const FString& Message)
{
	bConnected.store(false, std::memory_order_relaxed);
	FailRegistered(FTryllError(ETryllErrorCode::ConnectionDropped, Message));
}

void FTryllConnection::DispatchRegistered(const ::Tryll::Message* Msg)
{
	if (!Msg)
	{
		return;
	}

	const auto Body = Msg->body_type();
	std::uint64_t ReqId = 0;
	FTryllErrorContext Err;
	if (Body == ::Tryll::MessageBody_ErrorResponse)
	{
		const auto* ER = Msg->body_as<::Tryll::ErrorResponse>();
		if (!ER)
		{
			return;
		}
		ReqId = ER->request_id();
		Err.Code = ER->code();
		Err.Message = Utf8ToFString(ER->message());
		UE_LOG(LogTryll, Warning, TEXT("TryllConnection: server ErrorResponse code=%u msg='%s'"),
			Err.Code, *Err.Message);
	}
	else
	{
		ReqId = ::Tryll::ExtractRequestId(Msg);
	}
	if (ReqId == 0)
	{
		return;
	}

	// A streaming frame can share a call's request id without being its reply.
	if (::Tryll::IsRequestScopedPush(Body))
	{
		std::lock_guard<std::mutex> Lock(RegistryMutex);
		if (Registry.find(ReqId) != Registry.end())
		{
			return;
		}
	}

	TSharedPtr<FPendingEntry> Entry;
	{
		std::lock_guard<std::mutex> Lock(RegistryMutex);
		auto It = Registry.find(ReqId);
		if (It != Registry.end())
		{
			Entry = It->second;
		}
	}
	if (!Entry || Entry->bDone.load(std::memory_order_acquire))
	{
		if (!Entry && Body != ::Tryll::MessageBody_ErrorResponse && Body != ::Tryll::MessageBody_Ack)
		{
			UE_LOG(LogTryll, Verbose, TEXT("TryllConnection: reply request_id=%llu body=%d has no pending call"),
				static_cast<unsigned long long>(ReqId), static_cast<int>(Body));
		}
		return;
	}

	// Claim before the handler so teardown cannot run it as well. A progress
	// frame stays registered: the handler returns false and the turn continues.
	// A SendMessage error other than AgentBusy does too, because TurnComplete
	// still arrives on this request id. Every other frame, including AgentBusy,
	// is the end of the call.
	const auto Progress = ::Tryll::RpcProgressOf(Entry->RequestBody);
	const bool bProgress = Progress != ::Tryll::MessageBody_NONE && Body == Progress;
	const bool bStreamingError = Entry->RequestBody == ::Tryll::MessageBody_SendMessageRequest
		&& Body == ::Tryll::MessageBody_ErrorResponse
		&& Err.Code != static_cast<uint32>(ETryllErrorCode::AgentBusy);
	auto Claim = [&]() -> bool
	{
		bool bExpected = false;
		if (!Entry->bDone.compare_exchange_strong(bExpected, true))
		{
			return false;
		}
		std::lock_guard<std::mutex> Lock(RegistryMutex);
		auto It = Registry.find(ReqId);
		if (It != Registry.end() && It->second == Entry)
		{
			Registry.erase(It);
		}
		return true;
	};
	if (!bProgress && !bStreamingError && !Claim())
	{
		return;
	}
	if ((bProgress || bStreamingError) && Entry->bDone.load(std::memory_order_acquire))
	{
		return;
	}

	bool bTerminal = true;
	if (Entry->Handler)
	{
		bTerminal = Entry->Handler(Msg, Err);
	}
	if (bTerminal && (bProgress || bStreamingError))
	{
		Claim();
	}
}

void FTryllConnection::SetStateHook(TUniqueFunction<bool(const FTryllConnEvent&)> Hook)
{
	StateHook = MoveTemp(Hook);
}

void FTryllConnection::DropQueuedEvents()
{
	FTryllConnEvent Discard;
	while (EventQueue.Dequeue(Discard))
	{
	}
}

void FTryllConnection::PumpEvents()
{
	FTryllConnEvent E;
	while (TryPopEvent(E))
	{
		switch (E.Type)
		{
		case ETryllConnEventType::Completion:
			if (E.Completion)
			{
				E.Completion();
			}
			break;

		case ETryllConnEventType::Connected:
			if (StateHook && !StateHook(E))
			{
				GetObserverHub().Pump(64);
				return;
			}
			Events.OnConnectionChanged.Broadcast(true);
			break;

		case ETryllConnEventType::ConnectFailed:
			if (StateHook)
			{
				StateHook(E);
			}
			Events.OnError.Broadcast(E.Message);
			Events.OnConnectionChanged.Broadcast(false);
			break;

		case ETryllConnEventType::Disconnected:
			if (StateHook)
			{
				StateHook(E);
			}
			Events.OnConnectionChanged.Broadcast(false);
			break;

		default:
			PushRouter.Route(E);
			break;
		}
	}

	GetObserverHub().Pump(64);
}

bool FTryllConnection::TryBeginCreateSession()
{
	uint8 Expected = 0;
	return SessionPhase.compare_exchange_strong(Expected, 1);
}

void FTryllConnection::MarkSessionReady()
{
	SessionPhase.store(2);
}

void FTryllConnection::AbandonCreateSession()
{
	uint8 Expected = 1;
	SessionPhase.compare_exchange_strong(Expected, 0);
}

void FTryllConnection::ResetSessionPhase()
{
	SessionPhase.store(0);
	bSessionLocaleExplicit.store(false);
}

bool FTryllConnection::IsSessionConfigured() const
{
	return SessionPhase.load() == 2;
}

bool FTryllConnection::IsCreateSessionInFlight() const
{
	return SessionPhase.load() == 1;
}
