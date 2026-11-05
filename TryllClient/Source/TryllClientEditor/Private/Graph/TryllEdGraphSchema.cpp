// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "Graph/TryllEdGraphSchema.h"
#include "Graph/TryllEdGraph.h"
#include "Graph/TryllEdGraphNode.h"

#include "TryllWorkflowAsset.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphNode_Comment.h"
#include "UObject/UObjectHash.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "TryllEdGraphSchema"

// ── FTryllSchemaAction_NewNode ────────────────────────────────────────────────

UEdGraphNode* FTryllSchemaAction_NewNode::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin,
	const FVector2D Location, bool bSelectNewNode)
{
	if (!ParentGraph || !*ParamClass)
	{
		return nullptr;
	}

	const FScopedTransaction Transaction(LOCTEXT("AddNode", "Add Tryll Node"));
	ParentGraph->Modify();

	// Outer the params object under the asset so it serialises with the workflow.
	UObject* ParamsOuter = ParentGraph;
	if (UTryllEdGraph* TryllGraph = Cast<UTryllEdGraph>(ParentGraph))
	{
		if (UTryllWorkflowAsset* Asset = TryllGraph->WorkflowAsset.Get())
		{
			ParamsOuter = Asset;
		}
	}

	UTryllNodeParamsBase* Params = NewObject<UTryllNodeParamsBase>(
		ParamsOuter, ParamClass, NAME_None, RF_Transactional);

	const FName NewName = UTryllEdGraphSchema::MakeUniqueNodeName(ParentGraph, ParamClass);

	UTryllEdGraphNode* Node = NewObject<UTryllEdGraphNode>(ParentGraph, NAME_None, RF_Transactional);
	Node->InitializeAsNode(NewName, Params);
	Node->CreateNewGuid();
	Node->NodePosX = FMath::RoundToInt(Location.X);
	Node->NodePosY = FMath::RoundToInt(Location.Y);
	Node->bNeedsAutoLayout = false;
	ParentGraph->AddNode(Node, /*bUserAction*/ true, bSelectNewNode);
	Node->AllocateDefaultPins();

	// Auto-connect when spawned by dragging off an existing pin.
	if (FromPin)
	{
		const UEdGraphSchema* Schema = ParentGraph->GetSchema();
		if (FromPin->Direction == EGPD_Output)
		{
			if (UEdGraphPin* InPin = Node->GetInputPin())
			{
				Schema->TryCreateConnection(FromPin, InPin);
			}
		}
		else // dragging from an input pin: route the new node into it
		{
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin->Direction == EGPD_Output)
				{
					Schema->TryCreateConnection(Pin, FromPin);
					break;
				}
			}
		}
	}

	ParentGraph->NotifyGraphChanged();
	return Node;
}

// ── UTryllEdGraphSchema ───────────────────────────────────────────────────────

FName UTryllEdGraphSchema::MakeUniqueNodeName(const UEdGraph* Graph, const UClass* ParamClass)
{
	FString Base = ParamClass ? ParamClass->GetDisplayNameText().ToString() : TEXT("Node");
	Base.RemoveSpacesInline();
	if (Base.IsEmpty())
	{
		Base = TEXT("Node");
	}
	// Lower-case first letter for a tidy instance name (Generate -> generate1).
	Base[0] = FChar::ToLower(Base[0]);

	TSet<FString> Existing;
	if (Graph)
	{
		for (UEdGraphNode* Raw : Graph->Nodes)
		{
			if (const UTryllEdGraphNode* Node = Cast<UTryllEdGraphNode>(Raw))
			{
				if (!Node->NodeName.IsNone())
				{
					Existing.Add(Node->NodeName.ToString());
				}
			}
		}
	}

	for (int32 Suffix = 1; ; ++Suffix)
	{
		const FString Candidate = FString::Printf(TEXT("%s%d"), *Base, Suffix);
		if (!Existing.Contains(Candidate))
		{
			return FName(*Candidate);
		}
	}
}

void UTryllEdGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	TArray<UClass*> ParamClasses;
	GetDerivedClasses(UTryllNodeParamsBase::StaticClass(), ParamClasses, /*bRecursive*/ true);

	// Stable, alphabetical menu.
	ParamClasses.Sort([](const UClass& A, const UClass& B)
	{
		return A.GetDisplayNameText().ToString() < B.GetDisplayNameText().ToString();
	});

	const FText Category = LOCTEXT("AddNodeCategory", "Add Node");
	for (UClass* ParamClass : ParamClasses)
	{
		if (!ParamClass || ParamClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			continue;
		}

		const FText Label = ParamClass->GetDisplayNameText();
		const FText Tooltip = FText::Format(LOCTEXT("AddNodeTooltipFmt", "Add a {0} node."), Label);

		TSharedPtr<FTryllSchemaAction_NewNode> Action = MakeShared<FTryllSchemaAction_NewNode>(
			Category, Label, Tooltip, ParamClass);
		ContextMenuBuilder.AddAction(Action);
	}
}

const FPinConnectionResponse UTryllEdGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	if (!A || !B)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnNull", "Invalid pin."));
	}
	if (A->GetOwningNode() == B->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnSelf", "Cannot connect a node to itself."));
	}
	if (A->Direction == B->Direction)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnSameDir", "Connect an exit to a node input."));
	}

	// Output (exit) pins are single-target: a new connection replaces the existing one.
	const bool bAIsOutput = (A->Direction == EGPD_Output);
	const UEdGraphPin* OutPin = bAIsOutput ? A : B;
	if (OutPin->LinkedTo.Num() > 0)
	{
		return FPinConnectionResponse(
			bAIsOutput ? CONNECT_RESPONSE_BREAK_OTHERS_A : CONNECT_RESPONSE_BREAK_OTHERS_B,
			LOCTEXT("ConnReplace", "Replace exit target."));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, LOCTEXT("ConnOk", "Connect exit to node."));
}

bool UTryllEdGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	// Base handles BREAK_OTHERS + making the link. Notify on a successful make so
	// FTryllWorkflowAssetEditor::OnGraphChanged runs SyncGraphToAsset() — the Break*
	// overrides above already notify, but a plain new wire otherwise wouldn't, leaving
	// the asset-side exit fields (and the slot-input dropdown / validator) stale until Save.
	const bool bModified = Super::TryCreateConnection(A, B);
	if (bModified)
	{
		if (UEdGraphNode* Node = A ? A->GetOwningNodeUnchecked() : nullptr)
		{
			if (UEdGraph* Graph = Node->GetGraph())
			{
				Graph->NotifyGraphChanged();
			}
		}
	}
	return bModified;
}

FLinearColor UTryllEdGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FLinearColor(0.85f, 0.85f, 0.9f);
}

// The three Break* overrides each open a transaction, mirroring UEdGraphSchema_K2: the base
// implementations transact nothing and neither do the engine call sites, so without this the
// NotifyGraphChanged below reaches SyncGraphToAsset with no transaction open — the asset's
// Modify() records nothing and the break cannot be undone. Nesting is safe: a transaction
// opened by an outer command (node deletion) folds these into itself.

void UTryllEdGraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	const FScopedTransaction Transaction(LOCTEXT("BreakNodeLinks", "Break Node Links"));
	Super::BreakNodeLinks(TargetNode);
	if (UEdGraph* Graph = TargetNode.GetGraph())
	{
		Graph->NotifyGraphChanged();
	}
}

void UTryllEdGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const
{
	const FScopedTransaction Transaction(LOCTEXT("BreakPinLinks", "Break Pin Links"));
	Super::BreakPinLinks(TargetPin, bSendsNodeNotification);
	if (UEdGraphNode* Node = TargetPin.GetOwningNodeUnchecked())
	{
		if (UEdGraph* Graph = Node->GetGraph())
		{
			Graph->NotifyGraphChanged();
		}
	}
}

void UTryllEdGraphSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
	const FScopedTransaction Transaction(LOCTEXT("BreakSinglePinLink", "Break Pin Link"));
	Super::BreakSinglePinLink(SourcePin, TargetPin);
	if (SourcePin)
	{
		if (UEdGraphNode* Node = SourcePin->GetOwningNodeUnchecked())
		{
			if (UEdGraph* Graph = Node->GetGraph())
			{
				Graph->NotifyGraphChanged();
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
