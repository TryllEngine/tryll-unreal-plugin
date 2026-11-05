// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "Graph/TryllEdGraphNode.h"
#include "Graph/TryllNodeExitReflection.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"
#include "TryllGraphDescription.h"
#include "EdGraph/EdGraphPin.h"
#include "HAL/PlatformProcess.h"
#include "Styling/AppStyle.h"
#include "ToolMenu.h"

#define LOCTEXT_NAMESPACE "TryllEdGraphNode"

const FName TryllGraphPinCategory(TEXT("TryllExec"));
const FName TryllInputPinName(TEXT("In"));
const FName TryllStartPinName(TEXT("start"));

namespace
{
	/** Accent colour per node type — keeps the canvas readable at a glance. */
	FLinearColor ColorForNodeType(ETryllNodeType Type)
	{
		switch (Type)
		{
		case ETryllNodeType::Generate:              return FLinearColor(0.16f, 0.44f, 0.78f); // blue
		case ETryllNodeType::GenerateAndSpeak:      return FLinearColor(0.20f, 0.52f, 0.66f); // teal
		case ETryllNodeType::RegexGuardrail: return FLinearColor(0.74f, 0.22f, 0.22f); // red
		case ETryllNodeType::CannedResponse:        return FLinearColor(0.46f, 0.36f, 0.66f); // purple
		case ETryllNodeType::ToolCall:              return FLinearColor(0.78f, 0.52f, 0.16f); // amber
		case ETryllNodeType::Retrieve:              return FLinearColor(0.24f, 0.58f, 0.34f); // green
		case ETryllNodeType::Instruction:           return FLinearColor(0.40f, 0.40f, 0.44f); // slate
		case ETryllNodeType::ClassifyIntent:        return FLinearColor(0.58f, 0.46f, 0.20f); // olive
		case ETryllNodeType::ClassifyIntentLLM:     return FLinearColor(0.62f, 0.40f, 0.24f); // bronze
		case ETryllNodeType::IntentToInstruction:   return FLinearColor(0.34f, 0.50f, 0.56f); // steel
		case ETryllNodeType::Branch:                return FLinearColor(0.70f, 0.62f, 0.18f); // gold
		default:                                    return FLinearColor(0.30f, 0.30f, 0.34f);
		}
	}

	/** Human-readable type label, taken from the params UCLASS DisplayName meta. */
	FText TypeLabel(const UTryllNodeParamsBase* Params)
	{
		if (!Params)
		{
			return LOCTEXT("UnknownType", "Node");
		}
		return Params->GetClass()->GetDisplayNameText();
	}
}

void UTryllEdGraphNode::InitializeAsNode(FName InName, UTryllNodeParamsBase* InParams)
{
	NodeName = InName;
	NodeParams = InParams;
	bIsEntryNode = false;
}

void UTryllEdGraphNode::InitializeAsEntry()
{
	NodeName = NAME_None;
	NodeParams = nullptr;
	bIsEntryNode = true;
}

void UTryllEdGraphNode::AllocateDefaultPins()
{
	if (bIsEntryNode)
	{
		// Single output that drives StartNode.
		CreatePin(EGPD_Output, TryllGraphPinCategory, TryllStartPinName);
		return;
	}

	// One incoming pin (many nodes may route here).
	CreatePin(EGPD_Input, TryllGraphPinCategory, TryllInputPinName);

	// One output pin per declared exit.
	for (const FTryllNodeExit& Exit : FTryllNodeExitReflection::GetExits(NodeParams))
	{
		UEdGraphPin* Pin = CreatePin(EGPD_Output, TryllGraphPinCategory, FName(*Exit.ExitName));
		// Display-only: "no_tool_called" -> "No Tool Called". PinName keeps the wire name so
		// FindExitPin and the sync/export path still match by Exit.ExitName.
		Pin->PinFriendlyName = FText::FromString(FName::NameToDisplayString(Exit.ExitName, /*bIsBool*/ false));
	}
}

UEdGraphPin* UTryllEdGraphNode::GetInputPin() const
{
	return bIsEntryNode ? nullptr : FindPin(TryllInputPinName, EGPD_Input);
}

UEdGraphPin* UTryllEdGraphNode::GetStartPin() const
{
	return bIsEntryNode ? FindPin(TryllStartPinName, EGPD_Output) : nullptr;
}

UEdGraphPin* UTryllEdGraphNode::FindExitPin(const FString& ExitName) const
{
	return FindPin(FName(*ExitName), EGPD_Output);
}

FText UTryllEdGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (bIsEntryNode)
	{
		return LOCTEXT("EntryTitle", "Entry");
	}

	const FText Type = TypeLabel(NodeParams);
	if (NodeName.IsNone())
	{
		return Type;
	}

	// The inline rename box edits only the instance name; the menu/list label is the type.
	if (TitleType == ENodeTitleType::EditableTitle || TitleType == ENodeTitleType::MenuTitle)
	{
		return FText::FromName(NodeName);
	}
	if (TitleType == ENodeTitleType::ListView)
	{
		return Type;
	}

	// FullTitle: name on the first line, type as the faded subtitle line.
	return FText::Format(LOCTEXT("NodeTitleFmt", "{0}\n{1}"),
		FText::FromName(NodeName), Type);
}

FLinearColor UTryllEdGraphNode::GetNodeTitleColor() const
{
	if (bIsEntryNode)
	{
		return FLinearColor(0.10f, 0.10f, 0.12f);
	}
	return NodeParams ? ColorForNodeType(NodeParams->GetNodeType()) : FLinearColor::Gray;
}

FText UTryllEdGraphNode::GetTooltipText() const
{
	if (bIsEntryNode)
	{
		return LOCTEXT("EntryTooltip", "Workflow entry point. Wire it to the node that runs first.");
	}
	return FText::Format(LOCTEXT("NodeTooltipFmt", "{0} — {1}"),
		FText::FromName(NodeName), TypeLabel(NodeParams));
}

void UTryllEdGraphNode::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);

	const FString DocsUrl = NodeParams ? NodeParams->GetDocsUrl() : FString();
	if (DocsUrl.IsEmpty())
	{
		return;
	}
	FToolMenuSection& Section = Menu->AddSection(TEXT("TryllDocumentation"), LOCTEXT("DocsSection", "Documentation"));
	Section.AddMenuEntry(
		TEXT("TryllOpenDocumentation"),
		LOCTEXT("OpenDocs", "Open Documentation"),
		LOCTEXT("OpenDocsTooltip", "Open this node type's reference page in the browser."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Help"),
		FUIAction(FExecuteAction::CreateLambda([DocsUrl]
		{
			FPlatformProcess::LaunchURL(*DocsUrl, nullptr, nullptr);
		})));
}

#undef LOCTEXT_NAMESPACE
