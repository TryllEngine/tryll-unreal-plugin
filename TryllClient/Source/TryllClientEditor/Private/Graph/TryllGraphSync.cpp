// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "Graph/TryllGraphSync.h"
#include "Graph/TryllEdGraphNode.h"
#include "Graph/TryllNodeExitReflection.h"

#include "TryllWorkflowAsset.h"
#include "TryllGraphDescription.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Containers/Queue.h"

namespace
{
	constexpr int32 AutoLayoutColumnWidth = 360;
	constexpr int32 AutoLayoutRowHeight   = 170;

	UTryllEdGraphNode* FindStartTarget(UTryllEdGraphNode* Entry)
	{
		if (!Entry)
		{
			return nullptr;
		}
		UEdGraphPin* StartPin = Entry->GetStartPin();
		if (StartPin && StartPin->LinkedTo.Num() > 0)
		{
			return Cast<UTryllEdGraphNode>(StartPin->LinkedTo[0]->GetOwningNode());
		}
		return nullptr;
	}
}

void FTryllGraphSync::ImportDescriptionToGraph(UTryllWorkflowAsset* Asset, UEdGraph* Graph)
{
	if (!Asset || !Graph)
	{
		return;
	}

	const FTryllGraphDescription& Desc = Asset->Graph;

	Graph->Nodes.Empty();

	// Synthetic Entry node.
	UTryllEdGraphNode* Entry = NewObject<UTryllEdGraphNode>(Graph);
	Entry->InitializeAsEntry();
	Entry->CreateNewGuid();
	Entry->AllocateDefaultPins();
	Entry->bNeedsAutoLayout = true;
	Graph->AddNode(Entry, /*bUserAction*/ false, /*bSelectNewNode*/ false);

	// Regular nodes — reuse the asset-owned params object (no copy).
	TMap<FName, UTryllEdGraphNode*> ByName;
	for (const FTryllNodeDescription& NodeDesc : Desc.Nodes)
	{
		UTryllEdGraphNode* Node = NewObject<UTryllEdGraphNode>(Graph);
		Node->InitializeAsNode(FName(*NodeDesc.Name), NodeDesc.Params);
		Node->CreateNewGuid();
		Node->AllocateDefaultPins();

#if WITH_EDITORONLY_DATA
		if (NodeDesc.bHasEditorPos)
		{
			Node->NodePosX = FMath::RoundToInt(NodeDesc.EditorPos.X);
			Node->NodePosY = FMath::RoundToInt(NodeDesc.EditorPos.Y);
			Node->bNeedsAutoLayout = false;
		}
		else
#endif
		{
			Node->bNeedsAutoLayout = true;
		}

		Graph->AddNode(Node, false, false);
		if (!Node->NodeName.IsNone())
		{
			ByName.Add(Node->NodeName, Node);
		}
	}

	// Wire exits from the stored target strings.
	for (const FTryllNodeDescription& NodeDesc : Desc.Nodes)
	{
		UTryllEdGraphNode** NodePtr = ByName.Find(FName(*NodeDesc.Name));
		if (!NodePtr)
		{
			continue;
		}
		UTryllEdGraphNode* Node = *NodePtr;

		for (const FTryllNodeExit& Exit : FTryllNodeExitReflection::GetExits(Node->NodeParams))
		{
			const FString Target = FTryllNodeExitReflection::ReadTarget(Node->NodeParams, Exit);
			if (Target.IsEmpty())
			{
				continue; // END
			}
			UTryllEdGraphNode** TargetPtr = ByName.Find(FName(*Target));
			if (!TargetPtr)
			{
				continue; // dangling — leave unconnected; the validator surfaces it
			}
			UEdGraphPin* OutPin = Node->FindExitPin(Exit.ExitName);
			UEdGraphPin* InPin  = (*TargetPtr)->GetInputPin();
			if (OutPin && InPin)
			{
				OutPin->MakeLinkTo(InPin);
			}
		}
	}

	// Entry -> StartNode.
	if (!Desc.StartNode.IsEmpty())
	{
		if (UTryllEdGraphNode** StartPtr = ByName.Find(FName(*Desc.StartNode)))
		{
			UEdGraphPin* StartPin = Entry->GetStartPin();
			UEdGraphPin* InPin    = (*StartPtr)->GetInputPin();
			if (StartPin && InPin)
			{
				StartPin->MakeLinkTo(InPin);
			}
		}
	}

	AutoLayout(Graph, /*bOnlyUnpositioned*/ true);
}

void FTryllGraphSync::ExportGraphToDescription(const UEdGraph* Graph, FTryllGraphDescription& OutDesc)
{
	if (!Graph)
	{
		return;
	}

	OutDesc.Nodes.Reset();
	OutDesc.StartNode.Empty();
	// OutDesc.DefaultModelName is deliberately left untouched.

	TArray<UTryllEdGraphNode*> RegularNodes;
	UTryllEdGraphNode* Entry = nullptr;
	for (UEdGraphNode* Raw : Graph->Nodes)
	{
		UTryllEdGraphNode* Node = Cast<UTryllEdGraphNode>(Raw);
		if (!Node)
		{
			continue;
		}
		if (Node->bIsEntryNode)
		{
			Entry = Node;
		}
		else
		{
			RegularNodes.Add(Node);
		}
	}

	// Rebuild the node list (reusing params pointers + persisting positions).
	for (UTryllEdGraphNode* Node : RegularNodes)
	{
		FTryllNodeDescription NodeDesc;
		NodeDesc.Name   = Node->NodeName.ToString();
		NodeDesc.Params = Node->NodeParams;
#if WITH_EDITORONLY_DATA
		NodeDesc.EditorPos    = FVector2D(Node->NodePosX, Node->NodePosY);
		NodeDesc.bHasEditorPos = true;
#endif
		OutDesc.Nodes.Add(NodeDesc);
	}

	// Derive every exit target from the live pin connections.
	for (UTryllEdGraphNode* Node : RegularNodes)
	{
		if (!Node->NodeParams)
		{
			continue;
		}
		for (const FTryllNodeExit& Exit : FTryllNodeExitReflection::GetExits(Node->NodeParams))
		{
			FString Target;
			if (UEdGraphPin* OutPin = Node->FindExitPin(Exit.ExitName))
			{
				if (OutPin->LinkedTo.Num() > 0)
				{
					if (UTryllEdGraphNode* TargetNode = Cast<UTryllEdGraphNode>(OutPin->LinkedTo[0]->GetOwningNode()))
					{
						Target = TargetNode->NodeName.ToString();
					}
				}
			}
			FTryllNodeExitReflection::WriteTarget(Node->NodeParams, Exit, Target);
		}
	}

	// StartNode from the Entry connection.
	if (UTryllEdGraphNode* StartNode = FindStartTarget(Entry))
	{
		OutDesc.StartNode = StartNode->NodeName.ToString();
	}
}

void FTryllGraphSync::AutoLayout(UEdGraph* Graph, bool bOnlyUnpositioned)
{
	if (!Graph)
	{
		return;
	}

	UTryllEdGraphNode* Entry = nullptr;
	for (UEdGraphNode* Raw : Graph->Nodes)
	{
		if (UTryllEdGraphNode* Node = Cast<UTryllEdGraphNode>(Raw))
		{
			if (Node->bIsEntryNode)
			{
				Entry = Node;
				break;
			}
		}
	}

	// BFS depth from the start node along output connections.
	UTryllEdGraphNode* StartNode = FindStartTarget(Entry);
	TMap<UTryllEdGraphNode*, int32> Depth;
	if (StartNode)
	{
		TQueue<UTryllEdGraphNode*> Queue;
		Depth.Add(StartNode, 0);
		Queue.Enqueue(StartNode);

		UTryllEdGraphNode* Cur = nullptr;
		while (Queue.Dequeue(Cur))
		{
			const int32 D = Depth[Cur];
			for (UEdGraphPin* Pin : Cur->Pins)
			{
				if (Pin->Direction != EGPD_Output)
				{
					continue;
				}
				for (UEdGraphPin* Linked : Pin->LinkedTo)
				{
					UTryllEdGraphNode* Next = Cast<UTryllEdGraphNode>(Linked->GetOwningNode());
					if (!Next || Next->bIsEntryNode || Depth.Contains(Next))
					{
						continue;
					}
					Depth.Add(Next, D + 1);
					Queue.Enqueue(Next);
				}
			}
		}
	}

	int32 MaxDepth = 0;
	for (const TPair<UTryllEdGraphNode*, int32>& Pair : Depth)
	{
		MaxDepth = FMath::Max(MaxDepth, Pair.Value);
	}
	const int32 OrphanColumn = StartNode ? MaxDepth + 1 : 0;

	// Bucket nodes by column.
	TMap<int32, TArray<UTryllEdGraphNode*>> Columns;
	for (UEdGraphNode* Raw : Graph->Nodes)
	{
		UTryllEdGraphNode* Node = Cast<UTryllEdGraphNode>(Raw);
		if (!Node || Node->bIsEntryNode)
		{
			continue;
		}
		const int32 Col = Depth.Contains(Node) ? Depth[Node] : OrphanColumn;
		Columns.FindOrAdd(Col).Add(Node);
	}

	for (TPair<int32, TArray<UTryllEdGraphNode*>>& Pair : Columns)
	{
		const int32 Col = Pair.Key;
		TArray<UTryllEdGraphNode*>& ColNodes = Pair.Value;
		for (int32 Row = 0; Row < ColNodes.Num(); ++Row)
		{
			UTryllEdGraphNode* Node = ColNodes[Row];
			if (bOnlyUnpositioned && !Node->bNeedsAutoLayout)
			{
				continue;
			}
			Node->NodePosX = (Col + 1) * AutoLayoutColumnWidth; // column 0 reserved for the Entry node
			Node->NodePosY = Row * AutoLayoutRowHeight;
			Node->bNeedsAutoLayout = false;
		}
	}

	if (Entry)
	{
		Entry->NodePosX = 0;
		Entry->NodePosY = StartNode ? StartNode->NodePosY : 0;
		Entry->bNeedsAutoLayout = false;
	}
}
