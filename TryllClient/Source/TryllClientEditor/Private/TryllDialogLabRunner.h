// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "TryllDialogLabRunData.h"
#include "TryllError.h"
#include "TryllGraphDescription.h"

class FTryllAgent;
class UTryllAgentComponent;
class UTryllEditorSubsystem;

/**
 * Executes one immutable FTryllDialogLabRunSnapshot: for each dialogue, for each variant a fresh
 * agent, and for each repeat within it the same agent reseeded and rewound. Serial by design.
 * Never wires TTS callbacks.
 *
 * It reaches a UTryllAgentComponent only when a variant has bUseLiveTools and a step authors no
 * canned result for a tool: the scene component may still have a live handler, and those
 * components come in through Start rather than through the snapshot, because the snapshot is
 * immutable serialisable provenance and a scene object is neither.
 *
 * Unity's runner is written with async/await; here every suspension point is a completion
 * callback, so the same sequence is a continuation chain. Each callback is guarded by a
 * generation counter, so Stop() makes every in-flight continuation inert rather than racing it.
 */
class TRYLLCLIENTEDITOR_API FTryllDialogLabRunner
	: public TSharedFromThis<FTryllDialogLabRunner>
{
public:
	/** (completedTurnUnits, totalTurnUnits). A unit is one generated step. */
	DECLARE_DELEGATE_TwoParams(FOnProgress, int32, int32);
	DECLARE_DELEGATE_OneParam(FOnFinished, const FTryllDialogLabRunResult&);
	/**
	 * (run so far, completedDialogues): every variant and seed has run the first
	 * completedDialogues dialogues, which are final and may be shown; later ones are not.
	 * Not raised for the last dialogue — OnFinished covers it.
	 */
	DECLARE_DELEGATE_TwoParams(FOnDialogueCompleted, const FTryllDialogLabRunResult&, int32);

	~FTryllDialogLabRunner();

	bool IsRunning() const { return bRunning; }

	/**
	 * Seconds a turn may take before the run gives up on it; 0 = wait forever (the interactive
	 * Lab). A timed-out turn is an Error cell and ends the run as Failed: a hung server or model
	 * would hang every later turn too. Set before Start.
	 */
	double TurnTimeoutSeconds = 0.0;

	/** True when the last run ended because a turn ran past TurnTimeoutSeconds. */
	bool TimedOut() const { return bTimedOut; }

	/** "Turn timed out after 120 s" — the same wording as Unity. */
	static FString TimeoutMessage(double Seconds);

	/**
	 * InLiveToolAgents is indexed by variant and may be empty, short, or hold null entries; a
	 * variant without one simply has no live handlers and behaves exactly as before.
	 */
	void Start(
		UTryllEditorSubsystem* InSubsystem,
		const FTryllDialogLabRunSnapshot& InSnapshot,
		FOnProgress InOnProgress,
		FOnFinished InOnFinished,
		const TArray<TWeakObjectPtr<UTryllAgentComponent>>& InLiveToolAgents = {},
		FOnDialogueCompleted InOnDialogueCompleted = FOnDialogueCompleted());

	/**
	 * Cancels the in-flight turn and settles it locally. A disposed agent or a dead connection
	 * cannot produce TurnComplete, so the run must never be left waiting on one.
	 */
	void Stop();

private:
	// ── Sequence ──────────────────────────────────────────────────────────────
	void BeginVariant();
	void CreateAgentForVariant(bool bIsSeedRecreate);
	void HandleAgentCreated(TSharedPtr<FTryllAgent> Agent, FTryllError Error, bool bIsSeedRecreate);
	void BeginRepeat();
	void ChangeSeedForNode(int32 NodeCursor);
	void HandleSeedChanged(const FTryllError& Error, int32 NodeCursor);
	void AppendSeedHistory();
	void EvaluateSeededCells();
	void RunNextGeneratedStep();
	void SettleTurn(ETryllTurnStatus Status, const FString& DebugInfo, int32 Tokens,
		const FString& FailureMessage = FString());
	void FinishTurn(ETryllTurnStatus Status, const FString& DebugInfo, int32 Tokens);
	void FinishRepeat();
	void RewindThenAdvance();
	void AdvanceRepeat();
	void AdvanceVariant();
	void AdvanceDialogue();
	void Finish();

	// ── Failure paths ─────────────────────────────────────────────────────────
	void FailCurrentVariant(const FString& Reason, bool bMarkRunFailed);
	void AbortRepeatsWithSeedFailure(const FString& Reason);

	// ── Helpers ───────────────────────────────────────────────────────────────
	void RegisterCannedToolHandlers();

	/** The scene component serving live tool handlers for the current variant, or null. */
	UTryllAgentComponent* CurrentLiveToolAgent() const;

	/**
	 * Tell the current variant's scene component that the dialogue restarted, so a handler that
	 * keeps per-conversation state starts this repeat clean. One agent serves every repeat of a
	 * variant, and that agent is not the component's, so no delegate on it fires for us.
	 *
	 * Dispatched by name — any parameterless UFUNCTION named ResetToolState on the owning actor's
	 * components receives it. No-op unless the current variant opted into live tools. Optional
	 * by construction: a stateless handler declares none.
	 */
	void ResetLiveToolState();
	void HookAgentCallbacks();
	void DestroyCurrentAgent();
	void ReportProgress();
	FTryllLabSampleResult MakeSample(int32 VariantIndex, int32 Repeat, int32 Seed) const;
	void MakeRectangularTurns(FTryllLabSampleResult& Sample, const FString& SkipReason) const;
	void MarkAllTurns(FTryllLabSampleResult& Sample, ETryllLabCellStatus Status,
		const FString& Error) const;
	const FTryllLabVariantSnapshot& CurrentVariant() const;
	/** The dialogue being run. Every per-script read in the chain goes through this. */
	const FTryllLabScriptSnapshot& CurrentScript() const;
	/** "Dialogue 2 'Haggle': Variant 'B'" — the dialogue part only in multi-dialogue runs. */
	FString Who() const;
	/** Writes a finished (or skipped) repeat into the current dialogue's slice of its column. */
	void StoreSample(const FTryllLabSampleResult& Sample);
	const FTryllLabStepSnapshot&   CurrentStep() const;
	bool IsStale(uint64 Gen) const { return Gen != Generation || !bRunning; }

	// ── State ─────────────────────────────────────────────────────────────────
	TWeakObjectPtr<UTryllEditorSubsystem> Subsystem;
	FTryllDialogLabRunSnapshot Snapshot;
	FTryllDialogLabRunResult   Result;
	FOnProgress OnProgress;
	FOnFinished OnFinished;
	FOnDialogueCompleted OnDialogueCompleted;

	bool   bRunning       = false;
	bool   bStopRequested = false;
	bool   bTimedOut      = false;
	FTSTicker::FDelegateHandle TurnWatchdog;
	uint64 Generation     = 0;

	int32 DialogueIndex = 0;
	int32 VariantIndex = 0;
	int32 RepeatIndex  = 0;
	int32 TurnIndex    = 0;
	int32 DoneUnits    = 0;
	int32 TotalUnits   = 0;

	TSharedPtr<FTryllAgent> Agent;
	/** Per-variant scene components consulted for live tool handlers. Weak: a run outlives no actor. */
	TArray<TWeakObjectPtr<UTryllAgentComponent>> LiveToolAgents;
	/** Pristine per-variant graph with model + overrides baked, seeds not yet applied. */
	FTryllGraphDescription BaseGraph;
	FTryllGraphDescription WorkingGraph;
	TArray<TStrongObjectPtr<UTryllNodeParamsBase>> GraphKeepAlive;
	TArray<FString> SamplingNodeNames;

	FTryllLabSampleResult CurrentSample;
	bool bColumnFailed = false;

	FString AnswerBuffer;
	FString TurnFailure;
	/** Tool names empty-resumed this turn (Missing source). CheckFail still applies. */
	TArray<FString> MissingResumedTools;
	bool    bTurnSettled = false;
	double  TurnStartSeconds = 0.0;

	FDelegateHandle PausedHandle;
	FDelegateHandle ConnectionHandle;
};
