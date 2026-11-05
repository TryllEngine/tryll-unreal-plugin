// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "UObject/StrongObjectPtr.h"

#include "TryllEditorStatus.h"
#include "TryllEditorUiContract.h"
#include "TryllDialogLabDataset.h"
#include "TryllDialogLabExperiment.h"
#include "UI/TryllPanelSnapshot.h"
#include "TryllDialogLabRunData.h"

class FTryllDialogLabRunner;
class FTryllModelDownloadGuard;
class SProgressBar;
class STryllTurnInspectorView;
class SVerticalBox;
class UTryllAgentComponent;
class UTryllDialogScriptAsset;
class UTryllEditorSubsystem;
class UTryllRuntimeSettings;

/**
 * Tryll Dialog Lab — runs one dialog script across N samples and shows the spread, so an author
 * can see *variance* rather than one lucky answer.
 *
 * Deliberately a separate panel from STryllChatPanel: that one is single-agent with a text/voice
 * input as its centre of gravity; this one is N agents × N histories with no primary input at
 * all.
 */
class STryllDialogLabPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STryllDialogLabPanel)
		: _EditorSubsystemOverride(nullptr)
		, _PersistLayout(true)
		, _AgentComponentOverride(nullptr)
	{}
		/** Test-only: run through this subsystem instead of the live GEditor one. */
		SLATE_ARGUMENT(UTryllEditorSubsystem*, EditorSubsystemOverride)
		/** False: neither read nor write UTryllDialogLabConfig. */
		SLATE_ARGUMENT(bool, PersistLayout)
		/** Test-only: the default agent, instead of the persisted / scene one. */
		SLATE_ARGUMENT(UTryllAgentComponent*, AgentComponentOverride)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~STryllDialogLabPanel() override;

	/** Assign script, variants and repeats as the stage controls would. */
	/** InSource is a Dialog Script or a Dialog Suite. */
	void DebugBindForTests(UObject* InSource,
		const TArray<FTryllDialogLabVariant>& Variants, int32 Repeats);
	/** Press Run as a click would. */
	void DebugRunForTests() { OnRunClicked(); }
	bool DebugRunningForTests() const { return bRunning; }
	/**
	 * Rows are "variant#repeat:turn:status:answer" per completed turn; LocalState is
	 * Running / Completed / Failed / Idle; the banner is the run failure.
	 */
	FTryllPanelSnapshot DebugSnapshotForTests() const;

	/**
	 * Compare-in-Lab: pre-seed the chat's agent and its history as a transient script's seeded
	 * steps. The script is an unsaved instance — nothing reaches disk unless the user saves it.
	 */
	void OpenFromChat(
		UTryllAgentComponent* SourceAgent,
		const TArray<struct FTryllChatHistoryEntry>& History);

	/** Arrow keys move the selection across samples and turns — how you skim N samples. */
	virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	/** What the main stage area shows. Variant authoring is a workspace, not a config field. */
	enum class EStage : uint8 { Dialog, Variants };
	enum class EViewMode : uint8 { Statistics, Grid };

	// ── Layout ────────────────────────────────────────────────────────────────
	TSharedRef<SWidget> BuildConfigBox();
	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildProgressRow();
	TSharedRef<SWidget> BuildSegmented(
		const TArray<FText>& Labels,
		TFunction<int32()> GetActive,
		TFunction<void(int32)> SetActive,
		TFunction<bool()> IsEnabled);

	/** Rebuilds the stage content in the order §10.3 prescribes, then restores both offsets. */
	void RebuildStage();
	void RebuildStageContent();
	/** Deferred one frame: offsets cannot be applied before layout has run (10.5). */
	void ScheduleScrollRestore();

	// ── Results rendering ─────────────────────────────────────────────────────
	void BuildGridStage();
	TSharedRef<SWidget> BuildDialogueHeader(int32 DialogueIndex);
	/** The next turn row in Step's direction the Grid shows; folded dialogues are skipped. */
	int32 NextVisibleTurn(int32 From, int32 Step) const;
	/**
	 * Per-variant pass rates (layout A of dialog-lab-statistics-mockup.html), optionally as
	 * differences from the first variant, then where the failures are. Numbers come from
	 * FTryllDialogLabStatistics, the same function the export header uses.
	 */
	void BuildStatisticsStage();
	TSharedRef<SWidget> BuildStatsMetric(const struct FTryllDialogLabStatistics& Stats,
		const FString& Title, const FString& Hint,
		TFunction<const struct FTryllLabRate&(const struct FTryllLabVariantStats&)> Pick,
		bool bIntervals, bool bDelta);
	TSharedRef<SWidget> BuildStatsTableHeader(const struct FTryllDialogLabStatistics& Stats);
	TSharedRef<SWidget> BuildWhereTurnsFailTable(const struct FTryllDialogLabStatistics& Stats);
	TSharedRef<SWidget> BuildCheckTypeTable(const struct FTryllDialogLabStatistics& Stats);
	/** From a "where turns fail" cell to the Grid, on the first failing seed. */
	void JumpToTurn(int32 FlatIndex, int32 VariantIndex);
	TSharedRef<SWidget> BuildColumnHeader();
	/** The user band lives OUTSIDE the horizontal scroller so it always spans the viewport. */
	TSharedRef<SWidget> BuildBand(int32 TurnIndex);
	TSharedRef<SWidget> BuildCellRow(int32 TurnIndex);
	TSharedRef<SWidget> BuildCell(const FTryllLabCellView& Cell);
	TSharedRef<SWidget> BuildChip(
		const FText& Text, const FLinearColor& Foreground,
		const FLinearColor& Background, const FLinearColor& Border);
	TSharedRef<SWidget> BuildTimingBar(const FTryllLabCellView& Cell, float Height);

	/** One shared offset across the column header and every turn row. */
	TSharedRef<class SScrollBox> MakeHorizontalScroller();
	/** The stage's vertical scroller, whose offset survives a rebuild. */
	TSharedRef<class SScrollBox> MakeVerticalScroller();
	void SyncHorizontal(const TSharedPtr<class SScrollBox>& From, float Offset);

	// ── Variants stage ────────────────────────────────────────────────────────
	void BuildVariantsStage();
	TSharedRef<SWidget> BuildVariantCard(int32 VariantIndex);
	TSharedRef<SWidget> BuildModelRow(int32 VariantIndex);
	TSharedRef<SWidget> BuildOverrideEditor(int32 VariantIndex);
	/** Kind-appropriate value widget: toggle, enum dropdown, int, float, text, multiline. */
	TSharedRef<SWidget> BuildOverrideValueEditor(const struct FTryllOverridableField* Field);
	/** "(workflow default)" first, then the manifest's language models; never duplicated. */
	static TArray<FString> BuildModelChoices(
		const TArray<FString>& LanguageModels, const FString& CurrentOverride);
	static const TCHAR* ModelDefaultChoice;

	/** Re-renders only the inspector; cell chrome is attribute-bound and needs no rebuild. */
	void RefreshInspector();
	void SelectCell(const FString& Key);
	void PinColumn(const FString& SampleId);
	FReply OnCellMouseDown(const FGeometry& Geometry, const FPointerEvent& Event,
		FString Key, FString SampleId);

	// ── Run ───────────────────────────────────────────────────────────────────
	FReply OnRunClicked();
	FReply OnResetClicked();
	FReply OnExportClicked();
	FReply OnImportClicked();
	/**
	 * Empty when the configuration is runnable; otherwise the first failure, verbatim.
	 *
	 * bScriptOnly stops after the settings and script gates, which is how Run checks those before
	 * it mutates the experiment by adding a default variant.
	 */
	FString ValidateBeforeRun(bool bScriptOnly = false) const;
	void BeginSession();
	void HandleConnectionChanged(bool bConnected);
	void HandleCreateSessionComplete(bool bSuccess);
	void HandleModelsReady(bool bReady, FString Error);
	void StartRun();
	void HandleRunProgress(int32 Done, int32 Total);
	void HandleRunFinished(const FTryllDialogLabRunResult& Result);
	void InstallRunResult(const FTryllDialogLabRunResult& Result, bool bImported);
	void TeardownSession();
	void RefreshOwnerChrome();
	void HandleHostColorsRefreshed();
	void HandleOwnerPhaseChanged(ETryllOwnerPhase Phase, const FString& Detail);

	// ── Queries ───────────────────────────────────────────────────────────────
	UTryllAgentComponent* EffectiveAgent(const FTryllDialogLabVariant& Variant) const;
	const FTryllGraphDescription* GraphOf(const UTryllAgentComponent* Agent) const;
	/**
	 * True only when there are results safe to read. bRan is *supposed* to imply the dataset —
	 * they are assigned together in InstallRunResult and cleared together everywhere else — but
	 * an exception out of a UI callback aborts the whole rebuild and leaves the panel half-built.
	 * Checking the data rather than the flag makes every reader robust to that invariant breaking.
	 */
	bool HasResults() const { return bRan && Dataset.IsSet(); }

	/** Surfaces that invariant break once per panel instead of degrading silently (10.2). */
	void WarnIfInconsistent(const TCHAR* Where);
	int32 PlannedCells() const;
	int32 PlannedTurns() const;
	/** The dialogues the next run covers; empty when nothing usable is assigned. */
	TArray<const UTryllDialogScriptAsset*> SourceScripts() const;
	void HandleDialogueCompleted(const FTryllDialogLabRunResult& Result, int32 CompletedDialogues);
	FText PreRunMessage() const;
	FText RunStatusText() const;
	void RefreshAgentCandidates();

	// ── Persistence (§6.2) ────────────────────────────────────────────────────
	void LoadLabConfig();
	void SaveLabConfig() const;

	// ── State ─────────────────────────────────────────────────────────────────
	EStage    Stage    = EStage::Dialog;
	EViewMode ViewMode = EViewMode::Grid;
	/** Statistics: show differences from the first variant instead of absolute rates. */
	bool  bStatsDelta     = false;
	/** Statistics: draw 95% intervals when the run has enough seeds. */
	bool  bStatsIntervals = true;
	/** Statistics: the dialogue filter; -1 = every dialogue. Not persisted. */
	int32 StatsDialogue   = -1;
	int32     Density  = 1;
	float     CellWidth = FTryllEditorUiContract::CellWidthMedium;

	/** Inspector width the user dragged to, and whether the pane is showing. Both persisted. */
	float InspectorWidth    = 400.0f;
	bool  bInspectorVisible = true;

	TWeakObjectPtr<UTryllRuntimeSettings>   Settings;
	TWeakObjectPtr<UTryllAgentComponent>    DefaultAgent;
	/** What the Dialog Script field holds: a Dialog Script or a Dialog Suite. */
	TStrongObjectPtr<UObject> DialogSource;
	TArray<TWeakObjectPtr<UTryllAgentComponent>> AgentCandidates;

	FTryllDialogLabExperiment Experiment;

	UTryllEditorSubsystem* EditorSubsystem = nullptr;
	bool bPersistLayout = true;
	TSharedPtr<FTryllDialogLabRunner> Runner;
	TSharedPtr<FTryllModelDownloadGuard> ModelGuard;
	FTryllDialogLabRunSnapshot PendingSnapshot;
	/**
	 * Per-variant scene components, variant order, captured alongside PendingSnapshot. Kept out
	 * of the snapshot itself: that is immutable serialisable provenance and a scene object is
	 * neither. The runner uses these only to reach live tool handlers.
	 */
	TArray<TWeakObjectPtr<UTryllAgentComponent>> PendingLiveAgents;

	TOptional<FTryllDialogLabDataset> Dataset;
	TOptional<FTryllDialogLabRunResult> CompletedRun;
	/**
	 * The dialogues finished so far while a multi-dialogue run continues. Not a completed run:
	 * CompletedRun stays unset, so Export stays off.
	 */
	TOptional<FTryllDialogLabRunResult> PartialRun;
	/** Folded dialogue groups in the Grid (dialogue indices). */
	TSet<int32> CollapsedDialogues;
	bool bRan            = false;
	bool bRunning        = false;
	bool bResultsImported = false;
	bool bAcquiredConnection = false;
	uint64 RunGeneration = 0;
	/** Latches so the 10.2 diagnostic is emitted once, not on every rebuild. */
	mutable bool bWarnedInconsistentState = false;

	FString RunFailure;
	FString RunNotes;
	FString SessionStatus;
	int32   ProgressDone  = 0;
	int32   ProgressTotal = 0;

	FString SelectedKey;
	FString PinA;
	FString PinB;

	TSharedPtr<STryllTurnInspectorView> InspectorView;
	TSharedPtr<SVerticalBox> StageHost;
	TSharedPtr<SWidget>      ProgressRow;
	TSharedPtr<class STryllStatusBar> StatusBar;
	TSharedPtr<class STryllBanner>    ImportBanner;
	FDelegateHandle OwnerPhaseHandle;
	FDelegateHandle HostColorsHandle;

	/** Combo sources must outlive their widgets, so each rebuild parks them here. */
	TArray<TSharedPtr<TArray<TSharedPtr<FString>>>> ModelChoiceOptions;
	FString PendingOverrideNode;
	FString PendingOverrideField;
	FString PendingOverrideValue;
	/** "{node}/{fieldPath}" the pending value was pre-filled for; a change re-prefills it. */
	FString PendingValuePath;
	FString OverrideAddError;

	TArray<TSharedPtr<class SScrollBox>> HorizontalScrollers;
	float HorizontalOffset = 0.0f;
	bool  bSyncingHorizontal = false;

	TSharedPtr<class SScrollBox> VerticalScroller;
	float VerticalOffset = 0.0f;

	/**
	 * A Statistics → Grid jump: the flat turn JumpToTurn wants at the top. The next BuildGridStage
	 * asks the vertical scroller to bring that turn's band into view and the deferred restore then
	 * leaves the vertical offset alone. One-shot: consumed by that rebuild.
	 */
	int32 JumpTurn = -1;
	bool  bJumpScrollPending = false;
};
