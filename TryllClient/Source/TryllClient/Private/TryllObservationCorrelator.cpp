// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllObservationCorrelator.h"
#include "Generated/TryllMessageDescriptors.gen.h"
#include "TryllConnection.h"

namespace
{

bool IsSyntheticTurn(const FTryllObservationEnvelope& Envelope)
{
	return Envelope.IsPush(::Tryll::MessageBody_AnswerText)
		|| Envelope.IsPush(::Tryll::MessageBody_TurnComplete)
		|| Envelope.IsReplyTo(::Tryll::MessageBody_SendMessageRequest, false)
		|| Envelope.IsPush(::Tryll::MessageBody_TtsAudioFormatFrame)
		|| Envelope.IsPush(::Tryll::MessageBody_TtsAudioFrame)
		|| Envelope.IsPush(::Tryll::MessageBody_TtsVisemeFrame)
		|| Envelope.IsPush(::Tryll::MessageBody_NodeEvent);
}

bool IsSyntheticTerminal(const FTryllObservationEnvelope& Envelope)
{
	return Envelope.IsPush(::Tryll::MessageBody_TurnComplete)
		|| Envelope.IsReplyTo(::Tryll::MessageBody_SendMessageRequest, false);
}

bool IsStandaloneRequestTurn(const FTryllObservationEnvelope& Envelope)
{
	return Envelope.Direction == ETryllObservationDirection::Outbound
		&& Envelope.Connection == ETryllConnectionObservation::None
		&& Envelope.Body == Envelope.RequestBody
		&& ::Tryll::IsStandaloneTurnRequest(Envelope.Body);
}

bool IsCorrelatedReply(const FTryllObservationEnvelope& Envelope)
{
	return Envelope.Direction == ETryllObservationDirection::Inbound
		&& ::Tryll::IsScopedAck(Envelope.RequestBody);
}

} // namespace

void FTryllObservationCorrelator::ResetForNewEpoch(uint64 Epoch)
{
	CurrentEpoch            = Epoch;
	OpenSyntheticByAgent.Empty();
	NextSyntheticSeqByAgent.Empty();
	RequestTurnKeys.Empty();
	RequestStamps.Empty();
	OpenLogicalTurnByAgent.Empty();
}

FTryllTurnKey FTryllObservationCorrelator::TurnKeyForRequest(uint64 AgentId, uint64 RequestId) const
{
	if (RequestId == 0 || AgentId == 0)
	{
		return FTryllTurnKey{};
	}
	return FTryllTurnKey::FromRequest(CurrentEpoch, AgentId, RequestId);
}

FTryllTurnKey FTryllObservationCorrelator::ReserveSyntheticTurn(uint64 AgentId)
{
	if (AgentId == 0)
	{
		return FTryllTurnKey{};
	}

	uint64& NextSeq = NextSyntheticSeqByAgent.FindOrAdd(AgentId, 1);
	const FTryllTurnKey Key = FTryllTurnKey::FromSynthetic(CurrentEpoch, AgentId, NextSeq);
	++NextSeq;

	FOpenSyntheticTurn Open;
	Open.TurnKey = Key;
	OpenSyntheticByAgent.Add(AgentId, MoveTemp(Open));
	return Key;
}

FTryllTurnKey FTryllObservationCorrelator::OpenOrGetSyntheticTurn(uint64 AgentId)
{
	if (AgentId == 0)
	{
		return FTryllTurnKey{};
	}

	if (const FOpenSyntheticTurn* Open = OpenSyntheticByAgent.Find(AgentId))
	{
		return Open->TurnKey;
	}

	uint64& NextSeq = NextSyntheticSeqByAgent.FindOrAdd(AgentId, 1);
	const FTryllTurnKey Key = FTryllTurnKey::FromSynthetic(CurrentEpoch, AgentId, NextSeq);
	++NextSeq;

	FOpenSyntheticTurn Open;
	Open.TurnKey = Key;
	OpenSyntheticByAgent.Add(AgentId, MoveTemp(Open));
	return Key;
}

void FTryllObservationCorrelator::CloseSyntheticTurn(uint64 AgentId)
{
	OpenSyntheticByAgent.Remove(AgentId);
}

void FTryllObservationCorrelator::OpenLogicalTurn(FTryllObservationEnvelope& Envelope)
{
	if (Envelope.RequestId == 0 || Envelope.AgentId == 0)
	{
		return;
	}

	const FTryllTurnKey Key = TurnKeyForRequest(Envelope.AgentId, Envelope.RequestId);
	Envelope.TurnKey = Key;
	RequestTurnKeys.Add(Envelope.RequestId, Key);
	OpenLogicalTurnByAgent.Add(Envelope.AgentId, Key);
}

void FTryllObservationCorrelator::ContinueOrOpenLogicalTurn(FTryllObservationEnvelope& Envelope)
{
	if (Envelope.RequestId == 0 || Envelope.AgentId == 0)
	{
		return;
	}

	if (const FTryllTurnKey* Open = OpenLogicalTurnByAgent.Find(Envelope.AgentId))
	{
		if (Open->bHasValue)
		{
			Envelope.TurnKey = *Open;
			RequestTurnKeys.Add(Envelope.RequestId, *Open);
			return;
		}
	}

	OpenLogicalTurn(Envelope);
}

void FTryllObservationCorrelator::RecordOutboundTurnStart(FTryllObservationEnvelope& Envelope)
{
	if (Envelope.RequestId == 0 || Envelope.AgentId == 0)
	{
		return;
	}

	const FTryllTurnKey Key = TurnKeyForRequest(Envelope.AgentId, Envelope.RequestId);
	Envelope.TurnKey = Key;
	RequestTurnKeys.Add(Envelope.RequestId, Key);
}

void FTryllObservationCorrelator::CloseOpenTurns(uint64 AgentId, const FTryllTurnKey& ClosedKey)
{
	if (const FTryllTurnKey* Open = OpenLogicalTurnByAgent.Find(AgentId))
	{
		if (*Open == ClosedKey)
		{
			OpenLogicalTurnByAgent.Remove(AgentId);
		}
	}
	CloseSyntheticTurn(AgentId);
}

void FTryllObservationCorrelator::ApplyStoredTurnKey(FTryllObservationEnvelope& Envelope)
{
	if (Envelope.RequestId == 0)
	{
		return;
	}
	if (const FTryllTurnKey* Found = RequestTurnKeys.Find(Envelope.RequestId))
	{
		Envelope.TurnKey = *Found;
	}
}

void FTryllObservationCorrelator::AssignTurnEventKey(FTryllObservationEnvelope& Envelope)
{
	if (Envelope.RequestId != 0)
	{
		ApplyStoredTurnKey(Envelope);
		if (!Envelope.TurnKey.bHasValue)
		{
			Envelope.TurnKey = TurnKeyForRequest(Envelope.AgentId, Envelope.RequestId);
		}
		return;
	}

	if (const FTryllTurnKey* Open = OpenLogicalTurnByAgent.Find(Envelope.AgentId))
	{
		if (Open->bHasValue)
		{
			Envelope.TurnKey = *Open;
			return;
		}
	}

	Envelope.TurnKey = OpenOrGetSyntheticTurn(Envelope.AgentId);
}

void FTryllObservationCorrelator::CorrelateOutboundCmd(FTryllObservationEnvelope& Envelope)
{
	if (Envelope.IsRequest(::Tryll::MessageBody_UpdateAgentVariablesRequest))
	{
		if (Envelope.CarrierRequestId != 0 && Envelope.AgentId != 0)
		{
			const FTryllTurnKey CarrierKey =
				TurnKeyForRequest(Envelope.AgentId, Envelope.CarrierRequestId);
			Envelope.TurnKey = CarrierKey;
			if (Envelope.RequestId != 0)
			{
				RequestTurnKeys.Add(Envelope.RequestId, CarrierKey);
			}
		}
		else if (Envelope.RequestId != 0)
		{
			ApplyStoredTurnKey(Envelope);
		}
		return;
	}

	if (Envelope.IsRequest(::Tryll::MessageBody_SendMessageRequest))
	{
		OpenLogicalTurn(Envelope);
		return;
	}

	if (Envelope.IsRequest(::Tryll::MessageBody_ResumeAgentRequest))
	{
		// Tool-call Resume continues the in-flight Send turn.
		ContinueOrOpenLogicalTurn(Envelope);
		return;
	}

	if (IsStandaloneRequestTurn(Envelope))
	{
		RecordOutboundTurnStart(Envelope);
	}
}

void FTryllObservationCorrelator::CorrelateInboundEvent(FTryllObservationEnvelope& Envelope)
{
	if (IsCorrelatedReply(Envelope))
	{
		ApplyStoredTurnKey(Envelope);
		return;
	}

	if (IsSyntheticTurn(Envelope) && Envelope.AgentId != 0)
	{
		AssignTurnEventKey(Envelope);

		if (IsSyntheticTerminal(Envelope))
		{
			CloseOpenTurns(Envelope.AgentId, Envelope.TurnKey);
		}
		return;
	}

	if (Envelope.AgentId != 0 && Envelope.RequestId != 0)
	{
		ApplyStoredTurnKey(Envelope);
		if (!Envelope.TurnKey.bHasValue)
		{
			Envelope.TurnKey = TurnKeyForRequest(Envelope.AgentId, Envelope.RequestId);
		}
		return;
	}

	if (Envelope.AgentId != 0)
	{
		if (const FTryllTurnKey* Open = OpenLogicalTurnByAgent.Find(Envelope.AgentId))
		{
			Envelope.TurnKey = *Open;
		}
		else if (const FOpenSyntheticTurn* Synthetic = OpenSyntheticByAgent.Find(Envelope.AgentId))
		{
			Envelope.TurnKey = Synthetic->TurnKey;
		}
	}
}

void FTryllObservationCorrelator::StampRequest(const FTryllObservationEnvelope& Envelope)
{
	if (Envelope.Direction != ETryllObservationDirection::Outbound || Envelope.RequestId == 0 || Envelope.bIsLocal)
	{
		return;
	}
	if (Envelope.Body == ::Tryll::MessageBody_NONE || Envelope.Body != Envelope.RequestBody)
	{
		return;
	}
	FRequestStamp Stamp;
	Stamp.Body = Envelope.Body;
	Stamp.AgentId = Envelope.AgentId;
	RequestStamps.Add(Envelope.RequestId, Stamp);
}

void FTryllObservationCorrelator::BindReply(FTryllObservationEnvelope& Envelope)
{
	if (Envelope.Direction != ETryllObservationDirection::Inbound || Envelope.RequestId == 0)
	{
		return;
	}
	if (Envelope.Connection != ETryllConnectionObservation::None)
	{
		return;
	}
	const bool bGeneric = Envelope.Body == ::Tryll::MessageBody_Ack
		|| Envelope.Body == ::Tryll::MessageBody_ErrorResponse
		|| Envelope.bIsLocal;
	if (!bGeneric)
	{
		return;
	}
	const FRequestStamp* Stamp = RequestStamps.Find(Envelope.RequestId);
	if (Stamp == nullptr)
	{
		return;
	}
	if (Envelope.RequestBody == ::Tryll::MessageBody_NONE || Envelope.RequestBody == Envelope.Body)
	{
		Envelope.RequestBody = Stamp->Body;
	}
	if (Envelope.AgentId == 0)
	{
		Envelope.AgentId = Stamp->AgentId;
	}
	if (Envelope.Body == ::Tryll::MessageBody_ErrorResponse || Envelope.bIsLocal)
	{
		Envelope.bIsError = true;
	}
}

void FTryllObservationCorrelator::Correlate(FTryllObservationEnvelope& Envelope)
{
	Envelope.ConnectionEpoch = CurrentEpoch;
	StampRequest(Envelope);
	BindReply(Envelope);

	if (Envelope.Direction == ETryllObservationDirection::Outbound
		&& Envelope.Connection == ETryllConnectionObservation::None)
	{
		CorrelateOutboundCmd(Envelope);
	}
	else
	{
		CorrelateInboundEvent(Envelope);
	}
}
