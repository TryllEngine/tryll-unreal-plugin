// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogLabBatchSummary.h"

#include "TryllDialogLabBatchConfig.h"
#include "TryllDialogLabExport.h"
#include "TryllDialogLabRunData.h"
#include "TryllDialogLabStatistics.h"

namespace
{
	/** Lower-cased, as Unity's Status.ToString().ToLowerInvariant() and the export header write it. */
	const TCHAR* SummaryStatusName(const ETryllDialogLabRunStatus Status)
	{
		switch (Status)
		{
			case ETryllDialogLabRunStatus::Stopped: return TEXT("stopped");
			case ETryllDialogLabRunStatus::Failed:  return TEXT("failed");
			default:                                return TEXT("completed");
		}
	}

	FString SummaryQuoted(const FString& Value)
	{
		return TEXT("\"") + FTryllDialogLabExport::EscapeJson(Value) + TEXT("\"");
	}

	FString SummaryNum(const double Value)
	{
		return FTryllDialogLabStatistics::FormatNumber(Value);
	}
}

FString FTryllDialogLabBatchSummary::InfrastructureFailure(const FTryllDialogLabStatistics& Stats)
{
	int32 Errors = 0;
	int32 NotRun = 0;
	for (const FTryllLabVariantStats& V : Stats.Variants)
	{
		Errors += V.Errors;
		NotRun += V.NotRun;
	}
	if (Errors == 0 && NotRun == 0)
	{
		return FString();
	}
	return FString::Printf(
		TEXT("%d generated turn(s) ended in an error and %d did not run; the results are incomplete (see the other issues)."),
		Errors, NotRun);
}

FTryllDialogLabBatchSummary FTryllDialogLabBatchSummary::Build(const FTryllDialogLabRunResult* Run,
	const FTryllDialogLabBatchConfig& Config, const FString& InFailure)
{
	FTryllDialogLabBatchSummary Summary;
	TArray<FString> Issues = Run != nullptr ? Run->Issues : TArray<FString>();
	FString Status = Run == nullptr ? FString(TEXT("failed")) : FString(SummaryStatusName(Run->Status));
	FString Failure = InFailure;

	TOptional<FTryllDialogLabStatistics> Stats;
	if (Run != nullptr)
	{
		Stats = FTryllDialogLabStatistics::Compute(*Run);
	}
	bool bRanToEnd = Failure.IsEmpty() && Run != nullptr && Run->Status == ETryllDialogLabRunStatus::Completed;
	if (!bRanToEnd && Failure.IsEmpty())
	{
		Failure = Run == nullptr
			? FString(TEXT("The run did not start."))
			: FString::Printf(TEXT("The run ended as %s."), *Status);
	}
	// A run can complete with part of it broken: a dialogue whose agent could not be created, a
	// failed reseed or rewind, a turn the server errored. Those cells are Error or not run, never
	// a check failure, so they are the rig failing, not the NPC.
	if (bRanToEnd && Failure.IsEmpty())
	{
		Failure = InfrastructureFailure(*Stats);
	}
	// The config refuses this up front; a run that still has fewer seeds per variant (an
	// imported or synthesised one) must not gate on a bound its own stats omit.
	if (bRanToEnd && Failure.IsEmpty() && Config.bUseLowerBound && !Stats->IntervalsAvailable())
	{
		Failure = FTryllDialogLabBatchConfig::LowerBoundSeedsError(Stats->MinSeeds(), TEXT("run"));
	}
	if (!Failure.IsEmpty())
	{
		bRanToEnd = false;
		Issues.Add(Failure);
		if (Status == TEXT("completed"))
		{
			Status = TEXT("failed");
		}
	}
	TArray<FString> Checks;
	int32 Missed = 0;
	if (bRanToEnd && Stats->Variants.Num() > 0)
	{
		const TCHAR* Basis = Config.bUseLowerBound ? TEXT("lower_bound") : TEXT("point");
		const FTryllLabVariantStats& V = Stats->Variants[0];
		auto Gate = [&](const TCHAR* Metric, const TOptional<double>& Required, const FTryllLabRate& Rate)
		{
			if (!Required.IsSet())
			{
				return;
			}
			const double Actual = Config.bUseLowerBound ? Rate.Low() : Rate.Rate();
			const bool bPassed = Actual >= Required.GetValue() - 1e-12;
			if (!bPassed)
			{
				++Missed;
			}
			Checks.Add(FString::Printf(
				TEXT("{\"metric\":\"%s\",\"required\":%s,\"actual\":%s,\"basis\":\"%s\",\"passed\":%s}"),
				Metric, *SummaryNum(Required.GetValue()), *SummaryNum(Actual), Basis,
				bPassed ? TEXT("true") : TEXT("false")));
			Summary.Lines.Add(FString::Printf(TEXT("%s: %s %s %s (%s) %s"),
				Metric, *SummaryNum(Actual), bPassed ? TEXT(">=") : TEXT("<"), *SummaryNum(Required.GetValue()),
				Basis, bPassed ? TEXT("PASS") : TEXT("FAIL")));
		};
		Gate(TEXT("turns_passed"), Config.TurnsPassed, V.Turns);
		Gate(TEXT("checks_passed"), Config.ChecksPassed, V.Checks);
		Gate(TEXT("clean_dialogues"), Config.CleanDialogues, V.CleanDialogues);
	}

	Summary.ExitCode = !bRanToEnd ? ExitRunFailed : Missed > 0 ? ExitThresholdMissed : ExitPassed;
	if (Summary.ExitCode == ExitPassed)
	{
		Summary.Lines.Add(FString::Printf(TEXT("Dialog Lab: PASS (exit %d)"), ExitPassed));
	}
	else if (Summary.ExitCode == ExitThresholdMissed)
	{
		Summary.Lines.Add(FString::Printf(TEXT("Dialog Lab: FAIL, %d threshold%s missed (exit %d)"),
			Missed, Missed == 1 ? TEXT("") : TEXT("s"), ExitThresholdMissed));
	}
	else
	{
		Summary.Lines.Add(FString::Printf(TEXT("Dialog Lab: ERROR, %s (exit %d)"), *Failure, ExitRunFailed));
	}

	TArray<FString> QuotedIssues;
	for (const FString& Issue : Issues)
	{
		QuotedIssues.Add(SummaryQuoted(Issue));
	}
	Summary.Json = FString::Printf(
		TEXT("{\"format\":%d,\"status\":\"%s\",\"exit_code\":%d,\"run_id\":%s,\"stats\":%s,")
		TEXT("\"thresholds\":[%s],\"issues\":[%s]}"),
		FTryllDialogLabBatchConfig::Format, *Status, Summary.ExitCode,
		*SummaryQuoted(Run != nullptr ? Run->Snapshot.RunId : FString()),
		Stats.IsSet() ? *Stats->ToJson() : TEXT("null"),
		*FString::Join(Checks, TEXT(",")), *FString::Join(QuotedIssues, TEXT(",")));
	return Summary;
}
