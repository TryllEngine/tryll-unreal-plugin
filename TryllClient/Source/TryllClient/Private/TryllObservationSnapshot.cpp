// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllObservationSnapshot.h"
#include "TryllObservationClassifier.h"
#include "TryllConnection.h"

#include "HAL/PlatformTime.h"
#include "Misc/DateTime.h"

FString TryllSummarizeVariableUpdates(const TArray<FTryllVariableUpdate>& Updates)
{
	if (Updates.IsEmpty())
	{
		return FString();
	}

	TArray<FString> Parts;
	Parts.Reserve(Updates.Num());
	for (const FTryllVariableUpdate& Update : Updates)
	{
		if (Update.Name.IsEmpty())
		{
			continue;
		}
		Parts.Add(Update.Name);
	}
	return FString::Join(Parts, TEXT(", "));
}

FTryllObservationEnvelope FTryllObservationSnapshot::FromNote(
	const FTryllObservationNote& Note,
	uint64 EventId,
	uint64 Epoch)
{
	FTryllObservationEnvelope Envelope;
	Envelope.EventId          = EventId;
	Envelope.ConnectionEpoch  = Epoch;
	Envelope.Direction        = Note.Direction;
	Envelope.Scope            = FTryllObservationClassifier::Classify(Note);
	Envelope.WallTimeUtc      = FDateTime::UtcNow();
	Envelope.MonotonicSeconds = FPlatformTime::Seconds();
	Envelope.Body             = Note.Body;
	Envelope.RequestBody      = Note.RequestBody;
	Envelope.bIsError         = Note.bIsError;
	Envelope.Connection       = Note.Connection;
	Envelope.RequestId        = Note.RequestId;
	Envelope.AgentId          = Note.AgentId;
	Envelope.VoiceInputId     = Note.VoiceInputId;
	Envelope.CarrierRequestId = Note.CarrierRequestId;
	Envelope.Text             = Note.Text;
	Envelope.Text2            = Note.Text2;
	Envelope.Message          = Note.Message;
	Envelope.NodeName         = Note.NodeName;
	Envelope.BinaryKind       = Note.BinaryKind;
	Envelope.GraphDefaultModelName = Note.GraphDefaultModelName;
	Envelope.PayloadSummary   = Note.PayloadSummary;
	Envelope.bFlag            = Note.bFlag;
	Envelope.bFlag2           = Note.bFlag2;
	Envelope.TurnStatus       = Note.TurnStatus;
	Envelope.TokensGenerated  = Note.TokensGenerated;
	Envelope.Code             = Note.Code;
	Envelope.BinaryByteCount  = Note.BinaryByteCount;
	Envelope.bEnableDiagnostics = Note.bEnableDiagnostics;
	Envelope.bMaintainDialogueHistory = Note.bMaintainDialogueHistory;
	Envelope.SampleRate       = Note.SampleRate;
	Envelope.Channels         = Note.Channels;
	Envelope.BitsPerSample    = Note.BitsPerSample;
	Envelope.ThrottleLevel    = Note.ThrottleLevel;
	Envelope.RemoveInteractionCount = Note.RemoveInteractionCount;
	Envelope.VariableUpdateCount = Note.VariableUpdateCount;
	Envelope.DialogInteractionCount = Note.DialogInteractionCount;
	Envelope.TranscriptKind   = Note.TranscriptKind;
	Envelope.AudioMsConsumed  = Note.AudioMsConsumed;
	return Envelope;
}

FTryllObservationEnvelope FTryllObservationSnapshot::FromEvent(
	const FTryllConnEvent& Evt,
	uint64 EventId,
	uint64 Epoch)
{
	FTryllObservationNote Note;
	Note.Direction = ETryllObservationDirection::Inbound;
	Note.RequestId = Evt.RequestId;
	Note.AgentId = Evt.AgentId;
	Note.VoiceInputId = Evt.VoiceInputId;
	Note.Text = Evt.Text;
	Note.Message = Evt.Message;
	Note.bFlag = Evt.bFlag;
	Note.bFlag2 = Evt.bFlag2;
	Note.TurnStatus = Evt.TurnStatus;
	Note.TokensGenerated = Evt.TokensGenerated;
	Note.Code = Evt.Code;
	Note.SampleRate = Evt.TtsSampleRate;
	Note.Channels = Evt.TtsChannels;
	Note.BitsPerSample = Evt.TtsBitsPerSample;
	Note.BinaryByteCount = static_cast<uint32>(Evt.TtsPcm.Num() * sizeof(int16));
	Note.TranscriptKind = Evt.TranscriptKind;
	Note.AudioMsConsumed = Evt.AudioMsConsumed;

	switch (Evt.Type)
	{
	case ETryllConnEventType::Connected:
		Note.Connection = ETryllConnectionObservation::Connected;
		break;
	case ETryllConnEventType::ConnectFailed:
		Note.Connection = ETryllConnectionObservation::ConnectFailed;
		break;
	case ETryllConnEventType::Disconnected:
		Note.Connection = ETryllConnectionObservation::Disconnected;
		break;
	case ETryllConnEventType::AnswerText:
		Note.Body = ::Tryll::MessageBody_AnswerText;
		Note.RequestBody = ::Tryll::MessageBody_SendMessageRequest;
		Note.NodeName = Evt.Message;
		break;
	case ETryllConnEventType::TurnComplete:
		Note.Body = ::Tryll::MessageBody_TurnComplete;
		Note.RequestBody = ::Tryll::MessageBody_SendMessageRequest;
		break;
	case ETryllConnEventType::NodeEvent:
		Note.Body = ::Tryll::MessageBody_NodeEvent;
		Note.NodeName = Evt.Message;
		break;
	case ETryllConnEventType::TranscriptUpdate:
		Note.Body = ::Tryll::MessageBody_WireTranscriptUpdate;
		break;
	case ETryllConnEventType::TtsAudioFormat:
		Note.Body = ::Tryll::MessageBody_TtsAudioFormatFrame;
		Note.RequestBody = ::Tryll::MessageBody_SendMessageRequest;
		Note.BinaryKind = TEXT("tts_format");
		break;
	case ETryllConnEventType::TtsAudio:
		Note.Body = ::Tryll::MessageBody_TtsAudioFrame;
		Note.RequestBody = ::Tryll::MessageBody_SendMessageRequest;
		Note.BinaryKind = TEXT("tts_pcm");
		break;
	case ETryllConnEventType::TtsViseme:
		Note.Body = ::Tryll::MessageBody_TtsVisemeFrame;
		Note.RequestBody = ::Tryll::MessageBody_SendMessageRequest;
		break;
	default:
		break;
	}

	return FromNote(Note, EventId, Epoch);
}
