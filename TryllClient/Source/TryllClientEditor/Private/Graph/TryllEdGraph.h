// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "TryllEdGraph.generated.h"

class UTryllWorkflowAsset;

/**
 * Transient editor graph backing one UTryllWorkflowAsset.
 *
 * The graph is never serialized into the asset — FTryllGraphDescription remains the single
 * source of truth. The graph is rebuilt from the description on open and exported back on every
 * edit (see FTryllGraphSync). It only holds a weak pointer to the owning asset so the schema can
 * outer newly-created params objects under it.
 */
UCLASS()
class UTryllEdGraph : public UEdGraph
{
	GENERATED_BODY()

public:
	/** The asset this graph mirrors. New node-params objects are outered here so they persist. */
	UPROPERTY()
	TWeakObjectPtr<UTryllWorkflowAsset> WorkflowAsset;
};
