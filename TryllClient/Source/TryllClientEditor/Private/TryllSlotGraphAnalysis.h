// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

struct FTryllNodeDescription;
struct FTryllGraphDescription;

/**
 * Client-side, may-side-only slot reachability over a FTryllGraphDescription (the
 * editors need only the "may" side — "producers reachable-before this node on some
 * path" — not the server's full must/may fixed-point).
 *
 * Discovers producer/consumer fields generically via UProperty reflection over
 * FTryllSlotName / FTryllSlotInput struct-typed properties and the
 * `meta=(GetOptions="GetExitTargetOptions")` exit fields — no per-node-type Cast<>
 * dispatch, so new node types need no changes here.
 *
 * Used by FTryllSlotInputCustomization (the `input` dropdown) and by the
 * rename-propagation customizations (FTryllNodeDescriptionCustomization,
 * FTryllSlotNameCustomization).
 */
/**
 * Availability classification for one producer slot relative to a target node —
 * drives the verbose `input` dropdown: every producer slot in the graph is
 * listed, available ones enabled and unavailable ones disabled-with-a-reason,
 * rather than silently omitted.
 */
enum class ETryllSlotAvailability : uint8
{
	/** Every path from StartNode to the target writes this slot first. */
	Available,
	/** Some, not all, paths from StartNode to the target write this slot first (may-available). */
	BranchDependent,
	/** The producing node does not run before the target node (not an ancestor). */
	NotUpstream,
	/** The producing node is an ancestor, but Graph.StartNode is empty. */
	StartUnset,
	/** The producing node is an ancestor, but is not reachable from the (non-empty) StartNode. */
	StartUnreachable,
	/** The slot is the target node's own output slot. */
	Self,
	/** The current field value does not match any producer slot in the graph. */
	Missing,
};

/** One dropdown entry: a candidate slot value plus its availability and human-readable reasons. */
struct TRYLLCLIENTEDITOR_API FTryllSlotOption
{
	FString                 Value;
	ETryllSlotAvailability  Availability = ETryllSlotAvailability::Available;

	bool IsEnabled() const
	{
		return Availability == ETryllSlotAvailability::Available ||
		       Availability == ETryllSlotAvailability::BranchDependent;
	}

	/** Short note appended inline to the entry label, e.g. "(branch-dependent)". Empty when none. */
	FString ShortReason() const;

	/** Longer explanation surfaced as a tooltip (Unreal only). */
	FString LongReason(const FString& TargetNodeName) const;
};

namespace TryllSlotGraphAnalysis
{
	/**
	 * The effective slot name a node writes (its "output_name" override, or the
	 * node's own name when empty), or the empty string when the node's Params
	 * class has no FTryllSlotName-typed property (i.e. it produces no slot at all).
	 */
	// Exported (not just visible within TryllClientEditor) so TryllClientTests'
	// automation spec can link against these across the module DLL boundary.
	[[nodiscard]] TRYLLCLIENTEDITOR_API FString GetEffectiveWriteSlot(const FTryllNodeDescription& Node);

	/**
	 * Producers reachable-before TargetNodeName on some path from Graph.StartNode
	 * (may-side reachability) — the effective slot name of every such producer,
	 * plus the implicit "user_message" turn-input slot. De-duplicated;
	 * "user_message" first, then authoring order.
	 *
	 * Equivalent to filtering ClassifySlots() down to enabled entries; kept as a
	 * separate entry point for callers (the validator backstop) that only need the
	 * allow-list, not the full classification.
	 */
	[[nodiscard]] TRYLLCLIENTEDITOR_API TArray<FString> GetUpstreamSlots(const FTryllGraphDescription& Graph, const FString& TargetNodeName);

	/**
	 * Classifies every distinct producer slot in the graph relative to TargetNodeName
	 * (the verbose `input` dropdown) — one entry per effective slot name, de-duplicated by
	 * keeping the most-available classification when multiple nodes write the same
	 * slot. Does not include the implicit "user_message" default entry (callers add
	 * that themselves, since its label is context-dependent).
	 *
	 * When CurrentValue is non-empty and does not match any producer slot in the
	 * graph, a trailing ETryllSlotAvailability::Missing entry for CurrentValue is
	 * appended so it can still be shown (disabled, tinted) in the dropdown.
	 */
	[[nodiscard]] TRYLLCLIENTEDITOR_API TArray<FTryllSlotOption> ClassifySlots(
		const FTryllGraphDescription& Graph, const FString& TargetNodeName, const FString& CurrentValue = FString());

	/**
	 * True when SlotName is guaranteed to have been written before TargetNodeName executes
	 * on every path from Graph.StartNode — i.e. every such path visits at least one producer
	 * of SlotName. Implemented as a cut check: remove every producer node of SlotName from
	 * the graph (as a vertex, blocking any path through it) and see whether TargetNodeName
	 * is still reachable from StartNode — if so, some path avoids every producer, so the
	 * slot is only "may"-available (branch-dependent), not "must"-available. "user_message"
	 * is always must-available (turn input, not a node output). Mirrors the Unity port.
	 */
	[[nodiscard]] TRYLLCLIENTEDITOR_API bool IsMustAvailable(
		const FTryllGraphDescription& Graph, const FString& TargetNodeName, const FString& SlotName);

	/**
	 * True when Node's write is conditional — it does not happen on every execution, so a
	 * downstream reader of its slot must be treated as branch-dependent (may-only), never
	 * guaranteed, even when this is the graph's only producer of that slot. Mirrors the
	 * server's writeIsConditional flag (INode::GetSlotMetadata/NodeSlotMetadata) for the two
	 * node types that set it: IntentToInstruction (writes only on a storage hit) and ToolCall
	 * in CallOrAnswer mode (writes only when the model answers instead of calling a tool).
	 * Hardcoded per-type (like the server) rather than reflection-driven, since there is no
	 * schema attribute expressing "conditional on this specific enum value".
	 */
	[[nodiscard]] TRYLLCLIENTEDITOR_API bool IsWriteConditional(const FTryllNodeDescription& Node);
}
