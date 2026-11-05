// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllDialogLabChecks.h"

enum class ETryllDiagnosticsAbsenceReason : uint8;

class FJsonObject;
class FJsonValue;

enum class ETryllLabDiffOp : uint8
{
	Same,
	Added,
	Removed,
};

struct FTryllLabDiffLine
{
	ETryllLabDiffOp Op = ETryllLabDiffOp::Same;
	FString Line;
};

enum class ETryllLabSegmentPairing : uint8
{
	Both,
	OnlyMine,
	OnlyTheirs,
};

struct FTryllLabPromptMessage
{
	FString Role;
	FString Content;
};

/**
 * One prompt-bearing node execution: its prompt messages and its output text, kept under the
 * identity of the node that produced them.
 *
 * NodeIndex — the position in debug_info.nodes[] — is the only stable identity. The name is not:
 * nodes[] is appended once per OnNodeEnd, so a node re-entered within a turn
 * (ToolCall → Generate → ToolCall) legitimately appears more than once.
 */
struct FTryllLabPromptSegment
{
	int32   NodeIndex = 0;
	FString NodeName;
	FString NodeType;
	FString ExitRoute;
	TArray<FTryllLabPromptMessage> Prompt;
	FString OutputText;

	/**
	 * Pairing key for the A/B segment match. Name *and* type, so a renamed node reads as a route
	 * change rather than silently diffing against an unrelated one.
	 *
	 * Separated by NUL, as Unity does. With a printable separator, (name "A B", type "C") and
	 * (name "A", type "B C") key to the same string, so two unrelated segments pair up and get
	 * line-diffed against each other instead of being reported one-sided.
	 */
	FString PairKey() const { return NodeName + FString::Chr(TCHAR(0)) + NodeType; }

	bool HasContent() const { return Prompt.Num() > 0 || !OutputText.IsEmpty(); }
};

/**
 * One segment of an A/B comparison: a matched pair with its line diff, or a segment that exists
 * on only one side.
 */
struct FTryllLabSegmentDiff
{
	ETryllLabSegmentPairing Pairing = ETryllLabSegmentPairing::Both;
	/** Valid unless Pairing is OnlyTheirs. */
	FTryllLabPromptSegment Mine;
	/** Valid unless Pairing is OnlyMine. */
	FTryllLabPromptSegment Theirs;
	TArray<FTryllLabDiffLine> Lines;

	const FTryllLabPromptSegment& Either() const
	{
		return Pairing == ETryllLabSegmentPairing::OnlyTheirs ? Theirs : Mine;
	}
};

/**
 * Pure text rendering over schema_version 2 diagnostics, plus the prompt line diff. None of it
 * touches Slate: DiffLines decides what the pinned-cell prompt comparison shows, and it is the
 * one piece here that is genuinely easy to get wrong.
 */
struct TRYLLCLIENTEDITOR_API FTryllLabJsonRender
{
	/**
	 * Every prompt-bearing node execution in the turn, as a segment carrying the identity of the
	 * node that produced it.
	 *
	 * Node attribution is not cosmetic. A graph with two LLM nodes (ClassifyIntentLLM → Generate,
	 * ToolCall → Generate) otherwise flattens into one role-tagged blob, which makes the two
	 * indistinguishable to a reader *and* lets the A/B line diff align one variant's classifier
	 * prompt against the other's generate prompt. The comparison is per segment — see DiffSegments.
	 *
	 * Nodes carrying neither a prompt nor output text are skipped rather than emitting an empty
	 * heading: selection is by group presence, not node type, so this covers node types nothing
	 * here knows about.
	 */
	static TArray<FTryllLabPromptSegment> PromptSegments(const TSharedPtr<FJsonObject>& DebugInfo);

	/**
	 * The heading that names a segment's producer. Carries only structural fields — index, name,
	 * type, exit — so it never introduces diff churn, while an exit that genuinely differs
	 * between variants still shows up as a change.
	 */
	static FString SegmentHeader(const FTryllLabPromptSegment& Segment);

	/** One segment's role-labelled transcript, without its heading. */
	static FString RenderSegmentBody(const FTryllLabPromptSegment& Segment);

	/**
	 * The readable / copyable transcript for a set of segments. Headings appear only when there
	 * is more than one segment — with a single producer they restate the Route block, and the
	 * single-node case is the common one.
	 */
	static FString RenderSegments(const TArray<FTryllLabPromptSegment>& Segments);

	/**
	 * Pair two turns' segments and diff each pair's transcript on its own.
	 *
	 * Pairing runs an LCS over PairKey, so a route that gained or lost a node reports that node
	 * as present on one side only — the interesting finding — instead of sliding every later
	 * segment out of alignment and diffing unrelated prompts against each other. Segment counts
	 * are single digits, so the table is free.
	 */
	static TArray<FTryllLabSegmentDiff> DiffSegments(
		const TArray<FTryllLabPromptSegment>& Mine,
		const TArray<FTryllLabPromptSegment>& Theirs);

	/**
	 * Reconstructs a role-labelled transcript from every node's diagnostics.input.prompt[] plus
	 * diagnostics.output.text, in node order — the A/B diff target for live cells.
	 */
	static FString RenderedPromptFromJson(const TSharedPtr<FJsonObject>& DebugInfo);

	/** Why a turn carries no usable diagnostics — explicit so chat and Lab explain absence. */
	static FString NoDiagnosticsNote(ETryllDiagnosticsAbsenceReason Reason);

	/** Lab convenience: map a cell to the shared note. */
	static FString NoDiagnosticsNote(const struct FTryllLabCellView& Cell);


	static FString JsonScalarToString(const TSharedPtr<FJsonValue>& Value);

	static FString PrettyPrintJson(const TSharedPtr<FJsonValue>& Value, int32 Indent);
	static FString PrettyPrintJson(const TSharedPtr<FJsonObject>& Object, int32 Indent);

	/** Pretty-prints when the text parses as JSON, and returns it untouched when it does not. */
	static FString PrettyJsonOrRaw(const FString& Json);

	/**
	 * Line diff producing **both** sides: lines only in Mine (Added) and lines only in Theirs
	 * (Removed), interleaved in original order.
	 *
	 * A set-membership test is not enough — it can only mark lines that *are* present, so
	 * removals are invisible by construction. This walks an LCS table instead, which is what
	 * makes "prompt B dropped this instruction" visible at all, usually the more interesting
	 * half of a comparison. Matching is on the trimmed line so indentation is not churn.
	 */
	static TArray<FTryllLabDiffLine> DiffLines(const FString& Mine, const FString& Theirs);

	/** Above this many lines on either side, degrade rather than allocate an O(n*m) table. */
	static constexpr int32 MaxDiffLines = 800;
};
