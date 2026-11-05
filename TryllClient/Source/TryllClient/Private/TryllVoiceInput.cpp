// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllVoiceInput.h"
#include "ClientCodec.h"
#include "TryllConnection.h"
#include "Generated/TryllCalls.gen.h"
#include "TryllWeakCapture.h"
#include "TryllObservationTypes.h"

FTryllVoiceInput::FTryllVoiceInput(TSharedPtr<FTryllConnection> InConnection, std::uint64_t InVoiceInputId)
	: Connection(MoveTemp(InConnection))
	, VoiceInputId(InVoiceInputId)
{
}

FTryllVoiceInput::~FTryllVoiceInput() = default;

void FTryllVoiceInput::BeginUtterance(
	const FTryllUtteranceOptions&       Opts,
	TFunction<void(const FTryllError&)> OnComplete)
{
	if (!IsValid())
	{
		if (OnComplete)
			OnComplete(FTryllError(ETryllErrorCode::VoiceInputNotFound, TEXT("VoiceInput handle not valid")));
		return;
	}
	if (bUtteranceActive)
	{
		if (OnComplete)
			OnComplete(FTryllError(ETryllErrorCode::UtteranceInProgress, TEXT("Utterance already in progress")));
		return;
	}

	bUtteranceActive = true; // eager — allows SendAudioBuffer before the ack arrives
	const auto Weak = CaptureTryllLive(this);
	const uint64 Vid = VoiceInputId;
	Tryll::Client::Calls::BeginUtterance(
		*Connection,
		this,
		Vid,
		Opts.bAutoFinishOnSilence,
		Opts.MaxUtteranceMs,
		[Weak, OnComplete = MoveTemp(OnComplete)](const FTryllError& Error)
		{
			if (auto Live = Weak.Pin())
			{
				if (!Error.IsOk())
				{
					Live->bUtteranceActive = false;
				}
				if (OnComplete)
				{
					OnComplete(Error);
				}
			}
		},
		[Weak, Vid](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				FTryllObservationNote Note;
				Note.Direction = ETryllObservationDirection::Outbound;
				Note.Body = ::Tryll::MessageBody_BeginUtteranceRequest;
				Note.RequestBody = Note.Body;
				Note.RequestId = RequestId;
				Note.VoiceInputId = Vid;
				Live->Connection->GetObserverHub().AnnotateNote(Note);
			}
		});
}

void FTryllVoiceInput::SendAudioBuffer(const int16* Samples, int32 NumSamples)
{
	if (!IsValid() || !bUtteranceActive || Samples == nullptr || NumSamples <= 0)
	{
		return;
	}
	// int16 LE PCM — Windows is little-endian, so a raw byte copy is the wire format.
	TArray<uint8> Bytes;
	Bytes.SetNumUninitialized(NumSamples * 2);
	FMemory::Memcpy(Bytes.GetData(), Samples, static_cast<SIZE_T>(NumSamples) * 2);
	Connection->Send(Tryll::Client::BuildAudioBuffer(VoiceInputId, Bytes));
}

void FTryllVoiceInput::EndUtterance(TFunction<void(const FTryllError&)> OnComplete)
{
	if (!IsValid())
	{
		if (OnComplete)
			OnComplete(FTryllError(ETryllErrorCode::VoiceInputNotFound, TEXT("VoiceInput handle not valid")));
		return;
	}
	bUtteranceActive = false;
	const auto Weak = CaptureTryllLive(this);
	Tryll::Client::Calls::EndUtterance(
		*Connection,
		this,
		VoiceInputId,
		[Weak, OnComplete = MoveTemp(OnComplete)](const FTryllError& Error)
		{
			if (auto Live = Weak.Pin())
			{
				if (OnComplete)
				{
					OnComplete(Error);
				}
			}
		},
		{});
}

void FTryllVoiceInput::CancelUtterance(TFunction<void(const FTryllError&)> OnComplete)
{
	if (!IsValid())
	{
		if (OnComplete)
			OnComplete(FTryllError(ETryllErrorCode::VoiceInputNotFound, TEXT("VoiceInput handle not valid")));
		return;
	}
	bUtteranceActive = false;
	const auto Weak = CaptureTryllLive(this);
	Tryll::Client::Calls::CancelUtterance(
		*Connection,
		this,
		VoiceInputId,
		[Weak, OnComplete = MoveTemp(OnComplete)](const FTryllError& Error)
		{
			if (auto Live = Weak.Pin())
			{
				if (OnComplete)
				{
					OnComplete(Error);
				}
			}
		},
		{});
}

void FTryllVoiceInput::SetOnTranscriptUpdate(TFunction<void(const FTryllTranscriptUpdate&)> Callback)
{
	OnTranscriptUpdate = MoveTemp(Callback);
}

void FTryllVoiceInput::SetOnError(TFunction<void(const FTryllError&)> Callback)
{
	OnError = MoveTemp(Callback);
}

void FTryllVoiceInput::ClearAfterDestroyRequest()
{
	VoiceInputId     = 0;
	bUtteranceActive = false;
}

void FTryllVoiceInput::DrainPendingAcks(const FTryllError& TerminalError)
{
	if (Connection.IsValid())
	{
		Connection->FailOwnedBy(this, TerminalError);
	}
	bUtteranceActive = false;
}

void FTryllVoiceInput::HandleTranscriptUpdate(
	const FString& Text, ETryllTranscriptUpdateKind Kind, int32 AudioMsConsumed)
{
	// Every UtteranceFinal closes the turn (non-empty speech final,
	// EndUtterance, or the max-utterance timeout). Silence heartbeats arrive
	// as empty SegmentFinals and do not close.
	if (Kind == ETryllTranscriptUpdateKind::UtteranceFinal)
	{
		bUtteranceActive = false;
	}
	if (OnTranscriptUpdate)
	{
		FTryllTranscriptUpdate Update;
		Update.Text            = Text;
		Update.Kind            = Kind;
		Update.AudioMsConsumed = AudioMsConsumed;
		OnTranscriptUpdate(Update);
	}
}

void FTryllVoiceInput::HandleError(const FTryllError& Error)
{
	if (OnError)
	{
		OnError(Error);
	}
}
