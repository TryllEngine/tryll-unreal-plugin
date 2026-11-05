// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/StrongObjectPtr.h"

#include "TryllDialogLabChecks.h"
#include "TryllDialogLabScriptData.h"
#include "TryllDialogLabExperiment.h"
#include "TryllDialogScriptAsset.h"
#include "TryllGraphDescription.h"
#include "TryllVariableDecl.h"
#include "TryllVoiceProfile.h"
// TStrongObjectPtr below needs the complete params type, not a forward declaration.
#include "Generated/Nodes/TryllNodeParamsBase.h"

class UTryllAgentComponent;
class UTryllRuntimeSettings;
class UTryllWorkflowAsset;

/**
 * A headless run's agent: what FTryllDialogLabRunSnapshot::Capture reads off a scene component,
 * given directly. Variable overrides follow the component's rules (MergeVariableOverrides).
 */
struct FTryllLabAgentSpec
{
	const UTryllWorkflowAsset* Workflow = nullptr;
	FString Name;
	TArray<FTryllVariableOverride> VariableOverrides;
	bool    bMaintainDialogueHistory = true;
	/** Model override; empty = the workflow default. */
	FString Model;
};

enum class ETryllDialogLabRunStatus : uint8
{
	Completed,
	Stopped,
	Failed,
};

struct FTryllLabNodeSummary
{
	FString Name;
	FString Type;
};

/** Execution + provenance for one variant, captured before connect. */
struct FTryllLabVariantSnapshot
{
	int32        Index = 0;
	FString      Label;
	FLinearColor Swatch = FLinearColor::White;
	FString      AgentName;
	bool         bAgentIsOverride = false;
	FString      WorkflowAssetName;
	FString      GraphStartNode;

	/** The EFFECTIVE default model: the variant's override if set, else the graph's. */
	FString          DefaultModelName;
	TArray<FString>  ModelNames;
	TArray<FTryllLabNodeSummary>    Nodes;
	TArray<FTryllLabParamOverride>  Overrides;

	/**
	 * Mirrors the agent component's setting. Provenance *and* execution input: hardcoding
	 * true here would silently run a stateless agent — one whose dialogue the server clears
	 * after every turn — as a stateful one, so the Lab would not be testing the shipped
	 * configuration.
	 */
	bool bMaintainDialogueHistory = true;

	/**
	 * Empty when the variant runs the workflow's own DefaultModelName. DefaultModelName above
	 * already holds the effective value; this records whether it was overridden, which is what
	 * a reader of the export needs to interpret a model comparison.
	 */
	FString ModelOverride;

	/**
	 * When false (default), an uncanned tool call fails the cell. When true, the runner
	 * may invoke the variant's scene-agent handler. Provenance *and* execution input —
	 * the scene object itself stays out of this snapshot.
	 */
	bool bUseLiveTools = false;

	// Execution-only — never serialised to JSONL.
	FTryllGraphDescription    Graph;
	TArray<FTryllVariableDecl> Variables;
	/** The agent component's voice profile, so a speaking workflow can be created (protocol v27). */
	TOptional<FTryllVoiceProfileData> Voice;

	/**
	 * The cloned params objects. FTryllGraphDescription's TObjectPtr members are only
	 * GC-tracked while something reflected owns them, and this snapshot is a plain C++ struct
	 * — without these the clones can be collected mid-run.
	 */
	TArray<TStrongObjectPtr<UTryllNodeParamsBase>> ParamsKeepAlive;
};

/** Immutable run input captured at Run, before connection/model waits. */
struct TRYLLCLIENTEDITOR_API FTryllDialogLabRunSnapshot
{
	FString   RunId;
	FDateTime CapturedAtUtc;
	/**
	 * The dialogues this run covers, in order — one for a Dialog Script, several for a Dialog
	 * Suite. Every sample (one variant x one seed column) runs all of them, and its Turns hold
	 * their steps back to back.
	 */
	TArray<FTryllLabScriptSnapshot> Dialogues;
	int32     Repeats  = 1;
	int32     BaseSeed = 1;
	FString   DefaultAgentName;
	FString   SettingsAssetName;
	TArray<FTryllLabVariantSnapshot> Variants;

	int32 DeriveSeed(int32 VariantIndex, int32 Repeat) const
	{
		return BaseSeed + VariantIndex * 1000 + Repeat;
	}

	int32 PlannedCells() const { return Variants.Num() * FMath::Max(1, Repeats); }
	int32 GeneratedStepsPerSample() const
	{
		int32 Total = 0;
		for (const FTryllLabScriptSnapshot& Dialogue : Dialogues)
		{
			Total += Dialogue.GeneratedStepCount();
		}
		return Total;
	}
	int32 PlannedGeneratedTurns() const { return PlannedCells() * GeneratedStepsPerSample(); }
	int32 TotalSteps() const { return TurnOffset(Dialogues.Num()); }

	/** Index in a sample's flat Turns of dialogue Dialogue's first step. */
	int32 TurnOffset(int32 Dialogue) const
	{
		int32 Offset = 0;
		for (int32 Index = 0; Index < Dialogue && Index < Dialogues.Num(); ++Index)
		{
			Offset += Dialogues[Index].Steps.Num();
		}
		return Offset;
	}

	/**
	 * Headless capture: one variant built from a workflow asset instead of a scene component,
	 * through the same per-variant path as Capture. The voice is the workflow default.
	 */
	static FTryllDialogLabRunSnapshot CaptureFromSpec(
		const UTryllRuntimeSettings* Settings,
		const FTryllLabAgentSpec& Agent,
		const TArray<const UTryllDialogScriptAsset*>& ScriptAssets,
		int32 Seeds,
		int32 BaseSeed);

	static FTryllDialogLabRunSnapshot Capture(
		const UTryllRuntimeSettings*    Settings,
		const UTryllAgentComponent*     DefaultAgent,
		const TArray<const UTryllDialogScriptAsset*>& ScriptAssets,
		const FTryllDialogLabExperiment& Experiment,
		TFunctionRef<UTryllAgentComponent*(const FTryllDialogLabVariant&)> EffectiveAgent);

	/** Deep-clones a graph (params included) for CreateAgent baking. */
	static FTryllLabVariantSnapshot BuildVariant(
		int32 Index, const FTryllDialogLabVariant& Authored, const FString& AgentName,
		const UTryllWorkflowAsset* Workflow, const TArray<FTryllVariableDecl>& Variables,
		const TOptional<FTryllVoiceProfileData>& Voice, bool bMaintainDialogueHistory);

	static FTryllGraphDescription CloneGraph(
		const FTryllGraphDescription& Graph,
		TArray<TStrongObjectPtr<UTryllNodeParamsBase>>& OutKeepAlive);

	/**
	 * The node's own pinned model, or empty when it inherits the agent default. Found by
	 * reflection over a `ModelName` property rather than by enumerating node types, so a new
	 * model-using node needs no change here or in the window's pre-run gate.
	 */
	static FString PinnedModelOf(const UTryllNodeParamsBase* Params);

	/**
	 * Whether this node uses a model at all — i.e. declares a `ModelName` property.
	 *
	 * Distinct from PinnedModelOf returning empty, which is also what a Branch or Transform node
	 * yields because it has no such property. Conflating the two makes the pre-run gate demand a
	 * model for graphs that do not need one.
	 */
	static bool DeclaresModel(const UTryllNodeParamsBase* Params);
};

struct FTryllLabTurnRecord
{
	/** Which of the snapshot's dialogues this turn belongs to. */
	int32   DialogueIndex = 0;
	/** Step index within that dialogue. */
	int32   TurnIndex = 0;
	bool    bSeeded   = false;
	FString UserText;
	FString AnswerText;
	int32   Words = 0;
	ETryllLabCellStatus Status = ETryllLabCellStatus::Skipped;
	/**
	 * Every evaluated check on this turn, passes included — statistics need the denominator,
	 * not only the failures. Empty for turns that never ran.
	 */
	TArray<FTryllLabCheckOutcome> CheckOutcomes;

	TArray<FTryllLabCheckOutcome> FailedChecks() const
	{
		return CheckOutcomes.FilterByPredicate([](const FTryllLabCheckOutcome& O) { return !O.bPassed; });
	}
	FString ErrorMessage;
	int32   DurationMs      = 0;
	int32   TtftMs          = 0;
	int32   TokensGenerated = 0;
	FString DebugInfoRaw;
	TSharedPtr<FJsonObject> DebugInfo;
};

struct FTryllLabSampleResult
{
	FString      SampleId;
	int32        VariantIndex = 0;
	FString      VariantLabel;
	FLinearColor VariantSwatch = FLinearColor::White;
	int32        Repeat = 0;
	int32        Seed   = 0;
	TArray<FTryllLabTurnRecord> Turns;
};

/** Server identity as recorded in an export header. */
struct FTryllLabServerIdentity
{
	FString ExePath;
	FString BuildConfig;
	FString Version;
	FString RunId;
	bool    bManaged = false;
};

/** Immutable completed (or imported) run — the sole source for dataset and export. */
struct FTryllDialogLabRunResult
{
	FTryllDialogLabRunSnapshot Snapshot;
	FTryllLabServerIdentity    Server;
	FDateTime StartedAtUtc;
	FDateTime CompletedAtUtc;
	ETryllDialogLabRunStatus Status = ETryllDialogLabRunStatus::Completed;
	TArray<FString> Issues;
	TArray<FTryllLabSampleResult> Samples;
};
