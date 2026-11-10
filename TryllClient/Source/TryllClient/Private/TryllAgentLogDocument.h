// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllObservationTypes.h"

struct FTryllAgentLogExportAgent
{
	uint64 AgentId = 0;
	uint64 Epoch = 0;
	FString Label;
	bool bHasCreatedSeq = false;
	uint64 CreatedSeq = 0;
	bool bHasDestroyedSeq = false;
	uint64 DestroyedSeq = 0;
	bool bDiagnostics = false;
	bool bCreatedBeforeDiagnosticsCapture = false;
	int32 CompletedTurns = 0;
};

struct FTryllAgentLogExportInfo
{
	FString Host = TEXT("unreal");
	FString HostVersion;
	/** Set by the exporter from the plugin descriptor; empty when unknown. */
	FString PluginVersion;
	FString ProjectName;
	FString Os;
	FString ServerVersion;
	FString ServerRunId;
	FString ServerBuildConfig;
	FString ServerMode;
	FString CaptureState = TEXT("idle");
	bool bHistoryDiscarded = false;
	uint64 SelectedAgentId = 0;
	int32 MaxTurns = 200;
	int32 MaxObservations = 4000;
	int32 MaxRaw = 4000;
	int32 EvictedTurns = 0;
	int32 EvictedObservations = 0;
	TArray<FTryllAgentLogExportAgent> Agents;
};

/** Writes one tryll.agent_log document. Coalescing is export-only. */
class TRYLLCLIENT_API FTryllAgentLogDocument
{
public:
	static FString Write(const TArray<FTryllObservationEnvelope>& Events, const FTryllAgentLogExportInfo& Info);
	static FString FileName(const FString& ProjectName, const FDateTime& LocalNow);
};
