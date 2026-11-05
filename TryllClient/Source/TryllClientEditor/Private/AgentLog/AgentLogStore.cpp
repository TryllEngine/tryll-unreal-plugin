// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "AgentLog/AgentLogStore.h"

#include "TryllConnection.h"
#include "TryllEditorSubsystem.h"

namespace AgentLogStorePrivate
{
static const TCHAR* ReloadDiscardConfigSection = TEXT("/Script/TryllClientEditor.AgentLog");
static const TCHAR* ReloadDiscardConfigKey     = TEXT("bHistoryDiscardedByReload");
} // namespace AgentLogStorePrivate

FAgentLogStore::FAgentLogStore()
{
	Config = FAgentLogConfig::Load();
}

void FAgentLogStore::Attach(UTryllEditorSubsystem* EditorSubsystem)
{
	if (!EditorSubsystem)
	{
		return;
	}

	AttachedEditorSubsystem = EditorSubsystem;
	Attach(
		EditorSubsystem->GetSharedConnectionHandle(),
		[EditorSubsystem]()
		{
			return EditorSubsystem->SnapshotRegisteredAgents();
		});
}

void FAgentLogStore::Attach(
	const TSharedPtr<FTryllConnection>&                Connection,
	TFunction<TArray<FTryllRegisteredAgentSnapshot>()> SnapshotAgents)
{
	if (bAttached)
	{
		Detach();
	}

	AttachedConnection = Connection;
	SnapshotAgentsFn   = MoveTemp(SnapshotAgents);

	// No shared connection yet (Agent Log opened before Test Chat / PIE). Stay
	// unattached — the panel re-calls Attach when the live connection appears.
	if (!Connection.IsValid())
	{
		SnapshotAgentsFn = nullptr;
		return;
	}

	if (bHadPriorAttach)
	{
		InsertCaptureGapMarker();
	}

	TSharedRef<FAgentLogStore> Self = AsShared();
	Correlator.ResetForNewEpoch(Connection->GetObserverHub().GetConnectionEpoch());
	CorrelatorEpoch = Connection->GetObserverHub().GetConnectionEpoch();
	ObserverHandle = Connection->GetObserverHub().Subscribe(Self);
	DiagnosticsLease = Connection->GetDiagnosticsDemand()->Acquire();

	if (SnapshotAgentsFn)
	{
		SnapshotAgentsIntoDescriptors(SnapshotAgentsFn);
	}

	bAttached       = true;
	bHadPriorAttach = true;
}

void FAgentLogStore::Detach()
{
	if (AttachedConnection.IsValid() && ObserverHandle.IsValid())
	{
		AttachedConnection->GetObserverHub().Unsubscribe(ObserverHandle);
	}

	ObserverHandle.Reset();
	DiagnosticsLease.Reset();
	AttachedConnection.Reset();
	AttachedEditorSubsystem.Reset();
	SnapshotAgentsFn = nullptr;
	bAttached        = false;
}

bool FAgentLogStore::IsSubscribed() const
{
	return ObserverHandle.IsValid() && AttachedConnection.IsValid();
}

bool FAgentLogStore::IsSubscribedTo(const TSharedPtr<FTryllConnection>& Conn) const
{
	return IsSubscribed() && Conn.IsValid() && AttachedConnection.Get() == Conn.Get();
}

void FAgentLogStore::Clear()
{
	NodesByEventId.Empty();
	AgentScopedCounts.Empty();
	TurnMetaByKey.Empty();
	CompletedTurnsByAgent.Empty();
	RawSessionNodes.Empty();
	Journal.Empty();
	Descriptors.Clear();
	Correlator.ResetForNewEpoch(0);
	CorrelatorEpoch = 0;
	NextCompletedTurnSequence = 1;
	bHistoryDiscardedByReload = false;
	bHadPriorAttach           = false;
	EvictedTurns              = 0;
	EvictedObservations       = 0;
	NotifyChanged();
}

void FAgentLogStore::MarkHistoryDiscardedByReload()
{
	bHistoryDiscardedByReload = true;
}

void FAgentLogStore::AttachForTests()
{
	if (bAttached)
	{
		Detach();
	}

	if (bHadPriorAttach)
	{
		InsertCaptureGapMarker();
	}

	bAttached       = true;
	bHadPriorAttach = true;
}

void FAgentLogStore::AppendForTests(FTryllObservationEnvelope Envelope)
{
	if (Envelope.EventId == 0)
	{
		Envelope.EventId = FallbackEventId++;
	}
	CorrelateAndAppend(MoveTemp(Envelope));
}

void FAgentLogStore::SnapshotAgentsForTests(const TArray<FTryllRegisteredAgentSnapshot>& Snapshots)
{
	Descriptors.SnapshotRegisteredAgents(Snapshots);
}

void FAgentLogStore::RefreshAgentLabels()
{
	if (SnapshotAgentsFn)
	{
		SnapshotAgentsIntoDescriptors(SnapshotAgentsFn);
	}
}

int32 FAgentLogStore::GetCompletedTurnCount(const uint64 AgentId) const
{
	if (const TArray<FTryllTurnKey>* Turns = CompletedTurnsByAgent.Find(AgentId))
	{
		return Turns->Num();
	}
	return 0;
}

int32 FAgentLogStore::GetTotalCompletedTurnCount() const
{
	int32 Total = 0;
	for (const TPair<uint64, TArray<FTryllTurnKey>>& Pair : CompletedTurnsByAgent)
	{
		Total += Pair.Value.Num();
	}
	return Total;
}

void FAgentLogStore::OnObservation(const FTryllObservationEnvelope& Envelope)
{
	FTryllObservationEnvelope Copy = Envelope;
	CorrelateAndAppend(MoveTemp(Copy));
}

uint64 FAgentLogStore::AllocateEventId()
{
	if (AttachedConnection.IsValid())
	{
		return AttachedConnection->GetObserverHub().AllocateEventId();
	}
	return FallbackEventId++;
}

void FAgentLogStore::CorrelateAndAppend(FTryllObservationEnvelope Envelope)
{
	if (Envelope.ConnectionEpoch != CorrelatorEpoch)
	{
		if (CorrelatorEpoch != 0 && !Envelope.bIsCaptureGap)
		{
			InsertCaptureGapMarker(::Tryll::LogGapReason_epoch_change);
		}
		Correlator.ResetForNewEpoch(Envelope.ConnectionEpoch);
		CorrelatorEpoch = Envelope.ConnectionEpoch;
	}
	Correlator.Correlate(Envelope);
	AppendInternal(MoveTemp(Envelope));
}

FAgentLogStore::FRequestKey FAgentLogStore::RequestKey(const FTryllObservationEnvelope& Envelope)
{
	return FRequestKey(Envelope.ConnectionEpoch, Envelope.RequestId);
}

bool FAgentLogStore::IsTurnTerminalEvent(const FTryllObservationEnvelope& Envelope)
{
	return Envelope.IsPush(::Tryll::MessageBody_TurnComplete)
		|| Envelope.IsReplyTo(::Tryll::MessageBody_SendMessageRequest, false);
}

bool FAgentLogStore::CountsTowardAgentRetention(const FTryllObservationEnvelope& Envelope)
{
	if (Envelope.bIsCaptureGap)
	{
		return false;
	}

	if (Envelope.Scope == ETryllObservationScope::MetadataOnly)
	{
		return false;
	}

	return Envelope.Scope == ETryllObservationScope::CuratedAgent && Envelope.AgentId != 0;
}

bool FAgentLogStore::CountsTowardRawRetention(const FTryllObservationEnvelope& Envelope)
{
	if (Envelope.bIsCaptureGap)
	{
		return false;
	}

	return Envelope.Scope == ETryllObservationScope::RawSession;
}

void FAgentLogStore::InsertCaptureGapMarker(::Tryll::LogGapReason Reason)
{
	FTryllObservationEnvelope Gap;
	Gap.EventId          = AllocateEventId();
	Gap.bIsCaptureGap    = true;
	Gap.bIsLocal         = true;
	Gap.Scope           = ETryllObservationScope::RawSession;
	Gap.WallTimeUtc     = FDateTime::UtcNow();
	Gap.MonotonicSeconds = FPlatformTime::Seconds();
	Gap.Frame            = FTryllClientObserverHub::MakeGap(Reason);
	Gap.Message          = TEXT("capture-gap");
	AppendInternal(MoveTemp(Gap));
}

void FAgentLogStore::SnapshotAgentsIntoDescriptors(
	TFunction<TArray<FTryllRegisteredAgentSnapshot>()> SnapshotAgents)
{
	if (!SnapshotAgents)
	{
		return;
	}

	Descriptors.SnapshotRegisteredAgents(SnapshotAgents());
}

bool FAgentLogStore::TryGetEnvelope(const uint64 EventId, FTryllObservationEnvelope& Out) const
{
	if (const FTryllObservationEnvelope* Found = FindEnvelope(EventId))
	{
		Out = *Found;
		return true;
	}
	return false;
}

const FTryllObservationEnvelope* FAgentLogStore::FindEnvelope(const uint64 EventId) const
{
	if (EventId == 0)
	{
		return nullptr;
	}
	if (FJournalNode* const* NodePtr = NodesByEventId.Find(EventId))
	{
		if (*NodePtr != nullptr)
		{
			return &(*NodePtr)->GetValue();
		}
	}
	return nullptr;
}

void FAgentLogStore::ForEachEnvelope(TFunctionRef<void(const FTryllObservationEnvelope&)> Fn) const
{
	for (const FJournalNode* Node = Journal.GetHead(); Node != nullptr; Node = Node->GetNextNode())
	{
		Fn(Node->GetValue());
	}
}

void FAgentLogStore::AppendInternal(FTryllObservationEnvelope Envelope)
{
	// Keep bHistoryDiscardedByReload sticky until Clear() so the banner survives resume.
	//
	// Event ids come from the connection-owned observer hub. A new shared connection
	// starts that hub's counter over, while this store intentionally retains its journal.
	// Normalize at the persistence boundary so EventId remains unique/monotonic and the
	// EventId -> node index cannot make old timeline rows resolve to new-session payloads.
	if (Envelope.EventId == 0 || Envelope.EventId <= LastJournalEventId)
	{
		Envelope.EventId = LastJournalEventId + 1;
	}
	LastJournalEventId = Envelope.EventId;
	FallbackEventId = FMath::Max(FallbackEventId, LastJournalEventId + 1);

	Journal.AddTail(MoveTemp(Envelope));
	FJournalNode* Node = Journal.GetTail();
	check(Node != nullptr);
	IndexNode(Node);
	Descriptors.ObserveEnvelope(Node->GetValue());
	EvictIfNeeded();
	NotifyChanged();
}

void FAgentLogStore::IndexNode(FJournalNode* Node)
{
	check(Node != nullptr);
	const FTryllObservationEnvelope& Envelope = Node->GetValue();

	if (Envelope.EventId != 0)
	{
		NodesByEventId.Add(Envelope.EventId, Node);
	}

	if (CountsTowardAgentRetention(Envelope))
	{
		++AgentScopedCounts.FindOrAdd(Envelope.AgentId);
	}

	if (CountsTowardRawRetention(Envelope))
	{
		RawSessionNodes.Add(Node);
	}

	if (Envelope.TurnKey.bHasValue)
	{
		FTurnRetentionMeta& Meta = TurnMetaByKey.FindOrAdd(Envelope.TurnKey);
		if (Meta.MemberNodes.Num() == 0)
		{
			Meta.AgentId = Envelope.AgentId;
		}
		Meta.MemberNodes.Add(Node);

		if (IsTurnTerminalEvent(Envelope) && !Meta.bComplete)
		{
			Meta.bComplete = true;
			Meta.Sequence  = NextCompletedTurnSequence++;
			if (Envelope.AgentId != 0)
			{
				CompletedTurnsByAgent.FindOrAdd(Envelope.AgentId).Add(Envelope.TurnKey);
			}
		}
	}
}

void FAgentLogStore::UnlinkNode(FJournalNode* Node)
{
	if (Node == nullptr)
	{
		return;
	}

	const FTryllObservationEnvelope& Envelope = Node->GetValue();
	if (Envelope.EventId != 0)
	{
		NodesByEventId.Remove(Envelope.EventId);
	}

	if (CountsTowardAgentRetention(Envelope))
	{
		if (int32* Count = AgentScopedCounts.Find(Envelope.AgentId))
		{
			if (*Count <= 1)
			{
				AgentScopedCounts.Remove(Envelope.AgentId);
			}
			else
			{
				--(*Count);
			}
		}
	}

	Journal.RemoveNode(Node);
}

void FAgentLogStore::RemoveTurnNodes(const FTryllTurnKey& TurnKey)
{
	FTurnRetentionMeta Meta;
	if (!TurnMetaByKey.RemoveAndCopyValue(TurnKey, Meta))
	{
		return;
	}

	EvictedTurns += 1;
	EvictedObservations += Meta.MemberNodes.Num();
	for (FJournalNode* Node : Meta.MemberNodes)
	{
		UnlinkNode(Node);
	}
}

void FAgentLogStore::EvictIfNeeded()
{
	TArray<uint64> AgentIds;
	CompletedTurnsByAgent.GetKeys(AgentIds);

	for (const uint64 AgentId : AgentIds)
	{
		TArray<FTryllTurnKey>* CompletedTurns = CompletedTurnsByAgent.Find(AgentId);
		if (CompletedTurns == nullptr)
		{
			continue;
		}

		while (CompletedTurns->Num() > Config.MaxCompletedTurnsPerAgent
			|| AgentScopedCounts.FindRef(AgentId) > Config.MaxObservationsPerAgent)
		{
			if (CompletedTurns->Num() == 0)
			{
				break;
			}

			const FTryllTurnKey Oldest = (*CompletedTurns)[0];
			CompletedTurns->RemoveAt(0);
			RemoveTurnNodes(Oldest);

			CompletedTurns = CompletedTurnsByAgent.Find(AgentId);
			if (CompletedTurns == nullptr)
			{
				break;
			}
			if (CompletedTurns->Num() == 0)
			{
				CompletedTurnsByAgent.Remove(AgentId);
				break;
			}
		}
	}

	// Raw nodes are only unlinked here, so pointers stay valid until RemoveAt(0).
	while (RawSessionNodes.Num() > Config.MaxRawSessionObservations)
	{
		FJournalNode* OldestRaw = RawSessionNodes[0];
		RawSessionNodes.RemoveAt(0);
		if (OldestRaw != nullptr)
		{
			EvictedObservations += 1;
			UnlinkNode(OldestRaw);
		}
	}
}

void FAgentLogStore::NotifyChanged()
{
	OnJournalChanged.Broadcast();
}
