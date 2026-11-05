// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

struct FTryllNodeDescription;
struct FTryllGraphDescription;
class UTryllNodeParamsBase;
class UTryllClassifyAttributesLLMParams;

/**
 * Shared grammar/uniquify rules and reference-rewriting for the rename-commit widgets
 * (full scope: node Name and output_name both get commit-point widgets that propagate).
 *
 * Callers (the property customizations and the workflow graph editor) own the actual commit
 * point: read the old value, run ResolveNodeRename to validate/uniquify the new one, write it
 * back, then call the propagation helpers below — see FTryllNodeDescriptionCustomization,
 * FTryllSlotNameCustomization and FTryllWorkflowAssetEditor::CommitNodeRename.
 */
// Exported (not just visible within TryllClientEditor) so TryllClientTests'
// automation spec can link against these across the module DLL boundary.
namespace TryllRenamePropagation
{
	extern TRYLLCLIENTEDITOR_API const FString ReservedUserMessage;

	[[nodiscard]] TRYLLCLIENTEDITOR_API bool IsValidGrammar(const FString& Name);
	[[nodiscard]] TRYLLCLIENTEDITOR_API bool IsReserved(const FString& Name);

	/**
	 * Returns BaseName unchanged if not already present in ExistingNames; otherwise appends
	 * the smallest integer suffix (starting at 2) that makes it unique.
	 */
	[[nodiscard]] TRYLLCLIENTEDITOR_API FString Uniquify(const FString& BaseName, const TSet<FString>& ExistingNames);

	/** Result of validating + uniquifying a requested node name. */
	struct FNodeRenameOutcome
	{
		/** True when Name holds a committable node name. */
		bool bValid = false;

		/** Trimmed, uniquified name to commit. Only meaningful when bValid. */
		FString Name;

		/** User-facing reason the request was rejected. Only set when !bValid. */
		FText Error;
	};

	/**
	 * The single decision point for "may this node be renamed to this, and to what exactly":
	 * trims the request, rejects empty / bad-grammar / reserved names with a displayable
	 * reason, then uniquifies against ExistingNames. Callers pass the names of every *other*
	 * node in the graph (exclude the one being renamed, or it will uniquify against itself).
	 */
	[[nodiscard]] TRYLLCLIENTEDITOR_API FNodeRenameOutcome ResolveNodeRename(
		const FString& RequestedName, const TSet<FString>& ExistingNames);

	/**
	 * True when Node's effective output slot equals its own name — i.e. its
	 * FTryllSlotName-typed override property (output_name) is empty (the default:
	 * no output_name override, so the slot name falls back to the node's own name).
	 * Only such nodes need PropagateSlotRename when their node Name is renamed.
	 */
	[[nodiscard]] TRYLLCLIENTEDITOR_API bool EffectiveSlotIsNodeName(const FTryllNodeDescription& Node);

	/** As above, for callers holding the params object directly (the graph editor's EdGraph nodes). */
	[[nodiscard]] TRYLLCLIENTEDITOR_API bool EffectiveSlotIsNodeName(const UTryllNodeParamsBase* Params);

	/**
	 * Rewrites every exit field and StartNode equal to OldName to NewName. Does not touch
	 * input/template slot references — call PropagateSlotRename too when
	 * EffectiveSlotIsNodeName was true for the renamed node (checked against its state
	 * *before* the rename).
	 */
	TRYLLCLIENTEDITOR_API void PropagateNodeRename(FTryllGraphDescription& Graph, const FString& OldName, const FString& NewName);

	/**
	 * Rewrites every `input`-style selector (FTryllSlotInput) equal to OldSlot, every
	 * `{{slot.<OldSlot>}}` template tag, and every `{"slot": "<OldSlot>"}` filter-JSON
	 * operand (Branch's Condition) to reference NewSlot instead.
	 */
	TRYLLCLIENTEDITOR_API void PropagateSlotRename(FTryllGraphDescription& Graph, const FString& OldSlot, const FString& NewSlot);

	/**
	 * Rewrites the slots a node actually writes when its prefix changes.
	 * ClassifyAttributesLLM writes prefix_attribute, not the bare prefix.
	 */
	TRYLLCLIENTEDITOR_API void PropagateOwnedSlotRename(
		FTryllGraphDescription& Graph,
		const UTryllNodeParamsBase* Params,
		const FString& OldPrefix,
		const FString& NewPrefix);

	/**
	 * Rewrites references when one attribute's name changes. The slot is
	 * prefix_attribute, where prefix is OutputName or the owning node name.
	 */
	TRYLLCLIENTEDITOR_API void PropagateAttributeRename(
		FTryllGraphDescription& Graph,
		const UTryllClassifyAttributesLLMParams* Params,
		const FString& OwnerNodeName,
		const FString& OldAttributeName,
		const FString& NewAttributeName);
}
