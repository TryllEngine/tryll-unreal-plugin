// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWidget.h"
#include "TryllEditorStatus.h"
#include "TryllEditorUiContract.h"
#include "UI/TryllTailFollow.h"
#include "UI/TryllPanelSnapshot.h"
#include "AgentLog/AgentLogConfig.h"
#include "AgentLog/AgentLogProjection.h"
#include "AgentLog/AgentLogTurnCollapse.h"

class FAgentLogStore;
class STryllInspectorSplit;
class STryllAgentLogInspectorHost;
class UTryllEditorSubsystem;

/** One virtualized timeline row (stable EventId for selection across eviction). */
struct FAgentLogTimelineItem
{
	FAgentLogRow Row;
	uint64       EventId = 0;
	FString      Summary;
	bool         bDoubleHeight = false;
};

/** Agent list entry — first item is All Agents (P5 filter). */
struct FAgentLogAgentListItem
{
	bool   bAllAgents = false;
	uint64 AgentId    = 0;
	FString Title;
	FString Status;
};

/**
 * Agent Log editor window (Package 2 Phase 4, Option 3).
 * Agent list │ timeline │ collapsible inspector.
 */
class STryllAgentLogPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STryllAgentLogPanel)
		: _EditorSubsystemOverride(nullptr)
		, _PersistLayout(true)
	{}
		/** Test-only: attach through this subsystem instead of the live GEditor one. */
		SLATE_ARGUMENT(UTryllEditorSubsystem*, EditorSubsystemOverride)
		/** False: neither read nor write the Agent Log layout config. */
		SLATE_ARGUMENT(bool, PersistLayout)
	SLATE_END_ARGS()

	STryllAgentLogPanel();
	void Construct(const FArguments& InArgs);
	virtual ~STryllAgentLogPanel() override;

	/** Rebuild the agent list and chrome now, as the next journal change would. */
	void DebugRefreshForTests();
	/**
	 * Rows are agent-list titles without "All Agents"; LocalState is Capturing or
	 * Waiting (store subscription). Read after DebugRefreshForTests.
	 */
	FTryllPanelSnapshot DebugSnapshotForTests() const;

private:
	static constexpr float AgentListMinWidth   = FTryllEditorUiContract::AgentListMinWidth;
	static constexpr float TimelineMinWidth    = FTryllEditorUiContract::TimelineMinWidth;
	static constexpr float RowHeightSingle     = 22.0f;
	static constexpr float RowHeightDouble     = 34.0f;
	/** Leading columns are fixed so →/← and HH:MM:SS share one vertical rail. */
	static constexpr float ChevronColumnWidth    = 14.0f;
	static constexpr float MarkerColumnWidth     = 14.0f;
	static constexpr float TimestampColumnWidth  = 84.0f; // HH:MM:SS.mmm
	static constexpr float AgentGutterWidth      = 108.0f;
	/** R12.3a hierarchy cue — applied to the summary only, never the gutter. */
	static constexpr float ReceivedIndent        = 12.0f;

	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildBanners();
	TSharedRef<SWidget> BuildMainArea();
	TSharedRef<SWidget> BuildAgentList();
	TSharedRef<SWidget> BuildTimeline();
	TSharedRef<SWidget> BuildInspectorPane();
	float GetInnerColumnMinWidth() const;
	TSharedRef<SWidget> BuildSegmented(
		const TArray<FText>& Labels,
		TFunction<int32()> GetActive,
		TFunction<void(int32)> SetActive);

	TSharedRef<ITableRow> GenerateAgentRow(
		TSharedPtr<FAgentLogAgentListItem> Item,
		const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GenerateTimelineRow(
		TSharedPtr<FAgentLogTimelineItem> Item,
		const TSharedRef<STableViewBase>& OwnerTable);
	void RebuildAgentListItems();
	void RebuildTimelineItems();
	void RestoreTimelineSelection();
	void RefreshInspector();
	float MeasureTimelineDistance() const;
	void ScrollTimelineToEnd();
	EActiveTimerReturnType EndTimelineProgrammaticScroll(double CurrentTime, float DeltaTime);
	void OnStoreChanged();

	/** Re-subscribe to the editor subsystem's current shared connection when needed. */
	void AttachIfNeeded();
	void HandleConnectionChanged(bool bConnected);
	void RefreshStatusChrome();
	void RefreshBanners();
	void HandleHostColorsRefreshed();
	void HandleOwnerPhaseChanged(ETryllOwnerPhase Phase, const FString& Detail);

	FReply OnClearClicked();
	FReply OnExportClicked();
	FReply OnPauseResumeClicked();
	FReply OnInspectorToggleClicked();
	FReply OnTurnChevronClicked(FTryllTurnKey TurnKey, bool bCurrentlyCollapsed);
	void OnAgentSelectionChanged(
		TSharedPtr<FAgentLogAgentListItem> Item,
		ESelectInfo::Type SelectInfo);
	void OnTimelineSelectionChanged(
		TSharedPtr<FAgentLogTimelineItem> Item,
		ESelectInfo::Type SelectInfo);
	void LoadLayoutConfig();
	void SaveLayoutConfig() const;

	FAgentLogFilter CurrentFilter() const;
	const struct FTryllObservationEnvelope* EnvelopeForRow(const FAgentLogRow& Row) const;

	FAgentLogStore* Store = nullptr;
	UTryllEditorSubsystem* EditorSubsystem = nullptr;
	bool bPersistLayout = true;
	FDelegateHandle JournalChangedHandle;
	FDelegateHandle ConnectionChangedHandle;

	TArray<TSharedPtr<FAgentLogAgentListItem>> AgentListItems;
	TArray<TSharedPtr<FAgentLogTimelineItem>> TimelineItems;

	TSharedPtr<class STryllStatusBar> StatusBar;
	TSharedPtr<class STryllBanner> WaitingBanner;
	TSharedPtr<class STryllBanner> PauseBanner;
	TSharedPtr<class STryllBanner> DiscardedBanner;
	TSharedPtr<class STryllBanner> PreExistingBanner;
	FDelegateHandle OwnerPhaseHandle;
	FDelegateHandle HostColorsHandle;

	TSharedPtr<STryllInspectorSplit> OuterSplitter;
	TSharedPtr<STryllInspectorSplit> InnerSplitter;
	TSharedPtr<class SListView<TSharedPtr<FAgentLogAgentListItem>>> AgentListView;
	TSharedPtr<class SListView<TSharedPtr<FAgentLogTimelineItem>>> TimelineListView;
	TSharedPtr<STryllAgentLogInspectorHost> InspectorHost;
	FTryllTailFollow TimelineTail;

	TSharedPtr<FAgentLogAgentListItem> SelectedAgentItem;
	TSharedPtr<FAgentLogTimelineItem>  SelectedTimelineItem;
	uint64 PendingSelectionEventId = 0;

	bool bAllAgentsSelected = true;
	uint64 SelectedAgentId  = 0;
	bool bRawMode              = false;
	bool bCoalesceAnswerText   = true;
	bool bCollapseCompletedTurns = true;
	bool bUserPaused           = false;

	FAgentLogTurnCollapseState TurnCollapse;
	/** Full projected rows before collapse (for promoting selection out of hidden bodies). */
	TArray<FAgentLogRow> FullProjectedRows;

	float AgentListWidth = FAgentLogConfig::DefaultAgentListWidth;
	float TimelineWidth  = FAgentLogConfig::DefaultTimelineWidth;
	float InspectorWidth = FAgentLogConfig::DefaultInspectorWidth;
	bool  bInspectorVisible = false;
};
