// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllGraphDescription.h"
#include "TryllVariableDecl.h"

/**
 * Conversion between the agent-facing workflow JSON and the engine-side
 * FTryllGraphDescription + Variables pair.
 *
 * The JSON is deliberately not a mirror of the UPROPERTY layout. Three
 * differences exist because the reflected shape is hostile to a language model:
 *
 *   1. Node params are addressed by node *type* ("Generate"), not by the
 *      instanced UObject class the type happens to map to.
 *   2. Single-FString wrapper structs (FTryllModelName, FTryllSlotName,
 *      FTryllSlotInput) collapse to plain strings.
 *   3. The codegen's bOverrideX/X optional pairs collapse to presence: writing
 *      a field turns its toggle on, omitting it leaves the field inherited.
 *      Without this, a supplied value silently does nothing — the same class of
 *      failure FTryllNodeFieldReflection guards against for Dialog Lab.
 *
 * Shape:
 * {
 *   "startNode": "gen",
 *   "defaultModelName": "qwen2.5-0.5b-instruct",
 *   "variables": [ { "name": "price", "type": "Int",
 *                    "allowOutputSubstitution": true, "value": 0 } ],
 *   "nodes": [ { "name": "gen", "type": "Generate",
 *                "params": { "systemPrompt": "...", "defaultExit": "guard" } } ]
 * }
 */
namespace TryllWorkflowGraphJson
{
	struct FParseResult
	{
		FTryllGraphDescription     Graph;
		TArray<FTryllVariableDecl> Variables;

		/** Structural problems — malformed JSON, unknown node type, bad variable type. */
		TArray<FString> Errors;

		[[nodiscard]] bool IsValid() const { return Errors.IsEmpty(); }
	};

	/**
	 * @param ParamsOuter Outer for the created params UObjects. Must be the owning
	 *   workflow asset: UTryllNodeParamsBase::FindEnclosingGraph walks the Outer
	 *   chain, so params outered anywhere else lose their exit dropdowns.
	 */
	[[nodiscard]] FParseResult Parse(const FString& Json, UObject* ParamsOuter);

	[[nodiscard]] FString Serialize(
		const FTryllGraphDescription& Graph, const TArray<FTryllVariableDecl>& Variables);

	/** @param NodeTypeFilter Empty for every node type, otherwise a single type name. */
	[[nodiscard]] FString DescribeNodeTypes(const FString& NodeTypeFilter);

	/** Node type to its generated params class, resolved via each CDO's GetNodeType(). */
	[[nodiscard]] TMap<ETryllNodeType, UClass*> BuildNodeParamsClassMap();
}
