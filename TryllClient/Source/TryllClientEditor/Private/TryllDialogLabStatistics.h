// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

struct FTryllDialogLabRunResult;

/**
 * A pass rate: Passed of Total, pooled over every seed, plus the per-seed rates its interval is
 * computed from. Seeds, not turns, are the independent unit: turns from one seed share a
 * conversation and fail together, so an interval over all turns would come out several times
 * too narrow.
 */
struct TRYLLCLIENTEDITOR_API FTryllLabRate
{
	int32 Passed = 0;
	int32 Total  = 0;
	TArray<double> PerSeed;

	double Rate() const { return Total > 0 ? static_cast<double>(Passed) / Total : 0.0; }
	/** Standard error of the mean of the per-seed rates; 0 with fewer than 2 seeds. */
	double StandardError() const;
	/** Half-width of the 95% t-interval (df = seeds - 1). */
	double HalfWidth() const;
	double Low() const  { return FMath::Max(0.0, Rate() - HalfWidth()); }
	double High() const { return FMath::Min(1.0, Rate() + HalfWidth()); }
};

/** One variant's headline numbers. */
struct FTryllLabVariantStats
{
	int32        VariantIndex = 0;
	FString      Label;
	FLinearColor Swatch = FLinearColor::White;
	int32        Seeds = 0;

	/** Generated turns whose checks all passed. Errors and not-run turns count as not passed.
	 *  Seeded turns are excluded. */
	FTryllLabRate Turns;
	/** Authored checks on generated turns. The built-in "no unexpected tool call" gate fails
	 *  turns but is not an authored check, so it is not counted here. */
	FTryllLabRate Checks;
	/** Dialogue x seed runs in which every generated turn passed. */
	FTryllLabRate CleanDialogues;

	int32 Errors = 0;
	int32 NotRun = 0;
	TOptional<double> MedianWords;
	TOptional<double> P90Words;
	TOptional<double> MedianTtftMs;
	TOptional<double> P90TtftMs;
};

/** One generated turn's failures per variant: failed seeds of seeds. */
struct FTryllLabTurnFailRow
{
	int32 DialogueIndex = 0;
	int32 StepIndex = 0;
	/** Index into the sample's flat turn list (and the dataset's Turns). */
	int32 FlatIndex = 0;
	FString User;
	TArray<int32> Failed;
	TArray<int32> Seeds;
};

/** One check type's failures per variant: failed of evaluated. */
struct FTryllLabCheckTypeRow
{
	FString CheckType;
	TArray<int32> Failed;
	TArray<int32> Evaluated;
};

/**
 * Per-variant statistics for the Lab's Statistics view and the export header. Pure: a function
 * of a run result and a dialogue filter, so the view, the export and (later) headless mode
 * report the same numbers. A port of Unity's TryllDialogLabStatistics.
 */
struct TRYLLCLIENTEDITOR_API FTryllDialogLabStatistics
{
	/** Below this many seeds per variant an interval is too wide to mean anything. */
	static constexpr int32 MinSeedsForInterval = 5;

	TArray<FTryllLabVariantStats> Variants;
	TArray<FTryllLabTurnFailRow>  WhereTurnsFail;
	TArray<FTryllLabCheckTypeRow> ByCheckType;
	/** The filter this was computed for; unset = every dialogue. */
	TOptional<int32> Dialogue;

	/** Smallest seed count over the variants: what gates the intervals. */
	int32 MinSeeds() const;
	bool IntervalsAvailable() const { return MinSeeds() >= MinSeedsForInterval; }

	static FTryllDialogLabStatistics Compute(
		const FTryllDialogLabRunResult& Run, TOptional<int32> InDialogue = TOptional<int32>());

	/**
	 * Variant VariantIndex minus variant 0, in rate units, with the half-width of a 95% interval
	 * on the difference: Welch's standard error of the two per-seed means, at the smaller of the
	 * two variants' degrees of freedom (conservative).
	 */
	TPair<double, double> DifferenceFromBaseline(
		TFunctionRef<const FTryllLabRate&(const FTryllLabVariantStats&)> Metric, int32 VariantIndex) const;

	/** Two-sided 95% Student-t critical value; 0 for df < 1 (no interval). */
	static double TCritical95(int32 Df);

	/** The export header's "stats" object, byte-identical to Unity's. */
	FString ToJson() const;

	/** Unity's "0.####": at most four decimals, trailing zeros and point dropped. */
	static FString FormatNumber(double Value);
};
