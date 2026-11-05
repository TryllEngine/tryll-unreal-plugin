// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

class FJsonObject;
class FJsonValue;

enum class FTryllIntentKind : uint8
{
	Embedding,
	Llm,
};

struct TRYLLCLIENTEDITOR_API FTryllIntentCandidate
{
	int32 Rank = 0;
	FString Intent;
	TOptional<double> Distance;
};

struct TRYLLCLIENTEDITOR_API FTryllIntentLabelProb
{
	FString Label;
	double Prob = 0.0;
	int32 SourceIndex = 0;
};

/**
 * One classifier execution, normalised from the two node shapes so the inspector and the
 * intent checks share one parse.
 */
struct TRYLLCLIENTEDITOR_API FTryllIntentExecution
{
	int32 NodeIndex = 0;
	FString NodeName;
	FTryllIntentKind Kind = FTryllIntentKind::Embedding;
	FString ExitRoute;
	bool bAccepted = false;
	FString Intent;
	FString RejectedIntent;
	FString NotFoundReason;
	FString Query;

	TOptional<double> Distance;
	TOptional<double> RunnerUpDistance;
	TOptional<double> Margin;
	TOptional<double> Threshold;
	FString RunnerUpIntent;
	FString RecordId;
	FString RecordText;
	TOptional<int32> RecordIndex;
	TArray<FTryllIntentCandidate> Topk;

	TOptional<double> TopProb;
	TOptional<double> SecondProb;
	TOptional<double> MarginResult;
	TOptional<double> MarginThreshold;
	FString PredictedLetter;
	FString ModelName;
	TOptional<int32> HistoryTurns;
	TArray<FTryllIntentLabelProb> Probs;
	bool bProbsLengthMismatch = false;
	int32 ProbsCount = 0;
	int32 IntentsIdsCount = 0;
	TArray<FString> IntentsIds;

	FString StorageName;
	FString MetadataField;
};

/**
 * Pure parsing and presentation for ClassifyIntent* diagnostics. Slate-free so editor tests
 * and the checker share one definition of which label an execution picked.
 */
struct TRYLLCLIENTEDITOR_API FTryllDialogLabIntent
{
	static const TCHAR* TypeEmbed;
	static const TCHAR* TypeLlm;

	static bool IsClassifierType(const FString& Type);

	static FString FormatDistance(TOptional<double> Value);
	static FString FormatProb(TOptional<double> Value);

	/**
	 * The label this execution picked. Accepted route uses Intent; rejected uses
	 * RejectedIntent (A1 + fallbacks). A non-empty label on a non-found route is never
	 * treated as accepted.
	 */
	static FString ResolveLabel(const FTryllIntentExecution& Execution);

	static FString FormatHeader(const TArray<FTryllIntentExecution>& Executions);
	static FString FormatEmptyMessage(const FTryllIntentExecution& Execution);
	static FString FormatMismatchWarning(const FTryllIntentExecution& Execution);
	static FString FormatEmbedSubHeader(const FTryllIntentExecution& Execution);
	static FString FormatLlmSubHeader(const FTryllIntentExecution& Execution);

	static TArray<FTryllIntentExecution> ParseExecutions(
		const TSharedPtr<FJsonObject>& DebugInfo,
		const TArray<TPair<int32, TSharedPtr<FJsonObject>>>& ScopedNodes);

	static FTryllIntentExecution ParseOne(int32 NodeIndex, const TSharedPtr<FJsonObject>& Node);

	static TArray<FString> AttachedIntents(const TSharedPtr<FJsonObject>& DebugInfo);
};
