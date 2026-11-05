// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllConnection.h"
#include "TryllObservationTypes.h"

enum class ETryllSentPayloadForm : uint8
{
	TypedText,
	ClientInjected,
	Resume,
	ChangeParams,
	OtherCommand,
};

/** One deferred variable flush batched onto a carrier request (R12.6). */
struct FTryllDeferredVariableBatch
{
	int32  UpdateCount      = 0;
	uint64 CarrierRequestId = 0;
	FString Summary;
};

/** Neutral sent-row inspector input (R12.5 / R12.6). */
struct FTryllSentInspection
{
	FString Title;
	FString Hint;

	::Tryll::MessageBody  CommandType  = ::Tryll::MessageBody_NONE;
	FString               CommandLabel;
	ETryllSentPayloadForm Form           = ETryllSentPayloadForm::OtherCommand;
	FString               PayloadSummary;
	FString               PayloadDetail;

	uint64    RequestId = 0;
	uint64    AgentId   = 0;
	FDateTime Timestamp;

	TArray<FTryllDeferredVariableBatch> DeferredVariableBatches;
	TOptional<FTryllObservationEnvelope> JoinedAck;

	/** True when the inspector carries detail beyond what the timeline row shows. */
	bool bHasDetailBeyondRow = false;

	/** Non-empty when the panel would otherwise look blank (R12.8). */
	FString AbsenceNote;
};

struct TRYLLCLIENTEDITOR_API FTryllSentInspectionFactory
{
	static FTryllSentInspection Empty(const FString& Hint = TEXT("select a row"));

	static FTryllSentInspection FromAgentLogRow(
		const FTryllObservationEnvelope&           Envelope,
		const TArray<FTryllObservationEnvelope>&    DeferredUpdates,
		const TOptional<FTryllObservationEnvelope>& JoinedAck);
};

