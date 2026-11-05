// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

class FJsonValue;

/**
 * Pure parsing and presentation for RetrieveNode diagnostics. Slate-free so the Hybrid
 * null-distance ordering stays testable.
 */
struct TRYLLCLIENTEDITOR_API FTryllLabRetrievalHit
{
	FString Id;
	FString Text;
	/** Unset when the server emitted JSON null — a lexical-only hit has no dense leg. */
	TOptional<double> Distance;
	TOptional<double> Bm25Score;
	TOptional<int32>  RankDense;
	TOptional<int32>  RankSparse;

	FString FormatDistance() const;
};

struct TRYLLCLIENTEDITOR_API FTryllDialogLabRetrieval
{
	/** Parses output.results[] preserving server rank order; a null distance stays unset. */
	static TArray<FTryllLabRetrievalHit> ParseHits(const TSharedPtr<FJsonValue>& ResultsArray);

	static FString FormatHeader(
		int32 ResultCount, const FString& RetrievalMode, double Threshold, int32 RrfK);

	static FString FormatEmptyMessage(const FString& RetrievalMode);

	static FString FormatHitMeta(const FTryllLabRetrievalHit& Hit);
};
