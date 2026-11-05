// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "Graph/SGraphNode_Tryll.h"
#include "Graph/TryllEdGraphNode.h"

void SGraphNode_Tryll::Construct(const FArguments& InArgs, UTryllEdGraphNode* InNode)
{
	GraphNode = InNode;
	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

TSharedPtr<SGraphNode> FTryllGraphNodeFactory::CreateNode(UEdGraphNode* Node) const
{
	if (UTryllEdGraphNode* TryllNode = Cast<UTryllEdGraphNode>(Node))
	{
		return SNew(SGraphNode_Tryll, TryllNode);
	}
	return nullptr;
}
