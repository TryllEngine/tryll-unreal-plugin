// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Misc/NotifyHook.h"
#include "UObject/GCObject.h"
#include "EdGraph/EdGraphSchema.h"
#include "TryllWorkflowValidator.h"
#include "EditorUndoClient.h"
#include "Widgets/Views/SListView.h"

class UTryllWorkflowAsset;
class UTryllEdGraph;
class UTryllEdGraphNode;
class UTryllNodeParamsBase;
class UEdGraphNode;
class SGraphEditor;
class IDetailsView;
class SWidget;

/** One validation issue as shown in the Issues panel; Node is unset for graph-level issues. */
struct FTryllValidationEntry
{
	FTryllWorkflowIssue::ESeverity Severity = FTryllWorkflowIssue::ESeverity::Warning;
	FString Message;
	TWeakObjectPtr<UTryllEdGraphNode> Node;
};

/**
 * Asset editor toolkit for UTryllWorkflowAsset: a node-graph canvas (SGraphEditor) plus a Details
 * panel for the selected node, with Validate / Auto-Layout toolbar actions.
 *
 * FTryllGraphDescription remains authoritative. The toolkit builds a transient UTryllEdGraph from
 * it on open (FTryllGraphSync::ImportDescriptionToGraph), writes structural edits straight back,
 * and captures node positions at save time.
 */
class FTryllWorkflowAssetEditor
	: public FAssetEditorToolkit
	, public FGCObject
	, public FNotifyHook
	, public FEditorUndoClient
{
public:
	void InitEditor(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost,
		UTryllWorkflowAsset* InAsset);

	// FAssetEditorToolkit / IToolkit
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void SaveAsset_Execute() override;

	// FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("FTryllWorkflowAssetEditor"); }

	// FNotifyHook — fires when the Details panel edits a node's params or DefaultModelName.
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;

	// FEditorUndoClient
	virtual bool MatchesContext(const FTransactionContext& InContext,
		const TArray<TPair<UObject*, FTransactionObjectEvent>>& TransactionObjectContexts) const override;
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;

	~FTryllWorkflowAssetEditor();

	// ── Node renaming ─────────────────────────────────────────────────────────
	// Public because FTryllNodeParamsGraphCustomization drives the Details panel's
	// "Node Name" row through them; both rename affordances share one commit point.

	/**
	 * The single commit point for node renames: validates and uniquifies the request
	 * (TryllRenamePropagation::ResolveNodeRename), writes the name under a transaction, and
	 * rewrites the slot references that no pin describes. Returns true when the graph changed.
	 *
	 * Exit targets and StartNode deliberately need no rewriting here — ExportGraphToDescription
	 * re-derives them from the live pin connections, so SyncGraphToAsset picks up the new name.
	 */
	bool CommitNodeRename(UTryllEdGraphNode* Node, const FString& RequestedName);

	/** Names of every node in the graph except Exclude (pass the node being renamed). */
	TSet<FString> CollectNodeNames(const UTryllEdGraphNode* Exclude) const;

	/** The graph node wrapping the given params object, or null. */
	UTryllEdGraphNode* FindNodeByParams(const UTryllNodeParamsBase* Params) const;

	/**
	 * Clears every node's badge state, stamps each node-attributable issue onto the node it names
	 * (bHasCompilerMessage / ErrorType / ErrorMsg), and returns the flat display list with errors
	 * first. Graph-level issues come back with an unset Node. Public so the specs can drive it
	 * without standing up a toolkit.
	 */
	static TArray<TSharedPtr<FTryllValidationEntry>> AttributeIssuesToNodes(
		const TArray<FTryllWorkflowIssue>& Issues, UTryllEdGraph* InGraph,
		int32& OutNumErrors, int32& OutNumWarnings);

private:
	TSharedRef<SDockTab> SpawnTab_Graph(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Issues(const FSpawnTabArgs& Args);

	TSharedRef<SGraphEditor> CreateGraphEditor();
	void BindGraphCommands();
	void ExtendToolbar();

	void OnSelectionChanged(const TSet<UObject*>& NewSelection);

	/** Binds Details to Node's params, or to the workflow asset when Node is null or the entry node. */
	void ShowDetailsFor(UTryllEdGraphNode* Node);

	/**
	 * Fixed bar above the Details view: the selected node's name (editable, commits through
	 * CommitNodeRename) and a help button opening its docs, or "Workflow" when the asset is shown.
	 * Lives outside the details panel so it needs no category header of its own.
	 */
	TSharedRef<SWidget> MakeDetailsHeader();

	void OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitType, UEdGraphNode* Node);

	/** Gates the canvas title editor, so an invalid name shows an inline error instead of committing. */
	bool OnVerifyNodeTitle(const FText& NewText, UEdGraphNode* Node, FText& OutErrorMessage);

	void OnGraphChanged(const struct FEdGraphEditAction& Action);

	void SyncGraphToAsset();

	/**
	 * Discards the canvas and re-derives it from WorkflowAsset->Graph.
	 *
	 * The asset is the only object the transaction records, so after an undo the canvas may still
	 * hold nodes the transaction has marked as garbage — painting one of those crashes the editor.
	 * Rebuilding is also why the graph itself is deliberately kept out of the transaction: it is a
	 * derived view, not state.
	 */
	void RebuildGraphFromAsset();

	void RunValidation();

	/** Toolbar "Validate": re-validates and additionally dumps the issue list to the Output Log. */
	void OnValidateClicked();

	/** Writes the current issue list to the Output Log under LogTryllEditor. */
	void LogValidationIssues() const;

	TSharedRef<ITableRow> OnGenerateIssueRow(TSharedPtr<FTryllValidationEntry> Entry,
		const TSharedRef<STableViewBase>& OwnerTable);

	/** Selects and centres the node an issue belongs to (no-op for graph-level issues). */
	void FocusIssueNode(TSharedPtr<FTryllValidationEntry> Entry);

	void DeleteSelectedNodes();
	bool CanDeleteSelectedNodes() const;
	void DuplicateSelectedNodes();
	bool CanDuplicateSelectedNodes() const;
	void OnAutoLayout();

	FText GetValidationSummary() const { return ValidationSummary; }

	TObjectPtr<UTryllWorkflowAsset> WorkflowAsset = nullptr;
	TObjectPtr<UTryllEdGraph> EdGraph = nullptr;

	TSharedPtr<SGraphEditor> GraphEditorWidget;
	TSharedPtr<IDetailsView> DetailsView;
	/** Node whose params Details shows; null while Details shows the workflow asset. */
	TWeakObjectPtr<UTryllEdGraphNode> DetailsNode;
	TSharedPtr<FUICommandList> GraphEditorCommands;

	FText ValidationSummary;
	FDelegateHandle GraphChangedHandle;

	TArray<TSharedPtr<FTryllValidationEntry>> ValidationEntries;
	TSharedPtr<SListView<TSharedPtr<FTryllValidationEntry>>> IssueListView;
	int32 NumValidationErrors = 0;
	int32 NumValidationWarnings = 0;

	/** Guards against re-entrant export while we are mutating the graph ourselves. */
	bool bUpdatingGraph = false;

	static const FName GraphTabId;
	static const FName DetailsTabId;
	static const FName IssuesTabId;
};
