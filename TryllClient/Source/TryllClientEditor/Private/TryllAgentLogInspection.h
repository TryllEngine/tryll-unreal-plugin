// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "AgentLog/AgentDescriptor.h"
#include "AgentLog/AgentLogProjection.h"
#include "TryllSentInspection.h"
#include "TryllSystemInspection.h"
#include "TryllTurnInspectorModels.h"

class FAgentLogStore;

/** Pure helpers adapting the Agent Log journal to inspector models and timeline text. */
struct TRYLLCLIENTEDITOR_API FTryllAgentLogInspection
{
	/** Wire name from EnumNameMessageBody, or Connected / ConnectFailed / Disconnected. */
	static FString FormatWireLabel(const FTryllObservationEnvelope& Envelope);
	static FString FormatTimestamp(const FDateTime& WallTimeUtc);
	static FString FormatAgentGutterLabel(uint64 AgentId, const FAgentDescriptorRegistry& Descriptors);
	/** Dim status line under an agent name in the Agents list. */
	static FString FormatAgentListStatus(
		int32 CompletedTurns,
		bool bDestroyed,
		bool bLimitedDiagnostics,
		bool bPending);
	/** Dim status line under the All Agents entry. */
	static FString FormatAllAgentsStatus(int32 AgentCount, int32 CompletedTurns);

	static FString RowSummary(
		const FAgentLogRow&                  Row,
		const FTryllObservationEnvelope&     Envelope,
		const FAgentDescriptorRegistry&      Descriptors);

	static bool RowUsesDoubleHeight(const FString& Summary);

	static FTryllTurnInspection BuildTurnInspection(
		const FAgentLogRow&              Row,
		const FAgentLogStore&            Store,
		const FAgentDescriptorRegistry&  Descriptors);

	static FTryllSentInspection BuildSentInspection(const FAgentLogRow& Row, const FTryllObservationEnvelope& Envelope);

	static FTryllSystemInspection BuildSystemInspection(const FAgentLogRow& Row, const FTryllObservationEnvelope& Envelope);

	static bool InspectionNeverBlank(const FTryllTurnInspection& Turn);
	static bool InspectionNeverBlank(const FTryllSentInspection& Sent);
	static bool InspectionNeverBlank(const FTryllSystemInspection& System);

	static FAgentLogFilter MakeAllAgentsFilter(bool bRawMode);
	static FAgentLogFilter MakeSingleAgentFilter(uint64 AgentId, bool bRawMode);
};
