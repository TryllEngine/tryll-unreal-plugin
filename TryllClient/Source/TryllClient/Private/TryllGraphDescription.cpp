// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllGraphDescription.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"

// ── FTryllGraphDescription ────────────────────────────────────────────────────

bool FTryllGraphDescription::UsesTts() const
{
	for (const FTryllNodeDescription& Node : Nodes)
	{
		if (Node.Params && Node.Params->IsTtsConsumer())
		{
			return true;
		}
	}
	return false;
}

// ── FTryllGraphOptions ────────────────────────────────────────────────────────

TArray<FString> FTryllGraphOptions::GetNodeNameOptions(const FTryllGraphDescription& Graph)
{
	TArray<FString> Names;
	for (const FTryllNodeDescription& Node : Graph.Nodes)
	{
		if (!Node.Name.IsEmpty())
			Names.Add(Node.Name);
	}
	return Names;
}

// ── FTryllGraphBuilder ────────────────────────────────────────────────────────

FTryllGraphBuilder& FTryllGraphBuilder::AddNode(FString Name, UTryllNodeParamsBase* Params)
{
	FTryllNodeDescription Node;
	Node.Name   = MoveTemp(Name);
	Node.Params = TObjectPtr<UTryllNodeParamsBase>(Params);
	Description.Nodes.Add(MoveTemp(Node));
	return *this;
}

FTryllGraphBuilder& FTryllGraphBuilder::SetStartNode(FString Name)
{
	Description.StartNode = MoveTemp(Name);
	return *this;
}

FTryllGraphBuilder& FTryllGraphBuilder::SetDefaultModelName(FString Name)
{
	Description.DefaultModelName = MoveTemp(Name);
	return *this;
}
