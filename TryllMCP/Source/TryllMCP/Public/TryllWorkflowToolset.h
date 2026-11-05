// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "ToolsetRegistry/ToolsetDefinition.h"

#include "TryllWorkflowToolset.generated.h"

/** One problem found by workflow validation. */
USTRUCT(BlueprintType)
struct FTryllWorkflowIssueInfo
{
	GENERATED_BODY()

	/** "Error" blocks the write; "Warning" does not. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|MCP")
	FString Severity;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|MCP")
	FString Message;
};

/** Outcome of a successful WriteWorkflow. */
USTRUCT(BlueprintType)
struct FTryllWorkflowWriteResult
{
	GENERATED_BODY()

	/** Object path of the asset written. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|MCP")
	FString AssetPath;

	/** True when the asset did not exist and was created by this call. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|MCP")
	bool bCreated = false;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|MCP")
	int32 NodeCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|MCP")
	int32 VariableCount = 0;

	/** Non-blocking problems — orphan nodes, duplicate slot names, and similar. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|MCP")
	TArray<FTryllWorkflowIssueInfo> Warnings;
};

/** Outcome of ValidateWorkflow. Unlike WriteWorkflow, errors are reported rather than raised. */
USTRUCT(BlueprintType)
struct FTryllWorkflowValidationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|MCP")
	bool bValid = false;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|MCP")
	TArray<FTryllWorkflowIssueInfo> Issues;
};

/**
 * Authoring tools for Tryll workflow assets.
 *
 * Every write replaces the whole graph. There is deliberately no per-node edit
 * call: the generic reflection path fails on partial edits to the node array
 * ("elements changed alongside the size change"), and assembling a graph from
 * many small calls is what makes that failure reachable at all.
 *
 * Typical workflow:
 *   1. DescribeNodeTypes() to learn the node types and their parameters.
 *   2. ValidateWorkflow() to check a draft without touching the project.
 *   3. WriteWorkflow() to create or replace the asset.
 *   4. ReadWorkflow() to read one back in the same shape WriteWorkflow accepts.
 */
UCLASS(BlueprintType, MinimalAPI)
class UTryllWorkflowToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * Read a workflow asset as graph JSON, in the same shape WriteWorkflow accepts,
	 * so a graph can be read, edited and written back whole.
	 * Inherited parameters that the node does not override are omitted.
	 * @param AssetPath Package or object path, e.g. "/Game/Merchant/WF_Trader".
	 * @return Graph JSON: startNode, defaultModelName, variables and nodes.
	 */
	UFUNCTION(meta = (AICallable), Category = "TryllWorkflowToolset")
	static TRYLLMCP_API FString ReadWorkflow(const FString& AssetPath);

	/**
	 * Create or replace a workflow asset from graph JSON, then save it.
	 * The whole graph is replaced: nodes absent from the JSON are removed.
	 * Validation runs before anything is committed — when it reports an error the
	 * asset is left exactly as it was and the call fails with every problem listed.
	 * @param AssetPath Package or object path, e.g. "/Game/Merchant/WF_Trader".
	 *   Missing packages are created. Paths are case-sensitive.
	 * @param GraphJson Graph JSON. Setting a parameter marks it as overridden;
	 *   omitting one leaves it inherited.
	 * @return What was written, plus any non-blocking warnings.
	 */
	UFUNCTION(meta = (AICallable), Category = "TryllWorkflowToolset")
	static TRYLLMCP_API FTryllWorkflowWriteResult WriteWorkflow(
		const FString& AssetPath, const FString& GraphJson);

	/**
	 * Check graph JSON without creating, modifying or saving anything.
	 * Reports every problem found rather than stopping at the first.
	 * @param GraphJson Graph JSON, same shape as WriteWorkflow.
	 * @return Whether the graph is free of errors, and the full issue list.
	 */
	UFUNCTION(meta = (AICallable), Category = "TryllWorkflowToolset")
	static TRYLLMCP_API FTryllWorkflowValidationResult ValidateWorkflow(const FString& GraphJson);

	/**
	 * Describe the node types available in this build and the parameters each accepts,
	 * including value ranges, enum options, and which parameters are graph exits.
	 * Prefer this over guessing parameter names.
	 * @param NodeType Optional single type to describe, e.g. "Generate". Empty for all.
	 * @return JSON: {"nodeTypes": [{"type", "description", "params": {...}}]}. A param
	 *   marked "optional" is inherited from the model catalog when omitted, and pinned
	 *   to the value you send when present.
	 */
	UFUNCTION(meta = (AICallable), Category = "TryllWorkflowToolset")
	static TRYLLMCP_API FString DescribeNodeTypes(const FString& NodeType);
};
