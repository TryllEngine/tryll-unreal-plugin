// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "AgentLog/TryllAgentLogWriteStore.h"

#include "AgentLog/AgentLogStore.h"
#include "TryllAgentLogDocument.h"

FString FTryllAgentLogWriteStore::WriteStore(const FAgentLogStore& Store, FTryllAgentLogExportInfo Info)
{
	TArray<FTryllObservationEnvelope> Events;
	Store.ForEachEnvelope([&Events](const FTryllObservationEnvelope& Env) { Events.Add(Env); });
	Info.EvictedTurns = Store.GetEvictedTurns();
	Info.EvictedObservations = Store.GetEvictedObservations();
	Info.bHistoryDiscarded = Store.IsHistoryDiscarded();
	const FAgentLogConfig Config = Store.GetConfig();
	Info.MaxTurns = Config.MaxCompletedTurnsPerAgent;
	Info.MaxObservations = Config.MaxObservationsPerAgent;
	Info.MaxRaw = Config.MaxRawSessionObservations;
	if (Info.Agents.Num() == 0)
	{
		for (const TPair<uint64, FAgentDescriptor>& Pair : Store.GetDescriptors().GetAll())
		{
			FTryllAgentLogExportAgent Agent;
			Agent.AgentId = Pair.Value.AgentId;
			Agent.Label = Pair.Value.Label;
			Agent.bDiagnostics = Pair.Value.bDiagnosticsAvailable;
			Agent.bCreatedBeforeDiagnosticsCapture = Pair.Value.bCreatedBeforeDiagnosticsCapture;
			Agent.CompletedTurns = Store.GetCompletedTurnCount(Pair.Key);
			Info.Agents.Add(Agent);
		}
	}
	return FTryllAgentLogDocument::Write(Events, Info);
}
