// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "Graph/TryllWorkflowAssetEditor.h"
#include "Graph/TryllEdGraph.h"
#include "Graph/TryllEdGraphNode.h"
#include "Graph/TryllEdGraphSchema.h"
#include "Graph/TryllGraphSync.h"
#include "Graph/TryllWorkflowGraphCustomizations.h"

#include "TryllWorkflowAsset.h"
#include "TryllGraphDescription.h"
#include "TryllWorkflowValidator.h"
#include "TryllWorkflowVariablesValidator.h"
#include "TryllRenamePropagation.h"
#include "TryllClientEditorModule.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"

#include "GraphEditor.h"
#include "SGraphNode.h"
#include "Editor.h"
#include "Algo/StableSort.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphUtilities.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/PlatformProcess.h"
#include "Styling/AppStyle.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Views/STableRow.h"
#include "ScopedTransaction.h"
#include "Logging/TokenizedMessage.h"

#define LOCTEXT_NAMESPACE "TryllWorkflowAssetEditor"

const FName FTryllWorkflowAssetEditor::GraphTabId(TEXT("TryllWorkflowGraph_Graph"));
const FName FTryllWorkflowAssetEditor::DetailsTabId(TEXT("TryllWorkflowGraph_Details"));
const FName FTryllWorkflowAssetEditor::IssuesTabId(TEXT("TryllWorkflowGraph_Issues"));

namespace
{
	const FName AppIdentifier(TEXT("TryllWorkflowAssetEditorApp"));

	/** If a validator message names a node ("Node 'x' …" / "Node name 'x' …"), pull out that name. */
	bool ExtractNodeName(const FString& Message, FString& OutName)
	{
		static const FString Prefixes[] = { TEXT("Node '"), TEXT("Node name '") };

		for (const FString& Prefix : Prefixes)
		{
			if (!Message.StartsWith(Prefix))
			{
				continue;
			}
			const int32 Start = Prefix.Len();
			const int32 End = Message.Find(TEXT("'"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Start);
			if (End > Start)
			{
				OutName = Message.Mid(Start, End - Start);
				return true;
			}
		}
		return false;
	}
}

FTryllWorkflowAssetEditor::~FTryllWorkflowAssetEditor()
{
	if (GEditor)
	{
		GEditor->UnregisterForUndo(this);
	}
	if (EdGraph && GraphChangedHandle.IsValid())
	{
		EdGraph->RemoveOnGraphChangedHandler(GraphChangedHandle);
	}
}

void FTryllWorkflowAssetEditor::InitEditor(EToolkitMode::Type Mode,
	const TSharedPtr<IToolkitHost>& InitToolkitHost, UTryllWorkflowAsset* InAsset)
{
	WorkflowAsset = InAsset;

	// Build the transient editor graph from the authoritative description.
	EdGraph = NewObject<UTryllEdGraph>(GetTransientPackage(), NAME_None, RF_Transient);
	EdGraph->Schema = UTryllEdGraphSchema::StaticClass();
	EdGraph->WorkflowAsset = WorkflowAsset;
	FTryllGraphSync::ImportDescriptionToGraph(WorkflowAsset, EdGraph);

	GraphChangedHandle = EdGraph->AddOnGraphChangedHandler(
		FOnGraphChanged::FDelegate::CreateRaw(this, &FTryllWorkflowAssetEditor::OnGraphChanged));

	BindGraphCommands();

	// Details view with graph-scoped customizations (exits hidden; entry shows DefaultModelName).
	FPropertyEditorModule& PropertyModule =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.NotifyHook = this;
	DetailsView = PropertyModule.CreateDetailView(DetailsArgs);
	// The params object carries no name of its own; MakeDetailsHeader shows and renames the
	// owning node above the panel.
	DetailsView->RegisterInstancedCustomPropertyLayout(
		UTryllNodeParamsBase::StaticClass(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FTryllNodeParamsGraphCustomization::MakeInstance));
	DetailsView->RegisterInstancedCustomPropertyLayout(
		UTryllWorkflowAsset::StaticClass(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FTryllWorkflowAssetGraphCustomization::MakeInstance));
	DetailsView->SetObject(WorkflowAsset);

	const TSharedRef<FTabManager::FLayout> Layout =
		FTabManager::NewLayout("TryllWorkflowAssetEditor_Layout_v2")
		->AddArea(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
			->Split(
				FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
				->SetSizeCoefficient(0.72f)
				->Split(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.72f)
					->AddTab(GraphTabId, ETabState::OpenedTab)
					->SetHideTabWell(true))
				->Split(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.28f)
					->AddTab(IssuesTabId, ETabState::OpenedTab)))
			->Split(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.28f)
				->AddTab(DetailsTabId, ETabState::OpenedTab)));

	ExtendToolbar();

	InitAssetEditor(Mode, InitToolkitHost, AppIdentifier, Layout,
		/*bCreateDefaultStandaloneMenu*/ true, /*bCreateDefaultToolbar*/ true, WorkflowAsset);

	if (GEditor)
	{
		GEditor->RegisterForUndo(this);
	}

	RunValidation();
}

// ── Toolkit identity ──────────────────────────────────────────────────────────

FName FTryllWorkflowAssetEditor::GetToolkitFName() const { return FName("TryllWorkflowAssetEditor"); }
FText FTryllWorkflowAssetEditor::GetBaseToolkitName() const { return LOCTEXT("ToolkitName", "Tryll Workflow Editor"); }
FString FTryllWorkflowAssetEditor::GetWorldCentricTabPrefix() const { return LOCTEXT("TabPrefix", "Workflow ").ToString(); }
FLinearColor FTryllWorkflowAssetEditor::GetWorldCentricTabColorScale() const { return FLinearColor(0.16f, 0.44f, 0.78f, 0.5f); }

void FTryllWorkflowAssetEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(EdGraph);
	Collector.AddReferencedObject(WorkflowAsset);
}

// ── Tabs ──────────────────────────────────────────────────────────────────────

void FTryllWorkflowAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	const TSharedRef<FWorkspaceItem> Group = InTabManager->AddLocalWorkspaceMenuCategory(
		LOCTEXT("WorkspaceMenu", "Tryll Workflow Editor"));

	InTabManager->RegisterTabSpawner(GraphTabId,
		FOnSpawnTab::CreateRaw(this, &FTryllWorkflowAssetEditor::SpawnTab_Graph))
		.SetDisplayName(LOCTEXT("GraphTab", "Graph"))
		.SetGroup(Group)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x"));

	InTabManager->RegisterTabSpawner(DetailsTabId,
		FOnSpawnTab::CreateRaw(this, &FTryllWorkflowAssetEditor::SpawnTab_Details))
		.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
		.SetGroup(Group)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(IssuesTabId,
		FOnSpawnTab::CreateRaw(this, &FTryllWorkflowAssetEditor::SpawnTab_Issues))
		.SetDisplayName(LOCTEXT("IssuesTab", "Issues"))
		.SetGroup(Group)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Warning"));
}

void FTryllWorkflowAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(GraphTabId);
	InTabManager->UnregisterTabSpawner(DetailsTabId);
	InTabManager->UnregisterTabSpawner(IssuesTabId);
}

TSharedRef<SDockTab> FTryllWorkflowAssetEditor::SpawnTab_Graph(const FSpawnTabArgs& Args)
{
	GraphEditorWidget = CreateGraphEditor();
	return SNew(SDockTab)
		.Label(LOCTEXT("GraphTabLabel", "Graph"))
		[
			GraphEditorWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FTryllWorkflowAssetEditor::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("DetailsTabLabel", "Details"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				MakeDetailsHeader()
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				DetailsView.ToSharedRef()
			]
		];
}

TSharedRef<SWidget> FTryllWorkflowAssetEditor::MakeDetailsHeader()
{
	auto NodeVisibility = [this](bool bForNode)
	{
		return TAttribute<EVisibility>::CreateLambda([this, bForNode]
		{
			return DetailsNode.IsValid() == bForNode ? EVisibility::Visible : EVisibility::Collapsed;
		});
	};

	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
		.Padding(FMargin(8.f, 6.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				SNew(SEditableTextBox)
				.Visibility(NodeVisibility(true))
				.Font(FAppStyle::GetFontStyle("BoldFont"))
				.ToolTipText(LOCTEXT("NodeNameTooltip",
					"Instance name for this node, and the default name of the slot it writes. "
					"Renaming rewrites `input` selectors and slot template references that point at it."))
				.Text_Lambda([this]
				{
					return DetailsNode.IsValid() ? FText::FromName(DetailsNode->NodeName) : FText::GetEmpty();
				})
				.OnVerifyTextChanged_Lambda([this](const FText& NewText, FText& OutError)
				{
					if (!DetailsNode.IsValid())
					{
						return true;
					}
					const TryllRenamePropagation::FNodeRenameOutcome Outcome =
						TryllRenamePropagation::ResolveNodeRename(NewText.ToString(), CollectNodeNames(DetailsNode.Get()));
					if (!Outcome.bValid)
					{
						OutError = Outcome.Error;
						return false;
					}
					return true;
				})
				.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type CommitType)
				{
					if ((CommitType == ETextCommit::OnEnter || CommitType == ETextCommit::OnUserMovedFocus)
						&& DetailsNode.IsValid())
					{
						CommitNodeRename(DetailsNode.Get(), NewText.ToString());
					}
				})
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Visibility(NodeVisibility(false))
				.Font(FAppStyle::GetFontStyle("BoldFont"))
				.Text(LOCTEXT("WorkflowHeader", "Workflow"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4.f, 0.f, 0.f, 0.f)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ToolTipText(LOCTEXT("OpenNodeDocsTooltip", "Open this node type's documentation"))
				.Visibility(NodeVisibility(true))
				.OnClicked_Lambda([this]
				{
					if (DetailsNode.IsValid() && DetailsNode->NodeParams)
					{
						const FString Url = DetailsNode->NodeParams->GetDocsUrl();
						if (!Url.IsEmpty())
						{
							FPlatformProcess::LaunchURL(*Url, nullptr, nullptr);
						}
					}
					return FReply::Handled();
				})
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.Help"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			]
		];
}

TSharedRef<SDockTab> FTryllWorkflowAssetEditor::SpawnTab_Issues(const FSpawnTabArgs& Args)
{
	SAssignNew(IssueListView, SListView<TSharedPtr<FTryllValidationEntry>>)
		.ListItemsSource(&ValidationEntries)
		.SelectionMode(ESelectionMode::Single)
		.OnGenerateRow_Raw(this, &FTryllWorkflowAssetEditor::OnGenerateIssueRow)
		.OnMouseButtonClick_Raw(this, &FTryllWorkflowAssetEditor::FocusIssueNode);

	return SNew(SDockTab)
		.Label(LOCTEXT("IssuesTabLabel", "Issues"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("Brushes.Header"))
				.Padding(FMargin(8.f, 4.f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.TextStyle(FAppStyle::Get(), "SmallText")
						.Text_Raw(this, &FTryllWorkflowAssetEditor::GetValidationSummary)
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.TextStyle(FAppStyle::Get(), "SmallText")
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						.Text(LOCTEXT("IssuesHint", "Click an issue to select its node"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					IssueListView.ToSharedRef()
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.Text(LOCTEXT("IssuesEmpty", "No issues — this workflow validates cleanly."))
					.Visibility_Lambda([this]
					{
						return ValidationEntries.IsEmpty() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
					})
				]
			]
		];
}

TSharedRef<ITableRow> FTryllWorkflowAssetEditor::OnGenerateIssueRow(
	TSharedPtr<FTryllValidationEntry> Entry, const TSharedRef<STableViewBase>& OwnerTable)
{
	const bool bIsError = Entry.IsValid()
		&& Entry->Severity == FTryllWorkflowIssue::ESeverity::Error;

	const FText RowTooltip = (Entry.IsValid() && Entry->Node.IsValid())
		? LOCTEXT("IssueRowNodeTooltip", "Click to select this node in the graph.")
		: LOCTEXT("IssueRowGraphTooltip", "Graph-level issue — not attributable to a single node.");

	return SNew(STableRow<TSharedPtr<FTryllValidationEntry>>, OwnerTable)
		.ToolTipText(RowTooltip)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(6.f, 3.f, 6.f, 3.f))
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush(bIsError ? "Icons.Error" : "Icons.Warning"))
				.ColorAndOpacity(FAppStyle::Get().GetSlateColor(bIsError ? "Colors.Error" : "Colors.Warning"))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 3.f, 6.f, 3.f))
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Text(FText::FromString(Entry.IsValid() ? Entry->Message : FString()))
			]
		];
}

void FTryllWorkflowAssetEditor::FocusIssueNode(TSharedPtr<FTryllValidationEntry> Entry)
{
	if (!Entry.IsValid() || !GraphEditorWidget.IsValid())
	{
		return;
	}
	if (UTryllEdGraphNode* Node = Entry->Node.Get())
	{
		GraphEditorWidget->ClearSelectionSet();
		GraphEditorWidget->JumpToNode(Node, /*bRequestRename*/ false, /*bSelectNode*/ true);
	}
}

// ── Graph editor widget ───────────────────────────────────────────────────────

TSharedRef<SGraphEditor> FTryllWorkflowAssetEditor::CreateGraphEditor()
{
	SGraphEditor::FGraphEditorEvents Events;
	Events.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateRaw(
		this, &FTryllWorkflowAssetEditor::OnSelectionChanged);
	Events.OnTextCommitted = FOnNodeTextCommitted::CreateRaw(
		this, &FTryllWorkflowAssetEditor::OnNodeTitleCommitted);
	Events.OnVerifyTextCommit = FOnNodeVerifyTextCommit::CreateRaw(
		this, &FTryllWorkflowAssetEditor::OnVerifyNodeTitle);

	FGraphAppearanceInfo Appearance;
	Appearance.CornerText = LOCTEXT("CornerText", "Tryll Workflow");

	return SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.Appearance(Appearance)
		.GraphToEdit(EdGraph)
		.GraphEvents(Events);
}

void FTryllWorkflowAssetEditor::BindGraphCommands()
{
	GraphEditorCommands = MakeShared<FUICommandList>();
	const FGenericCommands& Generic = FGenericCommands::Get();

	GraphEditorCommands->MapAction(Generic.Delete,
		FExecuteAction::CreateRaw(this, &FTryllWorkflowAssetEditor::DeleteSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FTryllWorkflowAssetEditor::CanDeleteSelectedNodes));

	GraphEditorCommands->MapAction(Generic.Duplicate,
		FExecuteAction::CreateRaw(this, &FTryllWorkflowAssetEditor::DuplicateSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FTryllWorkflowAssetEditor::CanDuplicateSelectedNodes));

	GraphEditorCommands->MapAction(Generic.SelectAll,
		FExecuteAction::CreateLambda([this]
		{
			if (GraphEditorWidget.IsValid())
			{
				GraphEditorWidget->SelectAllNodes();
			}
		}));
}

void FTryllWorkflowAssetEditor::ExtendToolbar()
{
	TSharedPtr<FExtender> Extender = MakeShared<FExtender>();
	Extender->AddToolBarExtension("Asset", EExtensionHook::After, GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateLambda([this](FToolBarBuilder& Builder)
		{
			Builder.BeginSection("TryllWorkflow");

			Builder.AddToolBarButton(
				FUIAction(FExecuteAction::CreateRaw(this, &FTryllWorkflowAssetEditor::OnValidateClicked)),
				NAME_None,
				LOCTEXT("ValidateLabel", "Validate"),
				LOCTEXT("ValidateTooltip",
					"Re-check the workflow for errors and warnings. Results appear in the Issues panel, "
					"as badges on the offending nodes, and in the Output Log."),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Validate"));

			Builder.AddToolBarButton(
				FUIAction(FExecuteAction::CreateRaw(this, &FTryllWorkflowAssetEditor::OnAutoLayout)),
				NAME_None,
				LOCTEXT("AutoLayoutLabel", "Auto Layout"),
				LOCTEXT("AutoLayoutTooltip", "Re-arrange all nodes left-to-right from the entry point."),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.AlignNodesTop"));

			Builder.AddWidget(
				SNew(STextBlock)
				.Margin(FMargin(8.f, 0.f))
				.Text_Raw(this, &FTryllWorkflowAssetEditor::GetValidationSummary));

			Builder.EndSection();
		}));

	AddToolbarExtender(Extender);
}

// ── Selection / edits ─────────────────────────────────────────────────────────

void FTryllWorkflowAssetEditor::OnSelectionChanged(const TSet<UObject*>& NewSelection)
{
	if (!DetailsView.IsValid())
	{
		return;
	}

	UTryllEdGraphNode* Node = NewSelection.Num() == 1
		? Cast<UTryllEdGraphNode>(*NewSelection.CreateConstIterator())
		: nullptr;
	// Nothing (or many) selected: show the workflow-level fields.
	ShowDetailsFor(Node);
}

void FTryllWorkflowAssetEditor::ShowDetailsFor(UTryllEdGraphNode* Node)
{
	if (!DetailsView.IsValid())
	{
		return;
	}
	if (Node && !Node->bIsEntryNode && Node->NodeParams)
	{
		DetailsNode = Node;
		DetailsView->SetObject(Node->NodeParams);
	}
	else
	{
		DetailsNode = nullptr;
		DetailsView->SetObject(WorkflowAsset);
	}
}

TSet<FString> FTryllWorkflowAssetEditor::CollectNodeNames(const UTryllEdGraphNode* Exclude) const
{
	TSet<FString> Names;
	if (!EdGraph)
	{
		return Names;
	}
	for (UEdGraphNode* Raw : EdGraph->Nodes)
	{
		const UTryllEdGraphNode* Node = Cast<UTryllEdGraphNode>(Raw);
		if (!Node || Node == Exclude || Node->bIsEntryNode || Node->NodeName.IsNone())
		{
			continue;
		}
		Names.Add(Node->NodeName.ToString());
	}
	return Names;
}

UTryllEdGraphNode* FTryllWorkflowAssetEditor::FindNodeByParams(const UTryllNodeParamsBase* Params) const
{
	if (!EdGraph || !Params)
	{
		return nullptr;
	}
	for (UEdGraphNode* Raw : EdGraph->Nodes)
	{
		UTryllEdGraphNode* Node = Cast<UTryllEdGraphNode>(Raw);
		if (Node && Node->NodeParams == Params)
		{
			return Node;
		}
	}
	return nullptr;
}

bool FTryllWorkflowAssetEditor::CommitNodeRename(UTryllEdGraphNode* Node, const FString& RequestedName)
{
	if (!Node || Node->bIsEntryNode || !WorkflowAsset || !EdGraph)
	{
		return false;
	}

	const FString OldName = Node->NodeName.IsNone() ? FString() : Node->NodeName.ToString();

	const TryllRenamePropagation::FNodeRenameOutcome Outcome =
		TryllRenamePropagation::ResolveNodeRename(RequestedName, CollectNodeNames(Node));
	if (!Outcome.bValid)
	{
		UE_LOG(LogTryllEditor, Warning, TEXT("[Tryll] Node rename to '%s' rejected: %s"),
		       *RequestedName, *Outcome.Error.ToString());
		return false;
	}
	if (Outcome.Name == OldName)
	{
		return false;
	}
	if (Outcome.Name != RequestedName.TrimStartAndEnd())
	{
		UE_LOG(LogTryllEditor, Log, TEXT("[Tryll] Node name '%s' is already taken; using '%s'."),
		       *RequestedName.TrimStartAndEnd(), *Outcome.Name);
	}

	// Sampled *before* the write: whether this node's output slot follows its own name is what
	// decides if slot references point at the old name at all. A node with an explicit
	// output_name override keeps writing that slot, so nothing downstream needs rewriting.
	const bool bRewriteSlot =
		!OldName.IsEmpty() && TryllRenamePropagation::EffectiveSlotIsNodeName(Node->NodeParams);

	const FScopedTransaction Transaction(LOCTEXT("RenameNode", "Rename Tryll Node"));
	Node->Modify();

	if (bRewriteSlot)
	{
		// PropagateSlotRename edits params objects in place; they are asset-owned and
		// RF_Transactional, so they have to enter the transaction to be undoable.
		for (UEdGraphNode* Raw : EdGraph->Nodes)
		{
			UTryllEdGraphNode* Other = Cast<UTryllEdGraphNode>(Raw);
			if (Other && Other->NodeParams)
			{
				Other->NodeParams->Modify();
			}
		}
	}

	Node->NodeName = FName(*Outcome.Name);

	// Exit targets and StartNode are derived from the live pin connections by
	// ExportGraphToDescription, so this sync alone rewrites them to the new name.
	SyncGraphToAsset();

	// Slot references live in params data that no pin describes — rewrite those by hand,
	// after the sync so Graph.Nodes is populated.
	if (bRewriteSlot)
	{
		TryllRenamePropagation::PropagateOwnedSlotRename(
			WorkflowAsset->Graph, Node->NodeParams, OldName, Outcome.Name);
		WorkflowAsset->MarkPackageDirty();
	}

	RunValidation();
	return true;
}

void FTryllWorkflowAssetEditor::OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitType, UEdGraphNode* Node)
{
	// Escape (OnCleared) and programmatic changes must not commit a rename.
	if (CommitType != ETextCommit::OnEnter && CommitType != ETextCommit::OnUserMovedFocus)
	{
		return;
	}
	CommitNodeRename(Cast<UTryllEdGraphNode>(Node), NewText.ToString());
}

bool FTryllWorkflowAssetEditor::OnVerifyNodeTitle(const FText& NewText, UEdGraphNode* Node, FText& OutErrorMessage)
{
	const UTryllEdGraphNode* TryllNode = Cast<UTryllEdGraphNode>(Node);
	if (!TryllNode || TryllNode->bIsEntryNode)
	{
		return true;
	}

	// A name that merely collides is accepted here and uniquified on commit; only names the
	// graph can never carry are refused, so the user sees why while still typing.
	const TryllRenamePropagation::FNodeRenameOutcome Outcome =
		TryllRenamePropagation::ResolveNodeRename(NewText.ToString(), CollectNodeNames(TryllNode));
	if (!Outcome.bValid)
	{
		OutErrorMessage = Outcome.Error;
		return false;
	}
	return true;
}

void FTryllWorkflowAssetEditor::OnGraphChanged(const FEdGraphEditAction& Action)
{
	if (bUpdatingGraph)
	{
		return;
	}
	SyncGraphToAsset();
	RunValidation();
}

void FTryllWorkflowAssetEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	// Details edits mutate the asset-owned params / DefaultModelName directly; just persist + re-check.
	if (WorkflowAsset)
	{
		WorkflowAsset->MarkPackageDirty();
	}
	RunValidation();
}

// ── Sync + validation ─────────────────────────────────────────────────────────

void FTryllWorkflowAssetEditor::RebuildGraphFromAsset()
{
	if (!WorkflowAsset || !EdGraph)
	{
		return;
	}

	// The guard is essential, not defensive: ImportDescriptionToGraph fires OnGraphChanged, whose
	// SyncGraphToAsset would export the half-built canvas straight back over the asset the undo
	// just restored — silently cancelling the undo.
	TGuardValue<bool> Guard(bUpdatingGraph, true);

	if (GraphEditorWidget.IsValid())
	{
		GraphEditorWidget->ClearSelectionSet();
	}

	// Details may be bound to a params object the undo has marked as garbage.
	if (DetailsView.IsValid())
	{
		DetailsNode = nullptr;
		DetailsView->SetObject(WorkflowAsset);
	}

	FTryllGraphSync::ImportDescriptionToGraph(WorkflowAsset, EdGraph);

	if (GraphEditorWidget.IsValid())
	{
		GraphEditorWidget->NotifyGraphChanged();
	}
}

bool FTryllWorkflowAssetEditor::MatchesContext(const FTransactionContext& InContext,
	const TArray<TPair<UObject*, FTransactionObjectEvent>>& TransactionObjectContexts) const
{
	if (!WorkflowAsset)
	{
		return false;
	}
	for (const TPair<UObject*, FTransactionObjectEvent>& Pair : TransactionObjectContexts)
	{
		UObject* Object = Pair.Key;
		if (Object == WorkflowAsset || (Object && Object->IsIn(WorkflowAsset)))
		{
			return true;
		}
	}
	return false;
}

void FTryllWorkflowAssetEditor::PostUndo(bool bSuccess)
{
	RebuildGraphFromAsset();
	RunValidation();
}

void FTryllWorkflowAssetEditor::PostRedo(bool bSuccess)
{
	RebuildGraphFromAsset();
	RunValidation();
}

void FTryllWorkflowAssetEditor::SyncGraphToAsset()
{
	if (!WorkflowAsset || !EdGraph)
	{
		return;
	}
	WorkflowAsset->Modify();

	// Exit targets are properties of each node's Instanced params object, not fields of Graph, and
	// an object's transaction record covers only its own properties — for a reference, the pointer
	// and not the pointee. So the asset's record alone cannot undo a re-wire; every params object
	// the export is about to write must enter the transaction itself. StartNode is the exception
	// that proved this: it lives on Graph, which is why unlinking the Entry pin was the only
	// undoable wire edit. RF_Transactional is asserted because params built through
	// UTryllNodeParamsFactory (the Blueprint path) carry no flags and would silently not record.
	for (UEdGraphNode* Raw : EdGraph->Nodes)
	{
		UTryllEdGraphNode* Node = Cast<UTryllEdGraphNode>(Raw);
		if (Node && Node->NodeParams)
		{
			Node->NodeParams->SetFlags(RF_Transactional);
			Node->NodeParams->Modify();
		}
	}

	FTryllGraphSync::ExportGraphToDescription(EdGraph, WorkflowAsset->Graph);
	WorkflowAsset->MarkPackageDirty();
}

TArray<TSharedPtr<FTryllValidationEntry>> FTryllWorkflowAssetEditor::AttributeIssuesToNodes(
	const TArray<FTryllWorkflowIssue>& Issues, UTryllEdGraph* InGraph,
	int32& OutNumErrors, int32& OutNumWarnings)
{
	OutNumErrors = 0;
	OutNumWarnings = 0;

	TArray<TSharedPtr<FTryllValidationEntry>> Entries;
	if (!InGraph)
	{
		return Entries;
	}

	TMap<FString, UTryllEdGraphNode*> ByName;
	for (UEdGraphNode* Raw : InGraph->Nodes)
	{
		if (UTryllEdGraphNode* Node = Cast<UTryllEdGraphNode>(Raw))
		{
			Node->bHasCompilerMessage = false;
			Node->ErrorMsg.Empty();
			Node->ErrorType = EMessageSeverity::Info;

			if (!Node->bIsEntryNode && !Node->NodeName.IsNone())
			{
				ByName.Add(Node->NodeName.ToString(), Node);
			}
		}
	}

	for (const FTryllWorkflowIssue& Issue : Issues)
	{
		const bool bIsError = (Issue.Severity == FTryllWorkflowIssue::ESeverity::Error);
		bIsError ? ++OutNumErrors : ++OutNumWarnings;

		UTryllEdGraphNode* Node = nullptr;
		FString NodeName;
		if (ExtractNodeName(Issue.Message, NodeName))
		{
			if (UTryllEdGraphNode** NodePtr = ByName.Find(NodeName))
			{
				Node = *NodePtr;
				const int32 Sev = bIsError ? (int32)EMessageSeverity::Error : (int32)EMessageSeverity::Warning;
				// Keep the most severe (lower enum value = more severe).
				if (!Node->bHasCompilerMessage || Sev < Node->ErrorType)
				{
					Node->ErrorType = Sev;
				}
				Node->bHasCompilerMessage = true;
				Node->ErrorMsg = Node->ErrorMsg.IsEmpty()
					? Issue.Message
					: Node->ErrorMsg + TEXT("\n") + Issue.Message;
			}
		}

		TSharedPtr<FTryllValidationEntry> Entry = MakeShared<FTryllValidationEntry>();
		Entry->Severity = Issue.Severity;
		Entry->Message = Issue.Message;
		Entry->Node = Node;
		Entries.Add(Entry);
	}

	// Errors first, then warnings; original order is preserved within each group.
	Algo::StableSortBy(Entries,
		[](const TSharedPtr<FTryllValidationEntry>& Entry) { return (int32)Entry->Severity; },
		TGreater<int32>());

	return Entries;
}

void FTryllWorkflowAssetEditor::RunValidation()
{
	if (!WorkflowAsset || !EdGraph)
	{
		return;
	}

	TArray<FTryllWorkflowIssue> Issues = FTryllWorkflowValidator::Validate(WorkflowAsset->Graph);
	Issues.Append(TryllWorkflowVariablesValidator::Validate(WorkflowAsset->Graph, WorkflowAsset->Variables));

	ValidationEntries = AttributeIssuesToNodes(Issues, EdGraph,
		NumValidationErrors, NumValidationWarnings);

	if (NumValidationErrors == 0 && NumValidationWarnings == 0)
	{
		ValidationSummary = LOCTEXT("ValidationOk", "No issues");
	}
	else
	{
		ValidationSummary = FText::Format(
			LOCTEXT("ValidationSummaryFmt", "{0} error(s), {1} warning(s)"),
			FText::AsNumber(NumValidationErrors), FText::AsNumber(NumValidationWarnings));
	}

	if (IssueListView.IsValid())
	{
		IssueListView->RequestListRefresh();
	}

	if (GraphEditorWidget.IsValid())
	{
		// Guard so refreshing node badges doesn't re-enter OnGraphChanged.
		TGuardValue<bool> Guard(bUpdatingGraph, true);
		GraphEditorWidget->NotifyGraphChanged();
	}

	// NotifyGraphChanged reuses existing node widgets, and each one caches its error state at
	// construction — without an explicit refresh the badges never appear. FBlueprintEditor does
	// the same after a compile.
	for (UEdGraphNode* Raw : EdGraph->Nodes)
	{
		if (Raw)
		{
			if (const TSharedPtr<SGraphNode> Widget = Raw->DEPRECATED_NodeWidget.Pin())
			{
				Widget->RefreshErrorInfo();
			}
		}
	}
}

void FTryllWorkflowAssetEditor::OnValidateClicked()
{
	RunValidation();
	LogValidationIssues();
}

void FTryllWorkflowAssetEditor::LogValidationIssues() const
{
	const FString AssetName = WorkflowAsset ? WorkflowAsset->GetName() : TEXT("<none>");

	if (ValidationEntries.IsEmpty())
	{
		UE_LOG(LogTryllEditor, Display, TEXT("Workflow '%s': no issues."), *AssetName);
		return;
	}

	UE_LOG(LogTryllEditor, Display, TEXT("Workflow '%s': %d error(s), %d warning(s)."),
		*AssetName, NumValidationErrors, NumValidationWarnings);

	for (const TSharedPtr<FTryllValidationEntry>& Entry : ValidationEntries)
	{
		if (Entry->Severity == FTryllWorkflowIssue::ESeverity::Error)
		{
			UE_LOG(LogTryllEditor, Error, TEXT("  [error] %s"), *Entry->Message);
		}
		else
		{
			UE_LOG(LogTryllEditor, Warning, TEXT("  [warning] %s"), *Entry->Message);
		}
	}
}

// ── Commands ──────────────────────────────────────────────────────────────────

void FTryllWorkflowAssetEditor::DeleteSelectedNodes()
{
	if (!GraphEditorWidget.IsValid())
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("DeleteNodes", "Delete Node(s)"));
	TGuardValue<bool> Guard(bUpdatingGraph, true);

	const FGraphPanelSelectionSet Selected = GraphEditorWidget->GetSelectedNodes();
	GraphEditorWidget->ClearSelectionSet();

	for (UObject* Obj : Selected)
	{
		if (UTryllEdGraphNode* Node = Cast<UTryllEdGraphNode>(Obj))
		{
			if (!Node->CanUserDeleteNode())
			{
				continue;
			}
			Node->Modify();
			Node->DestroyNode();
		}
	}

	SyncGraphToAsset();
	RunValidation();
}

bool FTryllWorkflowAssetEditor::CanDeleteSelectedNodes() const
{
	if (!GraphEditorWidget.IsValid())
	{
		return false;
	}
	for (UObject* Obj : GraphEditorWidget->GetSelectedNodes())
	{
		if (const UTryllEdGraphNode* Node = Cast<UTryllEdGraphNode>(Obj))
		{
			if (Node->CanUserDeleteNode())
			{
				return true;
			}
		}
	}
	return false;
}

void FTryllWorkflowAssetEditor::DuplicateSelectedNodes()
{
	if (!GraphEditorWidget.IsValid() || !EdGraph || !WorkflowAsset)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("DuplicateNodes", "Duplicate Node(s)"));
	EdGraph->Modify();
	TGuardValue<bool> Guard(bUpdatingGraph, true);

	const FGraphPanelSelectionSet Selected = GraphEditorWidget->GetSelectedNodes();
	GraphEditorWidget->ClearSelectionSet();

	for (UObject* Obj : Selected)
	{
		UTryllEdGraphNode* Source = Cast<UTryllEdGraphNode>(Obj);
		if (!Source || !Source->CanDuplicateNode() || !Source->NodeParams)
		{
			continue;
		}

		UTryllNodeParamsBase* ParamsCopy = DuplicateObject<UTryllNodeParamsBase>(Source->NodeParams, WorkflowAsset);
		const FName NewName = UTryllEdGraphSchema::MakeUniqueNodeName(EdGraph, Source->NodeParams->GetClass());

		UTryllEdGraphNode* Copy = NewObject<UTryllEdGraphNode>(EdGraph, NAME_None, RF_Transactional);
		Copy->InitializeAsNode(NewName, ParamsCopy);
		Copy->CreateNewGuid();
		Copy->NodePosX = Source->NodePosX + 40;
		Copy->NodePosY = Source->NodePosY + 40;
		Copy->bNeedsAutoLayout = false;
		EdGraph->AddNode(Copy, /*bUserAction*/ true, /*bSelectNewNode*/ true);
		Copy->AllocateDefaultPins();

		GraphEditorWidget->SetNodeSelection(Copy, true);
	}

	SyncGraphToAsset();
	RunValidation();
}

bool FTryllWorkflowAssetEditor::CanDuplicateSelectedNodes() const
{
	if (!GraphEditorWidget.IsValid())
	{
		return false;
	}
	for (UObject* Obj : GraphEditorWidget->GetSelectedNodes())
	{
		if (const UTryllEdGraphNode* Node = Cast<UTryllEdGraphNode>(Obj))
		{
			if (Node->CanDuplicateNode())
			{
				return true;
			}
		}
	}
	return false;
}

void FTryllWorkflowAssetEditor::OnAutoLayout()
{
	if (!EdGraph)
	{
		return;
	}
	const FScopedTransaction Transaction(LOCTEXT("AutoLayoutTransaction", "Auto Layout"));
	EdGraph->Modify();
	{
		TGuardValue<bool> Guard(bUpdatingGraph, true);
		FTryllGraphSync::AutoLayout(EdGraph, /*bOnlyUnpositioned*/ false);
	}
	SyncGraphToAsset();
	if (GraphEditorWidget.IsValid())
	{
		GraphEditorWidget->NotifyGraphChanged();
		GraphEditorWidget->ZoomToFit(/*bOnlySelection*/ false);
	}
}

void FTryllWorkflowAssetEditor::SaveAsset_Execute()
{
	// Capture final node positions + wiring before the package is written.
	SyncGraphToAsset();
	FAssetEditorToolkit::SaveAsset_Execute();
}

#undef LOCTEXT_NAMESPACE
