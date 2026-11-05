// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllClientObserver.h"
#include "TryllObservationSnapshot.h"
#include "TryllConnection.h"
#include "TryllUtf8.h"
#include "Generated/TryllAgentLogJson.gen.h"
#include "Generated/TryllMessageDescriptors.gen.h"

#include <flatbuffers/flatbuffers.h>

// ── FTryllNullClientObserver ──────────────────────────────────────────────────

TSharedRef<FTryllNullClientObserver> FTryllNullClientObserver::Get()
{
	static TSharedRef<FTryllNullClientObserver> Instance = MakeShared<FTryllNullClientObserver>();
	return Instance;
}

void FTryllNullClientObserver::OnObservation(const FTryllObservationEnvelope& /*Envelope*/)
{
}

// ── FTryllCompositeClientObserver ─────────────────────────────────────────────

void FTryllCompositeClientObserver::Add(TSharedRef<ITryllClientObserver> Observer)
{
	Observers.Add(MoveTemp(Observer));
}

void FTryllCompositeClientObserver::OnObservation(const FTryllObservationEnvelope& Envelope)
{
	for (const TSharedRef<ITryllClientObserver>& Observer : Observers)
	{
		Observer->OnObservation(Envelope);
	}
}

// ── FTryllClientObserverHub ───────────────────────────────────────────────────

void FTryllClientObserverHub::BeginConnectionEpoch()
{
	++ConnectionEpoch;
	FScopeLock Lock(&Gate);
	Carriers.Empty();
}

bool FTryllClientObserverHub::HasSubscribers() const
{
	FScopeLock Lock(&Gate);
	return Subscribers.Num() > 0;
}

FDelegateHandle FTryllClientObserverHub::Subscribe(TSharedRef<ITryllClientObserver> Observer)
{
	const FDelegateHandle Handle(FDelegateHandle::GenerateNewHandle);
	FScopeLock Lock(&Gate);
	FSubscriberEntry Entry;
	Entry.Handle   = Handle;
	Entry.Observer = Observer;
	Subscribers.Add(MoveTemp(Entry));
	return Handle;
}

void FTryllClientObserverHub::Unsubscribe(FDelegateHandle Handle)
{
	FScopeLock Lock(&Gate);
	Subscribers.RemoveAll([&Handle](const FSubscriberEntry& Entry)
	{
		return Entry.Handle == Handle;
	});
}

void FTryllClientObserverHub::Observe(const FTryllObservationNote& Note)
{
	if (!HasSubscribers())
	{
		return;
	}

	const uint64 EventId = NextEventId.fetch_add(1, std::memory_order_relaxed);
	FTryllObservationEnvelope Envelope =
		FTryllObservationSnapshot::FromNote(Note, EventId, ConnectionEpoch);

	FScopeLock Lock(&Gate);
	Queue.Enqueue(MoveTemp(Envelope));
}

namespace TryllAgentLogCapture
{

static TArray<uint8> FinishLocal(
	flatbuffers::FlatBufferBuilder& Fbb,
	flatbuffers::Offset<::Tryll::LogLocalEnvelope> Envelope)
{
	::Tryll::FinishLogLocalEnvelopeBuffer(Fbb, Envelope);
	TArray<uint8> Out;
	Out.Append(Fbb.GetBufferPointer(), static_cast<int32>(Fbb.GetSize()));
	return Out;
}

static void Hydrate(FTryllObservationNote& Note, const uint8* Data)
{
	const ::Tryll::Message* Msg = ::Tryll::GetMessage(Data);
	if (!Msg)
	{
		return;
	}
	switch (Note.Body)
	{
	case ::Tryll::MessageBody_AnswerText:
		if (const ::Tryll::AnswerText* Row = Msg->body_as<::Tryll::AnswerText>())
		{
			Note.Text = Tryll::Client::Utf8ToFString(Row->text());
			Note.NodeName = Tryll::Client::Utf8ToFString(Row->node_name());
			Note.bFlag = Row->is_delta();
			Note.bFlag2 = Row->is_final();
		}
		break;
	case ::Tryll::MessageBody_SendMessageRequest:
		if (const ::Tryll::SendMessageRequest* Row = Msg->body_as<::Tryll::SendMessageRequest>())
		{
			Note.Text = Tryll::Client::Utf8ToFString(Row->text());
		}
		break;
	case ::Tryll::MessageBody_ResumeAgentRequest:
		if (const ::Tryll::ResumeAgentRequest* Row = Msg->body_as<::Tryll::ResumeAgentRequest>())
		{
			Note.Text = Tryll::Client::Utf8ToFString(Row->resume_node());
		}
		break;
	case ::Tryll::MessageBody_ChangeAgentParamRequest:
		if (const ::Tryll::ChangeAgentParamRequest* Row = Msg->body_as<::Tryll::ChangeAgentParamRequest>())
		{
			Note.NodeName = Tryll::Client::Utf8ToFString(Row->node_name());
		}
		break;
	case ::Tryll::MessageBody_TurnComplete:
		if (const ::Tryll::TurnComplete* Row = Msg->body_as<::Tryll::TurnComplete>())
		{
			Note.Text = Tryll::Client::Utf8ToFString(Row->debug_info());
			Note.TurnStatus = static_cast<uint8>(Row->status());
			Note.TokensGenerated = Row->tokens_generated();
		}
		break;
	case ::Tryll::MessageBody_ErrorResponse:
		if (const ::Tryll::ErrorResponse* Row = Msg->body_as<::Tryll::ErrorResponse>())
		{
			Note.Message = Tryll::Client::Utf8ToFString(Row->message());
			Note.Code = Row->code();
			Note.bIsError = true;
		}
		break;
	case ::Tryll::MessageBody_NodeEvent:
		if (const ::Tryll::NodeEvent* Row = Msg->body_as<::Tryll::NodeEvent>())
		{
			Note.NodeName = Tryll::Client::Utf8ToFString(Row->node_name());
		}
		break;
	case ::Tryll::MessageBody_TtsAudioFormatFrame:
		if (const ::Tryll::TtsAudioFormatFrame* Row = Msg->body_as<::Tryll::TtsAudioFormatFrame>())
		{
			Note.NodeName = Tryll::Client::Utf8ToFString(Row->node_name());
		}
		break;
	case ::Tryll::MessageBody_TtsAudioFrame:
		if (const ::Tryll::TtsAudioFrame* Row = Msg->body_as<::Tryll::TtsAudioFrame>())
		{
			Note.NodeName = Tryll::Client::Utf8ToFString(Row->node_name());
		}
		break;
	case ::Tryll::MessageBody_TtsVisemeFrame:
		if (const ::Tryll::TtsVisemeFrame* Row = Msg->body_as<::Tryll::TtsVisemeFrame>())
		{
			Note.NodeName = Tryll::Client::Utf8ToFString(Row->node_name());
		}
		break;
	case ::Tryll::MessageBody_CreateAgentRequest:
		if (const ::Tryll::CreateAgentRequest* Row = Msg->body_as<::Tryll::CreateAgentRequest>())
		{
			Note.GraphDefaultModelName = Tryll::Client::Utf8ToFString(Row->default_model_name());
			Note.bEnableDiagnostics = Row->enable_diagnostics();
			Note.bMaintainDialogueHistory = Row->maintain_dialogue_history();
		}
		break;
	case ::Tryll::MessageBody_UpdateAgentVariablesRequest:
		if (const ::Tryll::UpdateAgentVariablesRequest* Row = Msg->body_as<::Tryll::UpdateAgentVariablesRequest>())
		{
			Note.VariableUpdateCount = Row->updates() ? static_cast<int32>(Row->updates()->size()) : 0;
		}
		break;
	default:
		break;
	}
}

} // namespace TryllAgentLogCapture

void FTryllClientObserverHub::ObserveEvent(const FTryllConnEvent& Evt)
{
	if (!HasSubscribers())
	{
		return;
	}

	const uint64 EventId = NextEventId.fetch_add(1, std::memory_order_relaxed);
	FTryllObservationEnvelope Envelope =
		FTryllObservationSnapshot::FromEvent(Evt, EventId, ConnectionEpoch);
	switch (Evt.Type)
	{
	case ETryllConnEventType::Connected:
		Envelope.Frame = MakeConnection(::Tryll::LogConnState_connected, Evt.ServerVersion);
		Envelope.bIsLocal = true;
		break;
	case ETryllConnEventType::ConnectFailed:
		Envelope.Frame = MakeConnection(::Tryll::LogConnState_failed, Evt.Message);
		Envelope.bIsLocal = true;
		break;
	case ETryllConnEventType::Disconnected:
		Envelope.Frame = MakeConnection(::Tryll::LogConnState_dropped, Evt.Message);
		Envelope.bIsLocal = true;
		break;
	default:
		break;
	}

	FScopeLock Lock(&Gate);
	Queue.Enqueue(MoveTemp(Envelope));
}

void FTryllClientObserverHub::ObserveWire(const uint8* Data, int32 Size, ETryllObservationDirection Direction)
{
	if (!HasSubscribers() || Data == nullptr || Size <= 0)
	{
		return;
	}

	std::vector<FTryllOmittedField> Omitted;
	std::vector<uint8> Stripped = ::Tryll::AgentLog::StripMessage(Data, static_cast<size_t>(Size), Omitted);
	::Tryll::MessageBody Body = ::Tryll::MessageBody_NONE;
	uint64 RequestId = 0;
	uint64 AgentId = 0;
	uint64 VoiceInputId = 0;
	::Tryll::AgentLog::ReadScope(Data, static_cast<size_t>(Size), Body, RequestId, AgentId, VoiceInputId);

	::Tryll::MessageBody RequestBody = Body;
	if (Direction == ETryllObservationDirection::Inbound)
	{
		if (const ::Tryll::FTryllMessageDescriptor* Descriptor = ::Tryll::MessageDescriptor(Body))
		{
			if (Descriptor->Request != ::Tryll::MessageBody_NONE)
			{
				RequestBody = Descriptor->Request;
			}
		}
	}

	FTryllObservationNote Note;
	Note.Direction = Direction;
	Note.Body = Body;
	Note.RequestBody = RequestBody;
	Note.RequestId = RequestId;
	Note.AgentId = AgentId;
	Note.VoiceInputId = VoiceInputId;
	const std::string Label = ::Tryll::AgentLog::Label(Body, Data, static_cast<size_t>(Size));
	Note.Text = UTF8_TO_TCHAR(Label.c_str());
	TryllAgentLogCapture::Hydrate(Note, Data);

	{
		FScopeLock Lock(&Gate);
		if (uint64* Carrier = Carriers.Find(RequestId))
		{
			Note.CarrierRequestId = *Carrier;
			Carriers.Remove(RequestId);
		}
	}

	FTryllObservationEnvelope Envelope = FTryllObservationSnapshot::FromNote(Note, AllocateEventId(), ConnectionEpoch);
	const uint8* Kept = Stripped.empty() ? Data : Stripped.data();
	const int32 KeptSize = Stripped.empty() ? Size : static_cast<int32>(Stripped.size());
	Envelope.Frame.Append(Kept, KeptSize);
	Envelope.Omitted.Reserve(static_cast<int32>(Omitted.size()));
	for (const FTryllOmittedField& Field : Omitted)
	{
		FTryllObservationEnvelope::FOmittedField Row;
		Row.Path = UTF8_TO_TCHAR(Field.Path.c_str());
		Row.Count = Field.Count;
		Row.Bytes = Field.Bytes;
		Envelope.Omitted.Add(MoveTemp(Row));
	}

	FScopeLock Lock(&Gate);
	Queue.Enqueue(MoveTemp(Envelope));
}

void FTryllClientObserverHub::ObserveLocal(
	TArray<uint8> LocalFrame, uint64 RequestId, uint64 AgentId, bool bIsError, const FString& Message, uint32 Code)
{
	if (!HasSubscribers() || LocalFrame.Num() == 0)
	{
		return;
	}

	FTryllObservationEnvelope Envelope;
	Envelope.EventId = AllocateEventId();
	Envelope.ConnectionEpoch = ConnectionEpoch;
	Envelope.Direction = ETryllObservationDirection::Inbound;
	Envelope.Scope = ETryllObservationScope::RawSession;
	Envelope.WallTimeUtc = FDateTime::UtcNow();
	Envelope.MonotonicSeconds = FPlatformTime::Seconds();
	Envelope.RequestId = RequestId;
	Envelope.AgentId = AgentId;
	Envelope.bIsError = bIsError;
	Envelope.Message = Message;
	Envelope.Code = Code;
	Envelope.Frame = MoveTemp(LocalFrame);
	Envelope.bIsLocal = true;

	FScopeLock Lock(&Gate);
	Queue.Enqueue(MoveTemp(Envelope));
}

void FTryllClientObserverHub::AnnotateNote(const FTryllObservationNote& Note)
{
	if (Note.CarrierRequestId != 0)
	{
		AnnotateCarrier(Note.RequestId, Note.CarrierRequestId);
	}
}

void FTryllClientObserverHub::AnnotateCarrier(uint64 RequestId, uint64 CarrierRequestId)
{
	if (RequestId == 0 || CarrierRequestId == 0 || !HasSubscribers())
	{
		return;
	}
	FScopeLock Lock(&Gate);
	Carriers.Add(RequestId, CarrierRequestId);
}

TArray<uint8> FTryllClientObserverHub::MakeConnection(::Tryll::LogConnState State, const FString& Reason)
{
	flatbuffers::FlatBufferBuilder Fbb(128);
	const auto Body = Reason.IsEmpty()
		? ::Tryll::CreateLogConnectionState(Fbb, State)
		: ::Tryll::CreateLogConnectionStateDirect(Fbb, State, TCHAR_TO_UTF8(*Reason));
	return TryllAgentLogCapture::FinishLocal(
		Fbb, ::Tryll::CreateLogLocalEnvelope(Fbb, ::Tryll::LogLocalBody_LogConnectionState, Body.Union()));
}

TArray<uint8> FTryllClientObserverHub::MakeError(
	uint32 Code, const FString& CodeName, const FString& Message, ::Tryll::LogErrorOrigin Origin)
{
	flatbuffers::FlatBufferBuilder Fbb(256);
	const FTCHARToUTF8 NameUtf8(*CodeName);
	const FTCHARToUTF8 MessageUtf8(*Message);
	const auto Body = ::Tryll::CreateLogLocalErrorDirect(
		Fbb,
		Code,
		CodeName.IsEmpty() ? nullptr : NameUtf8.Get(),
		Message.IsEmpty() ? nullptr : MessageUtf8.Get(),
		Origin);
	return TryllAgentLogCapture::FinishLocal(
		Fbb, ::Tryll::CreateLogLocalEnvelope(Fbb, ::Tryll::LogLocalBody_LogLocalError, Body.Union()));
}

TArray<uint8> FTryllClientObserverHub::MakeGap(::Tryll::LogGapReason Reason)
{
	flatbuffers::FlatBufferBuilder Fbb(64);
	const auto Body = ::Tryll::CreateLogCaptureGap(Fbb, Reason);
	return TryllAgentLogCapture::FinishLocal(
		Fbb, ::Tryll::CreateLogLocalEnvelope(Fbb, ::Tryll::LogLocalBody_LogCaptureGap, Body.Union()));
}

int32 FTryllClientObserverHub::Pump(int32 MaxCount)
{
	if (MaxCount <= 0)
	{
		return 0;
	}

	TArray<TSharedPtr<ITryllClientObserver>> Snapshot;
	{
		FScopeLock Lock(&Gate);
		if (Subscribers.Num() == 0 || Queue.IsEmpty())
		{
			return 0;
		}
		Snapshot.Reserve(Subscribers.Num());
		for (const FSubscriberEntry& Entry : Subscribers)
		{
			if (Entry.Observer.IsValid())
			{
				Snapshot.Add(Entry.Observer);
			}
		}
	}

	int32 Delivered = 0;
	while (Delivered < MaxCount)
	{
		FTryllObservationEnvelope Envelope;
		{
			FScopeLock Lock(&Gate);
			if (!Queue.Dequeue(Envelope))
			{
				break;
			}
		}

		for (const TSharedPtr<ITryllClientObserver>& Observer : Snapshot)
		{
			if (Observer.IsValid())
			{
				Observer->OnObservation(Envelope);
			}
		}
		++Delivered;
	}

	return Delivered;
}

uint64 FTryllClientObserverHub::AllocateEventId()
{
	return NextEventId.fetch_add(1, std::memory_order_relaxed);
}
