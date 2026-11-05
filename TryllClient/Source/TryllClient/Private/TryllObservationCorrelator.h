// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "TryllObservationTypes.h"

class TRYLLCLIENT_API FTryllObservationCorrelator
{
public:
	void ResetForNewEpoch(uint64 Epoch);
	void Correlate(FTryllObservationEnvelope& Envelope);

private:
	struct FOpenSyntheticTurn
	{
		FTryllTurnKey TurnKey;
	};

	uint64 CurrentEpoch = 0;

	TMap<uint64, FOpenSyntheticTurn> OpenSyntheticByAgent;
	TMap<uint64, uint64>             NextSyntheticSeqByAgent;
	TMap<uint64, FTryllTurnKey>      RequestTurnKeys;
	struct FRequestStamp
	{
		::Tryll::MessageBody Body = ::Tryll::MessageBody_NONE;
		uint64 AgentId = 0;
	};
	TMap<uint64, FRequestStamp>      RequestStamps;
	/** Open Send/Resume conversation turn per agent — closed on TurnComplete/SendErr. */
	TMap<uint64, FTryllTurnKey>      OpenLogicalTurnByAgent;

	FTryllTurnKey TurnKeyForRequest(uint64 AgentId, uint64 RequestId) const;
	FTryllTurnKey ReserveSyntheticTurn(uint64 AgentId);
	FTryllTurnKey OpenOrGetSyntheticTurn(uint64 AgentId);
	void          CloseSyntheticTurn(uint64 AgentId);
	void          OpenLogicalTurn(FTryllObservationEnvelope& Envelope);
	void          ContinueOrOpenLogicalTurn(FTryllObservationEnvelope& Envelope);
	void          RecordOutboundTurnStart(FTryllObservationEnvelope& Envelope);
	void          CloseOpenTurns(uint64 AgentId, const FTryllTurnKey& ClosedKey);
	void          StampRequest(const FTryllObservationEnvelope& Envelope);
	void          BindReply(FTryllObservationEnvelope& Envelope);
	void          CorrelateOutboundCmd(FTryllObservationEnvelope& Envelope);
	void          CorrelateInboundEvent(FTryllObservationEnvelope& Envelope);
	void          ApplyStoredTurnKey(FTryllObservationEnvelope& Envelope);
	void          AssignTurnEventKey(FTryllObservationEnvelope& Envelope);
};
