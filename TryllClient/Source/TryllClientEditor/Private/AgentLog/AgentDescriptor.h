// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllSubsystem.h"

/** Compact agent identity that survives journal eviction. */
struct FAgentDescriptor
{
	uint64 AgentId = 0;
	FString Label;
	FString CreationSummary;
	bool bDestroyed                       = false;
	bool bCreatedBeforeDiagnosticsCapture = false;
	bool bDiagnosticsAvailable            = false;
};

/** Agent list keyed by id; independent of the observation journal. */
class TRYLLCLIENTEDITOR_API FAgentDescriptorRegistry
{
public:
	const TMap<uint64, FAgentDescriptor>& GetAll() const { return Descriptors; }
	const FAgentDescriptor*               Find(uint64 AgentId) const;
	TArray<uint64>                        ListAgentIds() const;

	void Clear();
	void SnapshotRegisteredAgents(const TArray<FTryllRegisteredAgentSnapshot>& Snapshots);
	void ObserveEnvelope(const struct FTryllObservationEnvelope& Envelope);

private:
	struct FPendingCreateKey
	{
		uint64 Epoch = 0;
		uint64 RequestId = 0;

		friend uint32 GetTypeHash(const FPendingCreateKey& Key)
		{
			return HashCombine(GetTypeHash(Key.Epoch), GetTypeHash(Key.RequestId));
		}

		bool operator==(const FPendingCreateKey& Other) const
		{
			return Epoch == Other.Epoch && RequestId == Other.RequestId;
		}
	};

	static FString DefaultLabel(uint64 AgentId);
	static FString BuildCreationSummary(const struct FTryllObservationEnvelope& Envelope);

	FAgentDescriptor& FindOrAdd(uint64 AgentId);
	void UpsertPendingCreate(const struct FTryllObservationEnvelope& Envelope);
	void UpsertFromCreateOk(const struct FTryllObservationEnvelope& Envelope);
	void UpsertFromCreateErr(const struct FTryllObservationEnvelope& Envelope);

	TMap<uint64, FAgentDescriptor> Descriptors;
	TMap<FPendingCreateKey, FAgentDescriptor> PendingCreates;
};
