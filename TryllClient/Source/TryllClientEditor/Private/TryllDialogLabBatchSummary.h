// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

struct FTryllDialogLabBatchConfig;
struct FTryllDialogLabRunResult;
struct FTryllDialogLabStatistics;

/**
 * A headless run's verdict: the exit code, the summary JSON written next to the JSONL, and the
 * lines printed to the log. Pure, so specs pin it without a server, and Unity's
 * TryllDialogLabBatchSummary writes the same bytes.
 */
struct TRYLLCLIENTEDITOR_API FTryllDialogLabBatchSummary
{
	static constexpr int32 ExitPassed          = 0;
	static constexpr int32 ExitThresholdMissed = 1;
	static constexpr int32 ExitRunFailed       = 2;

	int32 ExitCode = ExitRunFailed;
	FString Json;
	TArray<FString> Lines;

	/**
	 * Run is the finished run, or null when it never started. Failure is why the run could not
	 * start or finish; empty when it ran.
	 */
	static FTryllDialogLabBatchSummary Build(const FTryllDialogLabRunResult* Run,
		const FTryllDialogLabBatchConfig& Config, const FString& Failure = FString());

	/** Empty when every generated turn ran and none errored; else why the results are incomplete. */
	static FString InfrastructureFailure(const FTryllDialogLabStatistics& Stats);
};
