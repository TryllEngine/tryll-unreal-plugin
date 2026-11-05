// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "AgentLog/AgentLogProjection.h"

#include "AgentLog/AgentLogStore.h"
#include "Generated/TryllMessageDescriptors.gen.h"
#include "TryllConnection.h"

namespace AgentLogProjectionPrivate
{

using FRequestKey = TPair<uint64, uint64>;

FRequestKey RequestKey(const FTryllObservationEnvelope& Envelope)
{
	return FRequestKey(Envelope.ConnectionEpoch, Envelope.RequestId);
}

bool IsOutboundRequest(const FTryllObservationEnvelope& Envelope)
{
	return Envelope.Direction == ETryllObservationDirection::Outbound
		&& Envelope.Connection == ETryllConnectionObservation::None;
}

bool IsRequestCorrelatedAck(const FTryllObservationEnvelope& Envelope)
{
	// Joined onto the originating request row. Orphan replies still show.
	if (Envelope.Direction != ETryllObservationDirection::Inbound
		|| Envelope.Connection != ETryllConnectionObservation::None
		|| Envelope.RequestId == 0)
	{
		return false;
	}
	return ::Tryll::IsScopedAck(Envelope.RequestBody);
}

bool IsReceivedTurnEvent(const FTryllObservationEnvelope& Envelope)
{
	return Envelope.IsPush(::Tryll::MessageBody_AnswerText)
		|| Envelope.IsPush(::Tryll::MessageBody_TurnComplete)
		|| Envelope.IsReplyTo(::Tryll::MessageBody_SendMessageRequest, false)
		|| Envelope.IsPush(::Tryll::MessageBody_NodeEvent)
		|| Envelope.IsPush(::Tryll::MessageBody_TtsAudioFormatFrame)
		|| Envelope.IsPush(::Tryll::MessageBody_TtsAudioFrame)
		|| Envelope.IsPush(::Tryll::MessageBody_TtsVisemeFrame);
}

bool IsVoiceSentFinalTranscript(const FTryllObservationEnvelope& Envelope)
{
	return Envelope.IsPush(::Tryll::MessageBody_WireTranscriptUpdate)
		&& Envelope.TranscriptKind == static_cast<uint8>(ETryllTranscriptUpdateKind::UtteranceFinal)
		&& Envelope.CarrierRequestId != 0;
}

bool PassesAgentFilter(const FTryllObservationEnvelope& Envelope, const FAgentLogFilter& Filter)
{
	if (Envelope.bIsCaptureGap)
	{
		return true;
	}

	if (Filter.bAllAgents)
	{
		return true;
	}

	if (Filter.SingleAgentId.IsSet())
	{
		return Envelope.AgentId == Filter.SingleAgentId.GetValue();
	}

	return true;
}

bool PassesScopeFilter(const FTryllObservationEnvelope& Envelope, const FAgentLogFilter& Filter)
{
	if (Envelope.bIsCaptureGap)
	{
		return true;
	}

	if (Filter.bRawMode)
	{
		return Envelope.Scope == ETryllObservationScope::RawSession
			|| Envelope.Scope == ETryllObservationScope::CuratedAgent
			|| Envelope.Scope == ETryllObservationScope::MetadataOnly;
	}

	return Envelope.Scope == ETryllObservationScope::CuratedAgent;
}

bool IsFoldedVariableUpdate(const FTryllObservationEnvelope& Envelope)
{
	return Envelope.IsRequest(::Tryll::MessageBody_UpdateAgentVariablesRequest);
}

bool SameAnswerStream(const FAgentLogRow& Last, const FTryllObservationEnvelope& Envelope)
{
	if (Last.AgentId != Envelope.AgentId)
	{
		return false;
	}
	if (Last.TurnKey.bHasValue && Envelope.TurnKey.bHasValue)
	{
		return Last.TurnKey == Envelope.TurnKey;
	}
	return true;
}

FString AccumulateAnswerText(const FString& Existing, const FTryllObservationEnvelope& Envelope)
{
	if (Envelope.bFlag) // isDelta
	{
		return Existing + Envelope.Text;
	}
	return Envelope.Text.IsEmpty() ? Existing : Envelope.Text;
}

} // namespace AgentLogProjectionPrivate

TArray<FAgentLogRow> FAgentLogProjection::Project(
	const FAgentLogStore&  Store,
	const FAgentLogFilter& Filter)
{
	using namespace AgentLogProjectionPrivate;

	TMap<FRequestKey, TArray<FTryllObservationEnvelope>> DeferredVarsByCarrier;
	TMap<FRequestKey, FTryllObservationEnvelope>        AcksByRequest;
	TSet<FRequestKey> OriginatingRequests;

	Store.ForEachEnvelope(
		[&](const FTryllObservationEnvelope& Envelope)
		{
			if (IsOutboundRequest(Envelope) && Envelope.RequestId != 0)
			{
				OriginatingRequests.Add(RequestKey(Envelope));
			}

			if (IsFoldedVariableUpdate(Envelope) && Envelope.CarrierRequestId != 0)
			{
				const FRequestKey CarrierKey(Envelope.ConnectionEpoch, Envelope.CarrierRequestId);
				DeferredVarsByCarrier.FindOrAdd(CarrierKey).Add(Envelope);
				return;
			}

			if (IsRequestCorrelatedAck(Envelope))
			{
				AcksByRequest.Add(RequestKey(Envelope), Envelope);
			}
		});

	TArray<FAgentLogRow> Rows;
	Rows.Reserve(Store.GetEnvelopeCount());

	FTryllTurnKey PreviousTurnKey;

	Store.ForEachEnvelope(
		[&](const FTryllObservationEnvelope& Envelope)
		{
			if (!PassesScopeFilter(Envelope, Filter) || !PassesAgentFilter(Envelope, Filter))
			{
				return;
			}

			if (IsFoldedVariableUpdate(Envelope))
			{
				return;
			}

			if (Envelope.bIsCaptureGap)
			{
				FAgentLogRow Row;
				Row.Kind             = EAgentLogRowKind::CaptureGap;
				Row.EventId          = Envelope.EventId;
				Row.bIsCaptureGap    = true;
				Row.bShowAgentGutter = Filter.bAllAgents;
				Rows.Add(MoveTemp(Row));
				PreviousTurnKey = FTryllTurnKey{};
				return;
			}

			if (!IsOutboundRequest(Envelope))
			{
				// Hide ack rows only when the originating request is in the journal.
				if (IsRequestCorrelatedAck(Envelope)
					&& OriginatingRequests.Contains(RequestKey(Envelope)))
				{
					return;
				}

				if (Filter.bCoalesceAnswerText
					&& Envelope.IsPush(::Tryll::MessageBody_AnswerText)
					&& Rows.Num() > 0)
				{
					FAgentLogRow& Last = Rows.Last();
					if (Last.Kind == EAgentLogRowKind::Received
						&& Last.bIsAnswerTextRow
						&& SameAnswerStream(Last, Envelope))
					{
						FString Base = Last.OverrideSummary;
						if (Base.IsEmpty())
						{
							FTryllObservationEnvelope First;
							if (Store.TryGetEnvelope(Last.EventId, First))
							{
								Base = First.Text;
							}
						}
						Last.OverrideSummary = AccumulateAnswerText(Base, Envelope);
						// Keep Last.EventId as the first AnswerText id (Unity parity).
						return;
					}
				}
			}

			FAgentLogRow Row;
			Row.EventId          = Envelope.EventId;
			Row.AgentId          = Envelope.AgentId;
			Row.TurnKey          = Envelope.TurnKey;
			Row.bShowAgentGutter = Filter.bAllAgents;
			Row.bVoiceSentFinal  = IsVoiceSentFinalTranscript(Envelope);
			Row.bIsTurnTerminal = Envelope.IsPush(::Tryll::MessageBody_TurnComplete)
				|| Envelope.IsReplyTo(::Tryll::MessageBody_SendMessageRequest, false);

			if (Row.bVoiceSentFinal)
			{
				Row.Kind = EAgentLogRowKind::Sent;
			}
			else if (IsOutboundRequest(Envelope))
			{
				// Only turn-driving carriers are Sent. Create/Destroy/Cancel are System.
				if (Envelope.IsRequest(::Tryll::MessageBody_SendMessageRequest)
					|| Envelope.IsRequest(::Tryll::MessageBody_ResumeAgentRequest)
					|| Envelope.IsRequest(::Tryll::MessageBody_ChangeAgentParamRequest)
					|| Envelope.IsRequest(::Tryll::MessageBody_BeginUtteranceRequest))
				{
					Row.Kind = EAgentLogRowKind::Sent;
				}
				else
				{
					Row.Kind = EAgentLogRowKind::System;
				}
			}
			else
			{
				Row.Kind = IsReceivedTurnEvent(Envelope)
					? EAgentLogRowKind::Received
					: EAgentLogRowKind::System;
				Row.bIsAnswerTextRow = Envelope.IsPush(::Tryll::MessageBody_AnswerText);
			}

			if (Envelope.TurnKey.bHasValue
				&& (!PreviousTurnKey.bHasValue || !(Envelope.TurnKey == PreviousTurnKey)))
			{
				Row.bIsFirstRowOfTurn = true;
				PreviousTurnKey       = Envelope.TurnKey;
			}

			if (IsOutboundRequest(Envelope) && Envelope.RequestId != 0)
			{
				const FRequestKey SentKey = RequestKey(Envelope);

				if (const TArray<FTryllObservationEnvelope>* Deferred = DeferredVarsByCarrier.Find(SentKey))
				{
					Row.DeferredVariableUpdates = *Deferred;
					if (Deferred->Num() > 0 && (*Deferred)[0].RequestId != 0)
					{
						const FRequestKey DeferredAckKey(
							(*Deferred)[0].ConnectionEpoch,
							(*Deferred)[0].RequestId);
						if (const FTryllObservationEnvelope* DeferredAck = AcksByRequest.Find(DeferredAckKey))
						{
							Row.DeferredVariablesAck = *DeferredAck;
						}
					}
				}

				if (const FTryllObservationEnvelope* Ack = AcksByRequest.Find(SentKey))
				{
					Row.JoinedAck = *Ack;
				}
			}

			Rows.Add(MoveTemp(Row));
		});

	return Rows;
}
