// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "Templates/SubclassOf.h"
#include "TryllEdGraphSchema.generated.h"

class UTryllNodeParamsBase;

/** Context-menu action that spawns a new workflow node of a given params type. */
USTRUCT()
struct FTryllSchemaAction_NewNode : public FEdGraphSchemaAction
{
	GENERATED_BODY()

	/** Concrete UTryllNodeParamsBase subclass to instantiate. */
	UPROPERTY()
	TSubclassOf<UTryllNodeParamsBase> ParamClass;

	FTryllSchemaAction_NewNode() = default;

	FTryllSchemaAction_NewNode(FText InCategory, FText InMenuDesc, FText InToolTip,
		TSubclassOf<UTryllNodeParamsBase> InParamClass)
		: FEdGraphSchemaAction(MoveTemp(InCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), /*Grouping*/ 0)
		, ParamClass(InParamClass)
	{
	}

	// FEdGraphSchemaAction
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin,
		const FVector2D Location, bool bSelectNewNode = true) override;
};

/**
 * Schema for the workflow graph. Drives node creation (one menu entry per UTryllNodeParamsBase
 * subclass, discovered by reflection) and connection rules: output exit pins are single-target,
 * input pins accept many, no self-loops, output-to-input only.
 */
UCLASS()
class UTryllEdGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()

public:
	/** Produce a graph-unique node name from a params type display label. */
	static FName MakeUniqueNodeName(const UEdGraph* Graph, const UClass* ParamClass);

	// UEdGraphSchema interface
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;
	virtual void BreakNodeLinks(UEdGraphNode& TargetNode) const override;
	virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const override;
	virtual void BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const override;
};
