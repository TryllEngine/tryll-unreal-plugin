// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "AgentLog/AgentDescriptor.h"
#include "AgentLog/AgentLogConfig.h"
#include "Containers/List.h"
#include "TryllClientObserver.h"
#include "TryllDiagnosticsDemand.h"
#include "TryllObservationCorrelator.h"

class FTryllConnection;
class UTryllEditorSubsystem;

DECLARE_MULTICAST_DELEGATE(FOnAgentLogJournalChanged);

/**
 * Editor-module Agent Log journal and retention service (Package 2 Phase 3).
 * One chronological linked-list journal with secondary indexes; UI filters project it.
 * Eviction unlinks turn/raw nodes in O(turn size) without rebuilding indexes.
 */
class TRYLLCLIENTEDITOR_API FAgentLogStore final : public ITryllClientObserver, public TSharedFromThis<FAgentLogStore>
{
public:
	using FJournalList = TDoubleLinkedList<FTryllObservationEnvelope>;
	using FJournalNode = FJournalList::TDoubleLinkedListNode;

	FAgentLogStore();

	void Attach(UTryllEditorSubsystem* EditorSubsystem);
	void Attach(const TSharedPtr<FTryllConnection>& Connection,
		TFunction<TArray<FTryllRegisteredAgentSnapshot>()> SnapshotAgents);
	void Detach();
	void Clear();

	void OnObservation(const FTryllObservationEnvelope& Envelope) override;

	/** Automation / unit tests without a live server connection. */
	void AttachForTests();
	void AppendForTests(FTryllObservationEnvelope Envelope);
	void SnapshotAgentsForTests(const TArray<FTryllRegisteredAgentSnapshot>& Snapshots);
	/** Re-apply live DisplayName / LabelHint onto descriptors (e.g. after CreateAgentOk). */
	void RefreshAgentLabels();

	int32 GetEnvelopeCount() const { return Journal.Num(); }
	bool  TryGetEnvelope(uint64 EventId, FTryllObservationEnvelope& Out) const;
	const FTryllObservationEnvelope* FindEnvelope(uint64 EventId) const;
	void  ForEachEnvelope(TFunctionRef<void(const FTryllObservationEnvelope&)> Fn) const;

	/** Completed turns retained for an agent (retention FIFO length). */
	int32 GetCompletedTurnCount(uint64 AgentId) const;
	/** Sum of completed turns across all agents (All Agents status line). */
	int32 GetTotalCompletedTurnCount() const;

	const FAgentDescriptorRegistry& GetDescriptors() const { return Descriptors; }
	FAgentLogConfig                 GetConfig() const { return Config; }
	void                            SetConfig(const FAgentLogConfig& InConfig) { Config = InConfig; }

	bool IsAttached() const { return bAttached; }
	/** True when subscribed to a live connection ObserverHub (can receive observations). */
	bool IsSubscribed() const;
	/** True when subscribed specifically to Conn (detects stale hub after reconnect). */
	bool IsSubscribedTo(const TSharedPtr<FTryllConnection>& Conn) const;
	bool IsHistoryDiscarded() const { return bHistoryDiscardedByReload; }
	int32 GetEvictedTurns() const { return EvictedTurns; }
	int32 GetEvictedObservations() const { return EvictedObservations; }
	void MarkHistoryDiscardedByReload();

	FOnAgentLogJournalChanged OnJournalChanged;

private:
	struct FTurnRetentionMeta
	{
		uint64                 AgentId = 0;
		TArray<FJournalNode*>  MemberNodes;
		bool                   bComplete = false;
		/** Append order among completed turns (for oldest-first eviction). */
		uint64                 Sequence = 0;
	};

	using FRequestKey = TPair<uint64, uint64>;

	void AppendInternal(FTryllObservationEnvelope Envelope);
	void CorrelateAndAppend(FTryllObservationEnvelope Envelope);
	uint64 AllocateEventId();
	void InsertCaptureGapMarker(::Tryll::LogGapReason Reason = ::Tryll::LogGapReason_paused);
	void SnapshotAgentsIntoDescriptors(TFunction<TArray<FTryllRegisteredAgentSnapshot>()> SnapshotAgents);
	void IndexNode(FJournalNode* Node);
	void UnlinkNode(FJournalNode* Node);
	void RemoveTurnNodes(const FTryllTurnKey& TurnKey);
	void EvictIfNeeded();
	void NotifyChanged();

	static bool IsTurnTerminalEvent(const FTryllObservationEnvelope& Envelope);
	static bool CountsTowardAgentRetention(const FTryllObservationEnvelope& Envelope);
	static bool CountsTowardRawRetention(const FTryllObservationEnvelope& Envelope);
	static FRequestKey RequestKey(const FTryllObservationEnvelope& Envelope);

	FAgentLogConfig Config;
	FJournalList Journal;
	FAgentDescriptorRegistry Descriptors;

	TMap<uint64, FJournalNode*> NodesByEventId;
	TMap<uint64, int32> AgentScopedCounts;
	TMap<FTryllTurnKey, FTurnRetentionMeta> TurnMetaByKey;
	/** Oldest-complete-first FIFO per agent (RemoveAt(0) on eviction). */
	TMap<uint64, TArray<FTryllTurnKey>> CompletedTurnsByAgent;
	/** Oldest-raw-first FIFO (RemoveAt(0) on eviction). */
	TArray<FJournalNode*> RawSessionNodes;
	uint64 NextCompletedTurnSequence = 1;

	TSharedPtr<FTryllConnection> AttachedConnection;
	TWeakObjectPtr<UTryllEditorSubsystem> AttachedEditorSubsystem;
	FDelegateHandle ObserverHandle;
	TSharedPtr<FTryllDiagnosticsDemand::FLease> DiagnosticsLease;
	TFunction<TArray<FTryllRegisteredAgentSnapshot>()> SnapshotAgentsFn;

	FTryllObservationCorrelator Correlator;
	uint64 CorrelatorEpoch = 0;

	bool bAttached = false;
	bool bHistoryDiscardedByReload = false;
	bool bHadPriorAttach = false;
	uint64 FallbackEventId = 1;
	/** Store-wide id watermark; live observer hubs restart their counters on reconnect. */
	uint64 LastJournalEventId = 0;
	int32 EvictedTurns = 0;
	int32 EvictedObservations = 0;
};
