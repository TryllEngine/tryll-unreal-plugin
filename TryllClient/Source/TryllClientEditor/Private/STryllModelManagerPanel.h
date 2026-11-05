// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "TryllEditorStatus.h"
#include "TryllModelInfo.h"
#include "TryllModelManifest.h"
#include "TryllModelManagerPresentation.h"
#include "UI/TryllPanelSnapshot.h"

using FTryllDownloadJob = FTryllModelManagerPresentation::FDownloadJob;

class UTryllRuntimeSettings;
class UTryllEditorSubsystem;
class SScrollBox;
class SBox;
class STextBlock;
class SMenuAnchor;
class SVerticalBox;

/**
 * Editor-only model manager panel mirroring the Unity TryllModelManagerWindow:
 * a three-panel layout (filters / list / properties) over the shared connection.
 *
 * Routes through UTryllEditorSubsystem — the same connection/session/agent-map
 * shared with PIE (UTryllSubsystem) and the chat panel. Auto-connects on open
 * and lists the catalog.
 */
class STryllModelManagerPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STryllModelManagerPanel)
		: _EditorSubsystemOverride(nullptr)
		, _PersistLayout(true)
	{}
		SLATE_ARGUMENT(UTryllEditorSubsystem*, EditorSubsystemOverride)
		SLATE_ARGUMENT(bool, PersistLayout)
	SLATE_END_ARGS()

	STryllModelManagerPanel();
	void Construct(const FArguments& InArgs);
	virtual ~STryllModelManagerPanel() override;

	void DebugRetryOwnerForTests() { RetryOwner(); }
	bool DebugIsAcquiredForTests() const { return bAcquiredConnection; }
	int32 DebugOwnerPhaseHandlerCountForTests() const { return DebugOwnerPhaseHandlerCount; }
	void DebugFailForTests(const FString& Reason) { Fail(Reason); }
	/** Applies a successful ListModels without the first-list missing-models prompt. */
	void DebugListModelsOkForTests(const TArray<FTryllModelInfo>& NewModels)
	{
		bMissingModelsChecked = true;
		HandleListModelsOk(NewModels);
	}
	const FString& DebugOperationalErrorForTests() const { return OperationalError; }
	FTryllPanelSnapshot DebugSnapshotForTests() const;
	/** Press toolbar Refresh as a click would. */
	void DebugRefreshForTests() { OnRefreshClicked(); }

	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;

private:
	enum class EState : uint8 { Idle, Connecting, Configuring, Loading, Ready, Error };
	enum class ESortKey : uint8 { Name, Type, Size, Status, Registration };

	// ── Layout ──────────────────────────────────────────────────────────────
	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildFiltersPanel();
	TSharedRef<SWidget> BuildListPanel();
	TSharedRef<SWidget> BuildPropertiesPanel();
	TSharedRef<SWidget> BuildDownloadsPopover();
	void                PopulateDownloads();
	void                NoteDownloadProgress(const FString& Name, uint64 Bytes, uint64 Total, float Percent);
	void                NoteDownloadComplete(const FString& Name, bool bSuccess, const FString& Error);
	int32               ActiveDownloadCount() const;
	TSharedRef<SWidget> MakeListRow(int32 Index);
	TSharedRef<SWidget> MakePropertiesContent();
	void RebuildList();
	void RebuildProperties();
	void RebuildCounts();
	void RebuildAll();
	void SelectRow(int32 Index);
	void ResetFilters();
	TSharedRef<SWidget> BuildLegend();
	void LoadLayoutConfig();
	void SaveLayoutConfig() const;

	// Visible rows after search + status filter, as indices into Models.
	TArray<int32> FilteredIndices() const;

	// ── Actions ─────────────────────────────────────────────────────────────
	void OnDownload(const FString& ModelName);
	void OnDownloadAll();
	void OnLoadToggle(const FString& ModelName, bool bLoad);
	void OnDeleteModel(const FString& ModelName);

	/**
	 * feat-011: on the first model list after connecting, ask whether to download any
	 * Production/Experimental registered model that is Absent on disk. Prompts once per
	 * connect — unlike the Play/Chat guards, this panel never auto-downloads.
	 */
	void PromptDownloadMissingRegisteredModels();

	// ── Build registration (manifest) ─────────────────────────────────────────
	ETryllModelRegistration RegOf(const FString& ModelName) const;
	void SetRegistration(const FString& ModelName, ETryllModelRegistration Reg);

	/** Manifest placement for a model, or Auto when it has none. */
	ETryllMemoryPlacement PlacementOf(const FString& ModelName) const;

	/**
	 * Persist a placement choice. Never adds or removes an entry — an
	 * unregistered model has no placement to store, and the control is only
	 * offered once a model is registered.
	 */
	void SetPlacement(const FString& ModelName, ETryllMemoryPlacement Placement);

	// ── STT ↔ VAD companion helpers ───────────────────────────────────────────
	/** True when `Name` is a catalog STT model in the live ListModels snapshot. */
	/// Whether a placement choice can do anything for this model. False for VAD —
	/// see the definition for why that is a property of VAD, not of the editor.
	bool SupportsPlacement(const FString& Name) const;

	/** Whether hotword biasing means anything for this model. STT only. */
	bool SupportsHotwordBias(const FString& ModelName) const;

	/** This project's hotword-bias choice for a model, Default when it has none. */
	ETryllHotwordBias HotwordBiasOf(const FString& ModelName) const;

	/** Persist a hotword-bias choice. No-op for an unregistered model. */
	void SetHotwordBias(const FString& ModelName, ETryllHotwordBias Bias);

	bool IsSttModel(const FString& Name) const;
	/** Catalog VAD model names (companions for offline STT endpointing). */
	TArray<FString> VadCompanions() const;

	// ── Status ──────────────────────────────────────────────────────────────
	void        RefreshStatusChrome();
	void        HandleHostColorsRefreshed();
	void        HandleOwnerPhaseChanged(ETryllOwnerPhase Phase, const FString& Detail);
	void        SetState(EState S, const FString& Status);
	void        Fail(const FString& Reason);

	// ── Connection ──────────────────────────────────────────────────────────
	void Start();
	void Stop();
	void RetryOwner();
	void OnRefreshClicked();
	void AutofindSettings();
	void RequestList();
	void HandleListModelsOk(const TArray<FTryllModelInfo>& NewModels);

	// Subscribed to the shared UTryllEditorSubsystem's native delegates.
	void HandleConnectionChanged(bool bConnected);
	void HandleCreateSessionComplete(bool bSuccess);
	void HandleError(const FString& ErrorMessage);
	void HandleDownloadProgress(const FString& ModelName, uint64 BytesDownloaded, uint64 TotalBytes, float Percent);
	void HandleDownloadComplete(const FString& ModelName, bool bSuccess, const FString& ErrorMessage);
	void HandleLoadModelComplete(const FString& ModelName, bool bSuccess);
	void HandleUnloadModelComplete(const FString& ModelName, bool bSuccess);
	void HandleDeleteModelComplete(const FString& ModelName, bool bSuccess);

	// ── State ───────────────────────────────────────────────────────────────
	TWeakObjectPtr<UTryllRuntimeSettings> Settings;
	EState  State  = EState::Idle;
	FString Status = TEXT("Idle");

	/// Resolved fresh in Start() from GEditor; a UEditorSubsystem outlives any
	/// Slate window for the life of the editor process, so a raw pointer (cleared
	/// in Stop()) is safe without weak-object bookkeeping.
	UTryllEditorSubsystem* EditorSubsystem = nullptr;
	UTryllEditorSubsystem* EditorSubsystemOverride = nullptr;
	bool bPersistLayout = true;
	bool bAcquiredConnection = false;
	int32 DebugOwnerPhaseHandlerCount = 0;

	/// Set by HandleError() and consumed by bool-only completions that still
	/// lack an error string (CreateSession / DeleteModel). Download failures
	/// carry ErrorMessage on OnDownloadCompleteNative directly.
	FString LastErrorMessage;

	TArray<FTryllModelInfo>     Models;
	TMap<FString, float>        Progress;       // model name -> percent, while downloading
	int32                       SelectedIndex = INDEX_NONE;
	bool                        bMissingModelsChecked = false; // feat-011: reset in Start()
	int32                       ListGeneration = 0;

	TArray<FTryllDownloadJob>   Downloads;      // newest first; powers the downloads popover
	TSharedPtr<SMenuAnchor>     DownloadsAnchor;
	TSharedPtr<SVerticalBox>    DownloadsList;  // valid only while the popover is open

	FString                     SearchText;
	ESortKey                    SortKey = ESortKey::Name;
	TArray<TSharedPtr<FString>> SortOptions;
	TSet<ETryllModelStatus>     StatusFilter;   // empty set is treated as "all"
	TSet<FString>               TypeFilter;     // model_type wire values; empty = all
	TSet<FString>               EngineFilter;   // engine wire values; empty = all
	TSet<FString>               AudienceFilter; // audience wire values; empty = all
	TSet<ETryllModelRegistration> RegFilter;    // build registration; empty = all

	TSharedPtr<class STryllInspectorSplit> OuterSplitter;
	TSharedPtr<class STryllInspectorSplit> InnerSplitter;
	float FilterWidth = 250.0f;
	float PropsWidth  = 360.0f;

	TSharedPtr<class STryllStatusBar> StatusBar;
	TSharedPtr<class STryllBanner>    Banner;
	FString OperationalError;
	FDelegateHandle OwnerPhaseHandle;
	FDelegateHandle HostColorsHandle;

	TSharedPtr<SScrollBox> ListScroll;
	TSharedPtr<SBox>       PropertiesContainer;
	TSharedPtr<STextBlock> CountText;
	TSharedPtr<STextBlock> TotalDownloaded;
	TSharedPtr<STextBlock> TotalDisk;
	TSharedPtr<STextBlock> TotalProd;
	TSharedPtr<STextBlock> TotalExp;
	TSharedPtr<STextBlock> TotalShip;
	TSharedPtr<class SEditableTextBox> SearchBox;
};
