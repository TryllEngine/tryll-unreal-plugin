// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "AgentLog/AgentDescriptor.h"

#include "TryllConnection.h"
#include "TryllObservationTypes.h"

FString FAgentDescriptorRegistry::DefaultLabel(const uint64 AgentId)
{
	return FString::Printf(TEXT("Agent %llu"), AgentId);
}

FString FAgentDescriptorRegistry::BuildCreationSummary(const FTryllObservationEnvelope& Envelope)
{
	FString Summary;
	if (!Envelope.GraphDefaultModelName.IsEmpty())
	{
		Summary = FString::Printf(
			TEXT("Model: %s"),
			*Envelope.GraphDefaultModelName);
	}
	else
	{
		Summary = TEXT("Agent created");
	}

	Summary += Envelope.bEnableDiagnostics
		? TEXT("; diagnostics on")
		: TEXT("; diagnostics off");

	if (!Envelope.bMaintainDialogueHistory)
	{
		Summary += TEXT("; stateless");
	}

	return Summary;
}

const FAgentDescriptor* FAgentDescriptorRegistry::Find(const uint64 AgentId) const
{
	return Descriptors.Find(AgentId);
}

TArray<uint64> FAgentDescriptorRegistry::ListAgentIds() const
{
	TArray<uint64> Ids;
	Descriptors.GenerateKeyArray(Ids);
	Ids.Sort();
	return Ids;
}

void FAgentDescriptorRegistry::Clear()
{
	Descriptors.Empty();
	PendingCreates.Empty();
}

FAgentDescriptor& FAgentDescriptorRegistry::FindOrAdd(const uint64 AgentId)
{
	if (FAgentDescriptor* Existing = Descriptors.Find(AgentId))
	{
		return *Existing;
	}

	FAgentDescriptor Created;
	Created.AgentId = AgentId;
	Created.Label   = DefaultLabel(AgentId);
	return Descriptors.Add(AgentId, MoveTemp(Created));
}

void FAgentDescriptorRegistry::SnapshotRegisteredAgents(
	const TArray<FTryllRegisteredAgentSnapshot>& Snapshots)
{
	for (const FTryllRegisteredAgentSnapshot& Snapshot : Snapshots)
	{
		if (Snapshot.AgentId == 0)
		{
			continue;
		}

		FAgentDescriptor& Descriptor = FindOrAdd(Snapshot.AgentId);
		if (!Snapshot.LabelHint.IsEmpty())
		{
			Descriptor.Label = Snapshot.LabelHint;
		}
		else if (Descriptor.Label.StartsWith(TEXT("Pending agent (req ")))
		{
			Descriptor.Label = DefaultLabel(Snapshot.AgentId);
		}

		if (!Snapshot.bCreatedWithDiagnostics)
		{
			Descriptor.bCreatedBeforeDiagnosticsCapture = true;
			Descriptor.bDiagnosticsAvailable            = false;
		}
		else if (!Descriptor.bCreatedBeforeDiagnosticsCapture)
		{
			Descriptor.bDiagnosticsAvailable = true;
		}
	}
}

void FAgentDescriptorRegistry::UpsertPendingCreate(const FTryllObservationEnvelope& Envelope)
{
	const FPendingCreateKey Key{Envelope.ConnectionEpoch, Envelope.RequestId};
	FAgentDescriptor* Existing = PendingCreates.Find(Key);
	FAgentDescriptor Descriptor = Existing ? *Existing : FAgentDescriptor{};

	if (Envelope.AgentId != 0)
	{
		Descriptor.AgentId = Envelope.AgentId;
	}

	Descriptor.CreationSummary     = BuildCreationSummary(Envelope);
	Descriptor.bDiagnosticsAvailable = Envelope.bEnableDiagnostics;
	if (Envelope.bEnableDiagnostics)
	{
		Descriptor.bCreatedBeforeDiagnosticsCapture = false;
	}

	// Model name belongs in CreationSummary only — Label is agent identity
	// (DisplayName / LabelHint / "Agent N"), matching Unity.
	if (Descriptor.Label.IsEmpty())
	{
		Descriptor.Label = Envelope.AgentId != 0
			? DefaultLabel(Envelope.AgentId)
			: FString::Printf(TEXT("Pending agent (req %llu)"), Envelope.RequestId);
	}

	PendingCreates.Add(Key, MoveTemp(Descriptor));
}

void FAgentDescriptorRegistry::UpsertFromCreateOk(const FTryllObservationEnvelope& Envelope)
{
	if (Envelope.AgentId == 0)
	{
		return;
	}

	const FPendingCreateKey Key{Envelope.ConnectionEpoch, Envelope.RequestId};
	FAgentDescriptor Descriptor;
	if (const FAgentDescriptor* Pending = PendingCreates.Find(Key))
	{
		Descriptor = *Pending;
		PendingCreates.Remove(Key);
	}
	else
	{
		Descriptor = FindOrAdd(Envelope.AgentId);
	}

	Descriptor.AgentId = Envelope.AgentId;
	// CreateAgent is observed with AgentId=0 → "Pending agent (req N)". Promote on Ok.
	if (Descriptor.Label.IsEmpty() || Descriptor.Label.StartsWith(TEXT("Pending agent (req ")))
	{
		Descriptor.Label = DefaultLabel(Envelope.AgentId);
	}
	if (Envelope.bEnableDiagnostics || Descriptor.bDiagnosticsAvailable)
	{
		Descriptor.bDiagnosticsAvailable            = true;
		Descriptor.bCreatedBeforeDiagnosticsCapture = false;
	}

	Descriptors.Add(Envelope.AgentId, MoveTemp(Descriptor));
}

void FAgentDescriptorRegistry::UpsertFromCreateErr(const FTryllObservationEnvelope& Envelope)
{
	const FPendingCreateKey Key{Envelope.ConnectionEpoch, Envelope.RequestId};
	PendingCreates.Remove(Key);
}

void FAgentDescriptorRegistry::ObserveEnvelope(const FTryllObservationEnvelope& Envelope)
{
	if (Envelope.IsRequest(::Tryll::MessageBody_CreateAgentRequest))
	{
		UpsertPendingCreate(Envelope);
		if (Envelope.AgentId != 0)
		{
			UpsertFromCreateOk(Envelope);
		}
		return;
	}

	if (Envelope.IsRequest(::Tryll::MessageBody_DestroyAgentRequest))
	{
		if (Envelope.AgentId != 0)
		{
			FindOrAdd(Envelope.AgentId).bDestroyed = true;
		}
		return;
	}

	if (Envelope.IsReplyTo(::Tryll::MessageBody_CreateAgentRequest, true))
	{
		UpsertFromCreateOk(Envelope);
		return;
	}
	if (Envelope.IsReplyTo(::Tryll::MessageBody_CreateAgentRequest, false))
	{
		UpsertFromCreateErr(Envelope);
		return;
	}

	if (Envelope.AgentId == 0)
	{
		return;
	}

	if (Envelope.IsReplyTo(::Tryll::MessageBody_DestroyAgentRequest, true)
		|| Envelope.IsReplyTo(::Tryll::MessageBody_DestroyAgentRequest, false))
	{
		FindOrAdd(Envelope.AgentId).bDestroyed = true;
		return;
	}

	// Any other outbound command with an agent id registers the descriptor.
	// A SendMessage turn must still name the agent after the journal evicts it.
	if (Envelope.Direction == ETryllObservationDirection::Outbound)
	{
		FindOrAdd(Envelope.AgentId);
	}
}
