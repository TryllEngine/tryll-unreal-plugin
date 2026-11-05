// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllError.h"

#include "Misc/Optional.h"
#include "Misc/TVariant.h"

enum class ETryllDialogCompatibility : uint8
{
	Strict = 0,
	AllowGraphMismatch = 1,
};

enum class ETryllDialogSlotKind : uint8
{
	User = 0,
	Instruction = 1,
	General = 2,
};

struct TRYLLCLIENT_API FTryllDialogProjectionWindow
{
	FString NodeName;
	uint32  WindowStart = 0;
};

struct TRYLLCLIENT_API FTryllDialogSlot
{
	FString Name;
	FString Text;
	ETryllDialogSlotKind Kind = ETryllDialogSlotKind::General;
	uint8 HistoryRole = 0; // HistoryRole_None
	FString Producer;
};

struct TRYLLCLIENT_API FTryllDialogKnowledgeChunk
{
	FString Id;
	FString Text;
	float Distance = 0.0f;
	TOptional<float> Bm25Score;
	TOptional<uint32> RankDense;
	TOptional<uint32> RankSparse;
};

struct TRYLLCLIENT_API FTryllDialogKnowledge
{
	FString Source;
	TArray<FTryllDialogKnowledgeChunk> Chunks;
};

struct TRYLLCLIENT_API FTryllDialogIntention
{
	FString Intent;
};

struct TRYLLCLIENT_API FTryllDialogToolCallSnapshot
{
	FString ToolName;
	TArray<TPair<FString, FString>> Arguments;
	FString ArgumentsJson;
	bool bDispatched = false;
	FString Id;
	uint8 Disposition = 0; // ToolCallDisposition_Acknowledge
	FString ModelId;
	uint64 ExchangeId = 0;
};

struct TRYLLCLIENT_API FTryllDialogToolResult
{
	FString CallId;
	FString ToolName;
	FString Text;
	uint64 ExchangeId = 0;
};

/**
 * Rolling conversation summary written by a Summarize node.
 *
 * CoveredThrough is the index of the last interaction folded into Text,
 * inclusive. The server rejects an import whose value is at or past the
 * snapshot's interaction count.
 */
struct TRYLLCLIENT_API FTryllDialogSummary
{
	FString Text;
	uint32 CoveredThrough = 0;
	FString Producer;
};

using FTryllDialogComponent = TVariant<
	FTryllDialogSlot,
	FTryllDialogKnowledge,
	FTryllDialogIntention,
	FTryllDialogToolCallSnapshot,
	FTryllDialogToolResult,
	FTryllDialogSummary>;

struct TRYLLCLIENT_API FTryllDialogInteractionSnapshot
{
	bool bInterrupted = false;
	TArray<FTryllDialogComponent> Components;
};

struct TRYLLCLIENT_API FTryllDialogSnapshot
{
	uint32 FormatVersion = 1;
	FString GraphSignature;
	TArray<FTryllDialogInteractionSnapshot> Interactions;
	uint64 NextToolCallSeq = 0;
	uint64 NextExchangeId = 1;
	TArray<FTryllDialogProjectionWindow> ProjectionWindows;

	[[nodiscard]] bool ToFlatBufferBytes(TArray<uint8>& OutBytes, FTryllError& OutError) const;
	static bool FromFlatBufferBytes(
		TArrayView<const uint8> Bytes, FTryllDialogSnapshot& OutSnapshot, FTryllError& OutError);
};

struct TRYLLCLIENT_API FTryllDialogImportResult
{
	uint32 InteractionCount = 0;
	TArray<FString> Warnings;
};
