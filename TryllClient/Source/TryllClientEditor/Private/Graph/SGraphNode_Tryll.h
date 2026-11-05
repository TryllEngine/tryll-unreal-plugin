// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"
#include "EdGraphUtilities.h"

class UTryllEdGraphNode;

/**
 * Slate widget for a workflow node. Uses the standard SGraphNode chrome — which already renders
 * the node's per-type title colour (UTryllEdGraphNode::GetNodeTitleColor), pins with their exit
 * friendly-names, and the error/warning badge from the node's compiler-message state — so the
 * visual layer stays thin while the generic node class drives everything.
 */
class SGraphNode_Tryll : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNode_Tryll) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UTryllEdGraphNode* InNode);
};

/** Panel factory that maps UTryllEdGraphNode to SGraphNode_Tryll. */
class FTryllGraphNodeFactory : public FGraphPanelNodeFactory
{
public:
	virtual TSharedPtr<SGraphNode> CreateNode(UEdGraphNode* Node) const override;
};
