// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllObservationTypes.h"

class FAgentLogStore;

enum class EAgentLogRowKind : uint8
{
	Sent,
	Received,
	System,
	CaptureGap,
};

struct FAgentLogRow
{
	EAgentLogRowKind Kind = EAgentLogRowKind::System;
	/** Stable row identity — first observation EventId (kept across AnswerText coalesce). */
	uint64           EventId = 0;
	uint64           AgentId = 0;
	FTryllTurnKey    TurnKey;
	bool             bIsFirstRowOfTurn = false;
	bool             bShowAgentGutter  = false;
	bool             bIsCaptureGap     = false;
	bool             bIsTurnTerminal   = false;
	bool             bCanCollapseTurn  = false;
	bool             bIsTurnCollapsed  = false;
	int32            HiddenBodyCount   = 0;
	/** True when this row is (or coalesces) AnswerText — used by stream coalesce. */
	bool             bIsAnswerTextRow  = false;
	/** When set, timeline uses this instead of summarizing the source envelope. */
	FString          OverrideSummary;
	/** Joined in projection — journal is never mutated. */
	TOptional<FTryllObservationEnvelope> JoinedAck;
	TOptional<FTryllObservationEnvelope> DeferredVariablesAck;
	TArray<FTryllObservationEnvelope>    DeferredVariableUpdates;
	bool                                 bVoiceSentFinal = false;
};

struct FAgentLogFilter
{
	bool              bRawMode = false;
	bool              bAllAgents = false;
	bool              bCoalesceAnswerText = true;
	TOptional<uint64> SingleAgentId;
};

class TRYLLCLIENTEDITOR_API FAgentLogProjection
{
public:
	static TArray<FAgentLogRow> Project(
		const FAgentLogStore&  Store,
		const FAgentLogFilter& Filter);
};
