// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "messages_generated.h"

enum class ETryllConnectionObservation : uint8
{
	None = 0,
	Connected,
	ConnectFailed,
	Disconnected,
};

enum class ETryllObservationDirection : uint8
{
	Outbound,
	Inbound,
};

enum class ETryllObservationScope : uint8
{
	CuratedAgent,
	RawSession,
	MetadataOnly,
};

struct TRYLLCLIENT_API FTryllTurnKey
{
	uint64 ConnectionEpoch = 0;
	uint64 AgentId         = 0;
	uint64 RequestId       = 0; // 0 means synthetic
	uint64 SyntheticSequence = 0;
	bool   bHasValue       = false;

	static FTryllTurnKey FromRequest(uint64 Epoch, uint64 InAgentId, uint64 InRequestId);
	static FTryllTurnKey FromSynthetic(uint64 Epoch, uint64 InAgentId, uint64 Seq);
	bool operator==(const FTryllTurnKey& Other) const;
};

struct TRYLLCLIENT_API FTryllObservationEnvelope
{
	uint64 EventId = 0;
	uint64 ConnectionEpoch = 0;
	ETryllObservationDirection Direction = ETryllObservationDirection::Outbound;
	ETryllObservationScope Scope = ETryllObservationScope::RawSession;
	FDateTime WallTimeUtc;
	double MonotonicSeconds = 0.0; // FPlatformTime::Seconds()
	::Tryll::MessageBody Body = ::Tryll::MessageBody_NONE;
	::Tryll::MessageBody RequestBody = ::Tryll::MessageBody_NONE;
	bool bIsError = false;
	ETryllConnectionObservation Connection = ETryllConnectionObservation::None;
	uint64 RequestId = 0;
	uint64 AgentId = 0;
	uint64 VoiceInputId = 0;
	FTryllTurnKey TurnKey;
	uint64 CarrierRequestId = 0;
	FString Text, Text2, Message, NodeName, BinaryKind, GraphDefaultModelName, PayloadSummary;
	bool bFlag = false, bFlag2 = false;
	uint8 TurnStatus = 0;
	uint32 TokensGenerated = 0;
	uint32 Code = 0;
	uint32 BinaryByteCount = 0;
	bool bEnableDiagnostics = false;
	bool bMaintainDialogueHistory = true;
	uint32 SampleRate = 0;
	uint16 Channels = 0, BitsPerSample = 0;
	float ThrottleLevel = 0.f;
	uint32 RemoveInteractionCount = 0;
	int32 VariableUpdateCount = 0;
	int32 DialogInteractionCount = 0;
	uint8 TranscriptKind = 0;
	uint32 AudioMsConsumed = 0;
	/** Synthetic marker inserted when capture resumes after Detach (Agent Log). */
	bool bIsCaptureGap = false;

	/** Stripped wire bytes, or a local LogLocalEnvelope. */
	TArray<uint8> Frame;
	bool bIsLocal = false;

	struct FOmittedField
	{
		FString Path;
		int64 Count = 0;
		int64 Bytes = 0;
	};
	TArray<FOmittedField> Omitted;

	[[nodiscard]] bool IsRequest(::Tryll::MessageBody Request) const
	{
		return Direction == ETryllObservationDirection::Outbound
			&& Connection == ETryllConnectionObservation::None
			&& Body == Request
			&& RequestBody == Request;
	}

	[[nodiscard]] bool IsReplyTo(::Tryll::MessageBody Request, bool bOk) const
	{
		return Direction == ETryllObservationDirection::Inbound
			&& Connection == ETryllConnectionObservation::None
			&& RequestBody == Request
			&& bIsError == !bOk;
	}

	[[nodiscard]] bool IsPush(::Tryll::MessageBody PushBody) const
	{
		return Direction == ETryllObservationDirection::Inbound
			&& Connection == ETryllConnectionObservation::None
			&& Body == PushBody;
	}
};

/** Semantic extras a wrapper attaches for one outbound frame or one inbound completion. */
struct TRYLLCLIENT_API FTryllObservationNote
{
	ETryllObservationDirection Direction = ETryllObservationDirection::Outbound;
	::Tryll::MessageBody Body = ::Tryll::MessageBody_NONE;
	::Tryll::MessageBody RequestBody = ::Tryll::MessageBody_NONE;
	bool bIsError = false;
	ETryllConnectionObservation Connection = ETryllConnectionObservation::None;
	uint64 RequestId = 0;
	uint64 AgentId = 0;
	uint64 VoiceInputId = 0;
	uint64 CarrierRequestId = 0;
	FString Text, Text2, Message, NodeName, BinaryKind, GraphDefaultModelName, PayloadSummary;
	bool bFlag = false, bFlag2 = false;
	uint8 TurnStatus = 0;
	uint32 TokensGenerated = 0;
	uint32 Code = 0;
	uint32 BinaryByteCount = 0;
	bool bEnableDiagnostics = false;
	bool bMaintainDialogueHistory = true;
	uint32 SampleRate = 0;
	uint16 Channels = 0, BitsPerSample = 0;
	float ThrottleLevel = 0.f;
	uint32 RemoveInteractionCount = 0;
	int32 VariableUpdateCount = 0;
	int32 DialogInteractionCount = 0;
	uint8 TranscriptKind = 0;
	uint32 AudioMsConsumed = 0;
};

TRYLLCLIENT_API uint32 GetTypeHash(const FTryllTurnKey& Key);
