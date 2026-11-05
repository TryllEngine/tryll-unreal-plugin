// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TryllGraphDescription.h"
#include "TryllVariableDecl.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#include "UObject/ObjectSaveContext.h"
#endif
#include "TryllWorkflowAsset.generated.h"

class UStringTable;

/**
 * Data asset that stores a Tryll workflow graph description.
 * Equivalent to a Unity ScriptableObject holding node and wire lists.
 *
 * Usage:
 *   1. In the Content Browser: right-click → Tryll → Workflow Asset.
 *   2. Edit nodes and routes in the Details panel.
 *   3. Assign this asset to a UTryllAgentComponent's WorkflowAsset property.
 *
 * Programmatic alternative: populate FTryllGraphDescription directly in C++ using
 * FTryllGraphBuilder and pass it to UTryllSubsystem::RequestCreateAgent.
 *
 * Editor / cook: IsDataValid + PreSave(IsCooking) fail when the graph references a
 * model that is not marked Production in the Tryll Model Manifest (so packaging
 * cannot silently omit it).
 */
UCLASS(BlueprintType)
class TRYLLCLIENT_API UTryllWorkflowAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Workflow graph stored by this asset. Assign this asset to UTryllAgentComponent::WorkflowAsset to use it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tryll|Workflow",
		meta = (ToolTip = "Workflow graph stored by this asset. Edit nodes, routes, start node and default model here."))
	FTryllGraphDescription Graph;

	/**
	 * String table asset for every Text ID in this workflow. Picking it sets TextTable
	 * and gives the cooker a reference, so the table ships with the workflow.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tryll|Workflow",
		meta = (ToolTip = "String table asset with the Text IDs of this workflow. Sets Text Table and makes sure the table is cooked."))
	TSoftObjectPtr<UStringTable> TextTableAsset;

	/**
	 * Table name passed to the text provider with every Text ID in this workflow.
	 * Each Localized field stores only the key. Unreal string-table ids are asset paths;
	 * a CSV-registered table or a custom provider's table is typed here directly.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tryll|Workflow",
		meta = (GetOptions = "GetTextTableOptions",
			ToolTip = "Table name passed to the text provider with every Text ID in this workflow. Set from Text Table Asset when one is picked."))
	FName TextTable;

	UFUNCTION()
	TArray<FString> GetTextTableOptions() const;

	/**
	 * Declared per-agent Variables for agents created from this asset — the complete
	 * declaration (name, type, initial value) sent once in CreateAgentRequest.
	 * Consumable from Mustache templates as {{var.<name>}} and the Retrieve/
	 * ClassifyIntent filter grammar as {"var": "<name>"}. A UTryllAgentComponent may
	 * override initial values (not name/type) via its own VariableOverrides
	 * (Material-Instance pattern).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tryll|Variables")
	TArray<FTryllVariableDecl> Variables;

	virtual void PostLoad() override;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("TryllWorkflow"), GetFName());
	}

	/** Options provider for FTryllGraphDescription::StartNode dropdown in the Details panel. */
	UFUNCTION()
	TArray<FString> GetStartNodeOptions() const
	{
		return FTryllGraphOptions::GetNodeNameOptions(Graph);
	}

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void SyncLocalizedTextFields();
};
