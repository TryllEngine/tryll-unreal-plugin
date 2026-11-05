// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Misc/Guid.h"
#include "TryllDialogChecks.h"
#include "TryllDialogScriptAsset.generated.h"

/**
 * Custom version for Dialog Script assets, kept apart from the node-params version
 * (FTryllAssetVersion) so the two migrate independently. Append-only.
 */
struct TRYLLCLIENTEDITOR_API FTryllDialogScriptVersion
{
	enum Type
	{
		BeforeCustomVersionWasAdded = 0,
		/** Fixed per-step check fields became the typed Checks list (and GlobalChecks). */
		TypedChecks,

		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	static const FGuid GUID;
};

/**
 * Canned result for a workflow that pauses on a step waiting for AwaitResult tool calls.
 * Keyed by tool name, not call id: the server allocates each call's id at dispatch time, so
 * it cannot be authored ahead of a run.
 */
USTRUCT(BlueprintType)
struct FTryllCannedToolResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll")
	FString ToolName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll")
	FString Result;
};

/**
 * Per-step intent assertion. Empty Node addresses every classifier that ran this turn.
 * bExpectNotFound asserts the classifier rejected the input instead of naming a label.
 * JSON field names: node, intent, expect_not_found.
 */
USTRUCT(BlueprintType)
struct FTryllIntentCheck
{
	GENERATED_BODY()

	/** Classifier node name. Empty = any classifier that ran this turn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Checks")
	FString Node;

	/** Expected intent label. Ignored when bExpectNotFound is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Checks")
	FString Intent;

	/** Assert the classifier rejected the input instead of naming a label. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Checks")
	bool bExpectNotFound = false;
};

/**
 * Per-step ClassifyAttributesLLM assertion. Value is an exact slot string.
 * bScoreRange checks the normalised score inclusively and ignores Value.
 * JSON field names: node, name, value, score_range, score_min, score_max.
 */
USTRUCT(BlueprintType)
struct FTryllAttributeCheck
{
	GENERATED_BODY()

	/** Classifier node name. Empty = any ClassifyAttributesLLM node. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Checks")
	FString Node;

	/** Attribute name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Checks")
	FString Name;

	/** Expected slot value. Ignored when bScoreRange is set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Checks")
	FString Value;

	/** When set, Value is ignored and the slot must be a score in range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Checks")
	bool bScoreRange = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Checks")
	float ScoreMin = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Checks")
	float ScoreMax = 1.f;
};

USTRUCT(BlueprintType)
struct FTryllDialogScriptStep
{
	GENERATED_BODY()

	/** Always required — the player line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll", meta = (MultiLine = true))
	FString User;

	/**
	 * Required when this step is seeded (its index < SeedTurnCount); ignored for generated
	 * steps, where the workflow produces the reply.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll", meta = (MultiLine = true))
	FString Assistant;

	/**
	 * Deterministic checks on this step's answer — no model involved, ever. Add as many as
	 * needed, of any type, including several of the same type.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Tryll|Checks")
	TArray<TObjectPtr<UTryllDialogCheck>> Checks;

	// ── Legacy check fields (before typed checks). Migrated into Checks in
	// UTryllDialogScriptAsset::PostLoad and then cleared; kept, with their names, only so
	// older packages still load into them. Not editable. ─────────────────────────────────
	UPROPERTY()
	TArray<FString> MustContain;
	UPROPERTY()
	TArray<FString> MustNotContain;
	UPROPERTY()
	TArray<FString> RegexForbid;
	UPROPERTY()
	int32 MaxWords = 0;
	UPROPERTY()
	TArray<FTryllIntentCheck> IntentChecks;
	UPROPERTY()
	TArray<FTryllAttributeCheck> AttributeChecks;

	/**
	 * A paused step whose tool has no entry here resumes with an empty result and
	 * CheckFails the cell — unless the variant has Use Live Tools and the scene agent
	 * has a live handler. A result here is fixture data only: to assert the tool was
	 * called, add a Calls tool check.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll")
	TArray<FTryllCannedToolResult> ToolResults;
};

/**
 * A durable, version-controlled dialog test case for the Dialog Lab. The leading
 * SeedTurnCount steps are injected via AppendInteractions (no graph run); the rest are sent
 * through SendMessage and their answers checked.
 *
 * The durable half of the Lab's two-tier authoring model: variants (parameter sweeps) are
 * usually throwaway and live in the Lab window, while a script is the reusable test case that
 * variants run against.
 */
UCLASS(BlueprintType)
class TRYLLCLIENTEDITOR_API UTryllDialogScriptAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll", meta = (MultiLine = true))
	FString Description;

	/**
	 * Number of leading Steps that are seeded (both sides supplied, injected with no graph
	 * run) rather than generated (user side only, sent through the workflow).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll", meta = (ClampMin = "0"))
	int32 SeedTurnCount = 0;

	/**
	 * Checks applied to every *generated* turn, on top of each step's own Checks. Use them for
	 * constraints that hold for the whole dialog (a persona break, a leaked instruction marker,
	 * a length cap) instead of repeating the same check on every step. Seeded turns are
	 * author-written, so global checks skip them.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Tryll|Checks")
	TArray<TObjectPtr<UTryllDialogCheck>> GlobalChecks;

	/** Legacy (pre-typed-check) global patterns; migrated into GlobalChecks on load. */
	UPROPERTY()
	TArray<FString> GlobalRegexForbid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll")
	TArray<FTryllDialogScriptStep> Steps;

	int32 GeneratedStepCount() const
	{
		return FMath::Max(0, Steps.Num() - SeedTurnCount);
	}

	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;

	/**
	 * Moves the legacy check fields into Checks / GlobalChecks and clears them. Returns true
	 * when anything moved. Behaviour-preserving: a canned tool result on a generated step used
	 * to assert "must call that tool", so it becomes a Calls tool check. Called by PostLoad for
	 * packages saved before FTryllDialogScriptVersion::TypedChecks; public for tests.
	 */
	bool MigrateLegacyChecks();
};
