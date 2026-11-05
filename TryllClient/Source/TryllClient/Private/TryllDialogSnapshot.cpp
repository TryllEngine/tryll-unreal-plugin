// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogSnapshot.h"

#include "TryllSnapshotHelpers.h"
#include "Win32Undef.h"
#include "messages_generated.h"
#include "tryll/ErrorCodes.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <flatbuffers/flatbuffers.h>

namespace
{

// Shared with TryllMkbSnapshot.cpp through TryllSnapshotHelpers.h (td-042).
using Tryll::Client::Snapshot::Fail;
using Tryll::Client::Snapshot::FromUtf8;
using Tryll::Client::Snapshot::MakeFStringOpt;
using Tryll::Client::Snapshot::ToUtf8;

[[nodiscard]] bool PackComponent(
	flatbuffers::FlatBufferBuilder& Fbb,
	const FTryllDialogComponent& Component,
	::Tryll::PersistedComponent& TypeOut,
	flatbuffers::Offset<void>& ValueOut,
	FTryllError& OutError)
{
	if (const FTryllDialogSlot* Slot = Component.TryGet<FTryllDialogSlot>())
	{
		TypeOut = ::Tryll::PersistedComponent_PersistedSlot;
		ValueOut = ::Tryll::CreatePersistedSlot(
			Fbb,
			MakeFStringOpt(Fbb, Slot->Name),
			MakeFStringOpt(Fbb, Slot->Text),
			static_cast<::Tryll::PersistedSlotKind>(Slot->Kind),
			static_cast<::Tryll::HistoryRole>(Slot->HistoryRole),
			MakeFStringOpt(Fbb, Slot->Producer)).Union();
		return true;
	}
	if (const FTryllDialogKnowledge* Knowledge = Component.TryGet<FTryllDialogKnowledge>())
	{
		TypeOut = ::Tryll::PersistedComponent_PersistedKnowledge;
		std::vector<flatbuffers::Offset<::Tryll::PersistedKnowledgeChunk>> ChunkOffs;
		ChunkOffs.reserve(static_cast<size_t>(Knowledge->Chunks.Num()));
		for (const FTryllDialogKnowledgeChunk& Chunk : Knowledge->Chunks)
		{
			const auto IdOff = MakeFStringOpt(Fbb, Chunk.Id);
			const auto TextOff = MakeFStringOpt(Fbb, Chunk.Text);
			::Tryll::PersistedKnowledgeChunkBuilder ChunkBuilder{Fbb};
			ChunkBuilder.add_id(IdOff);
			ChunkBuilder.add_text(TextOff);
			if (std::isnan(Chunk.Distance))
				ChunkBuilder.add_distance_is_nan(true);
			else
				ChunkBuilder.add_distance(Chunk.Distance);
			if (Chunk.Bm25Score.IsSet())
				ChunkBuilder.add_bm25_score(Chunk.Bm25Score.GetValue());
			if (Chunk.RankDense.IsSet())
				ChunkBuilder.add_rank_dense(Chunk.RankDense.GetValue());
			if (Chunk.RankSparse.IsSet())
				ChunkBuilder.add_rank_sparse(Chunk.RankSparse.GetValue());
			ChunkOffs.push_back(ChunkBuilder.Finish());
		}
		ValueOut = ::Tryll::CreatePersistedKnowledge(
			Fbb,
			MakeFStringOpt(Fbb, Knowledge->Source),
			Fbb.CreateVector(ChunkOffs)).Union();
		return true;
	}
	if (const FTryllDialogIntention* Intention = Component.TryGet<FTryllDialogIntention>())
	{
		TypeOut = ::Tryll::PersistedComponent_PersistedIntention;
		ValueOut = ::Tryll::CreatePersistedIntention(
			Fbb, MakeFStringOpt(Fbb, Intention->Intent)).Union();
		return true;
	}
	if (const FTryllDialogToolCallSnapshot* Call = Component.TryGet<FTryllDialogToolCallSnapshot>())
	{
		TypeOut = ::Tryll::PersistedComponent_PersistedToolCall;
		TArray<TPair<FString, FString>> Arguments = Call->Arguments;
		Arguments.Sort([](const TPair<FString, FString>& A, const TPair<FString, FString>& B)
		{
			return A.Key < B.Key;
		});
		std::vector<flatbuffers::Offset<::Tryll::PersistedStringPair>> ArgOffs;
		ArgOffs.reserve(static_cast<size_t>(Arguments.Num()));
		for (const TPair<FString, FString>& Pair : Arguments)
		{
			const std::string KeyUtf8 = ToUtf8(Pair.Key);
			ArgOffs.push_back(::Tryll::CreatePersistedStringPair(
				Fbb, Fbb.CreateString(KeyUtf8), MakeFStringOpt(Fbb, Pair.Value)));
		}
		ValueOut = ::Tryll::CreatePersistedToolCall(
			Fbb,
			MakeFStringOpt(Fbb, Call->ToolName),
			Fbb.CreateVector(ArgOffs),
			MakeFStringOpt(Fbb, Call->ArgumentsJson),
			Call->bDispatched,
			MakeFStringOpt(Fbb, Call->Id),
			static_cast<::Tryll::ToolCallDisposition>(Call->Disposition),
			MakeFStringOpt(Fbb, Call->ModelId),
			Call->ExchangeId).Union();
		return true;
	}
	if (const FTryllDialogToolResult* Result = Component.TryGet<FTryllDialogToolResult>())
	{
		TypeOut = ::Tryll::PersistedComponent_PersistedToolResult;
		ValueOut = ::Tryll::CreatePersistedToolResult(
			Fbb,
			MakeFStringOpt(Fbb, Result->CallId),
			MakeFStringOpt(Fbb, Result->ToolName),
			MakeFStringOpt(Fbb, Result->Text),
			Result->ExchangeId).Union();
		return true;
	}
	if (const FTryllDialogSummary* Summary = Component.TryGet<FTryllDialogSummary>())
	{
		TypeOut = ::Tryll::PersistedComponent_PersistedSummary;
		ValueOut = ::Tryll::CreatePersistedSummary(
			Fbb,
			MakeFStringOpt(Fbb, Summary->Text),
			Summary->CoveredThrough,
			MakeFStringOpt(Fbb, Summary->Producer)).Union();
		return true;
	}
	Fail(OutError, ETryllErrorCode::DialogUnsupportedComponent, TEXT("Unsupported dialog component type"));
	return false;
}

[[nodiscard]] bool UnpackComponent(
	::Tryll::PersistedComponent Type,
	const void* Table,
	FTryllDialogComponent& OutComponent,
	FTryllError& OutError)
{
	switch (Type)
	{
	case ::Tryll::PersistedComponent_PersistedSlot:
	{
		const auto* Slot = static_cast<const ::Tryll::PersistedSlot*>(Table);
		if (Slot == nullptr)
		{
			Fail(OutError, ETryllErrorCode::DialogSnapshotInvalid, TEXT("Null PersistedSlot"));
			return false;
		}
		FTryllDialogSlot Payload;
		Payload.Name = FromUtf8(Slot->name());
		Payload.Text = FromUtf8(Slot->text());
		Payload.Kind = static_cast<ETryllDialogSlotKind>(Slot->kind());
		Payload.HistoryRole = static_cast<uint8>(Slot->history_role());
		Payload.Producer = FromUtf8(Slot->producer());
		OutComponent.Set<FTryllDialogSlot>(MoveTemp(Payload));
		return true;
	}
	case ::Tryll::PersistedComponent_PersistedKnowledge:
	{
		const auto* Knowledge = static_cast<const ::Tryll::PersistedKnowledge*>(Table);
		if (Knowledge == nullptr)
		{
			Fail(OutError, ETryllErrorCode::DialogSnapshotInvalid, TEXT("Null PersistedKnowledge"));
			return false;
		}
		FTryllDialogKnowledge Payload;
		Payload.Source = FromUtf8(Knowledge->source());
		if (const auto* Chunks = Knowledge->chunks())
		{
			Payload.Chunks.Reserve(static_cast<int32>(Chunks->size()));
			for (const auto* Chunk : *Chunks)
			{
				if (Chunk == nullptr)
				{
					Fail(OutError, ETryllErrorCode::DialogSnapshotInvalid, TEXT("Null chunk"));
					return false;
				}
				FTryllDialogKnowledgeChunk Decoded;
				Decoded.Id = FromUtf8(Chunk->id());
				Decoded.Text = FromUtf8(Chunk->text());
				Decoded.Distance = Chunk->distance_is_nan()
					? std::numeric_limits<float>::quiet_NaN()
					: Chunk->distance();
				if (const auto Score = Chunk->bm25_score())
					Decoded.Bm25Score = *Score;
				if (const auto RankDense = Chunk->rank_dense())
					Decoded.RankDense = *RankDense;
				if (const auto RankSparse = Chunk->rank_sparse())
					Decoded.RankSparse = *RankSparse;
				Payload.Chunks.Add(MoveTemp(Decoded));
			}
		}
		OutComponent.Set<FTryllDialogKnowledge>(MoveTemp(Payload));
		return true;
	}
	case ::Tryll::PersistedComponent_PersistedIntention:
	{
		const auto* Intention = static_cast<const ::Tryll::PersistedIntention*>(Table);
		if (Intention == nullptr)
		{
			Fail(OutError, ETryllErrorCode::DialogSnapshotInvalid, TEXT("Null PersistedIntention"));
			return false;
		}
		FTryllDialogIntention Payload;
		Payload.Intent = FromUtf8(Intention->intent());
		OutComponent.Set<FTryllDialogIntention>(MoveTemp(Payload));
		return true;
	}
	case ::Tryll::PersistedComponent_PersistedToolCall:
	{
		const auto* Call = static_cast<const ::Tryll::PersistedToolCall*>(Table);
		if (Call == nullptr)
		{
			Fail(OutError, ETryllErrorCode::DialogSnapshotInvalid, TEXT("Null PersistedToolCall"));
			return false;
		}
		FTryllDialogToolCallSnapshot Payload;
		Payload.ToolName = FromUtf8(Call->tool_name());
		if (const auto* Arguments = Call->arguments())
		{
			Payload.Arguments.Reserve(static_cast<int32>(Arguments->size()));
			for (const auto* Pair : *Arguments)
			{
				if (Pair == nullptr || Pair->key() == nullptr)
				{
					Fail(OutError, ETryllErrorCode::DialogSnapshotInvalid, TEXT("Tool argument missing key"));
					return false;
				}
				Payload.Arguments.Emplace(FromUtf8(Pair->key()), FromUtf8(Pair->value()));
			}
		}
		Payload.ArgumentsJson = FromUtf8(Call->arguments_json());
		Payload.bDispatched = Call->dispatched();
		Payload.Id = FromUtf8(Call->id());
		Payload.Disposition = static_cast<uint8>(Call->disposition());
		Payload.ModelId = FromUtf8(Call->model_id());
		Payload.ExchangeId = Call->exchange_id();
		OutComponent.Set<FTryllDialogToolCallSnapshot>(MoveTemp(Payload));
		return true;
	}
	case ::Tryll::PersistedComponent_PersistedToolResult:
	{
		const auto* Result = static_cast<const ::Tryll::PersistedToolResult*>(Table);
		if (Result == nullptr)
		{
			Fail(OutError, ETryllErrorCode::DialogSnapshotInvalid, TEXT("Null PersistedToolResult"));
			return false;
		}
		FTryllDialogToolResult Payload;
		Payload.CallId = FromUtf8(Result->call_id());
		Payload.ToolName = FromUtf8(Result->tool_name());
		Payload.Text = FromUtf8(Result->text());
		Payload.ExchangeId = Result->exchange_id();
		OutComponent.Set<FTryllDialogToolResult>(MoveTemp(Payload));
		return true;
	}
	case ::Tryll::PersistedComponent_PersistedSummary:
	{
		const auto* Summary = static_cast<const ::Tryll::PersistedSummary*>(Table);
		if (Summary == nullptr)
		{
			Fail(OutError, ETryllErrorCode::DialogSnapshotInvalid, TEXT("Null PersistedSummary"));
			return false;
		}
		FTryllDialogSummary Payload;
		Payload.Text = FromUtf8(Summary->text());
		Payload.CoveredThrough = Summary->covered_through();
		Payload.Producer = FromUtf8(Summary->producer());
		OutComponent.Set<FTryllDialogSummary>(MoveTemp(Payload));
		return true;
	}
	case ::Tryll::PersistedComponent_NONE:
		Fail(OutError, ETryllErrorCode::DialogSnapshotInvalid, TEXT("PersistedComponent is NONE"));
		return false;
	default:
		Fail(OutError, ETryllErrorCode::DialogUnsupportedComponent, TEXT("Unknown PersistedComponent union arm"));
		return false;
	}
}

} // namespace

namespace Tryll::Client {

bool PackPersistedDialogSnapshot(
	flatbuffers::FlatBufferBuilder& Fbb,
	const FTryllDialogSnapshot& Snapshot,
	flatbuffers::Offset<::Tryll::PersistedDialogSnapshot>& OutOffset,
	FTryllError& OutError)
{
	OutError = FTryllError{};
	std::vector<flatbuffers::Offset<::Tryll::PersistedInteraction>> InteractionOffs;
	InteractionOffs.reserve(static_cast<size_t>(Snapshot.Interactions.Num()));
	for (const FTryllDialogInteractionSnapshot& Interaction : Snapshot.Interactions)
	{
		std::vector<flatbuffers::Offset<::Tryll::PersistedComponentEntry>> Entries;
		Entries.reserve(static_cast<size_t>(Interaction.Components.Num()));
		for (const FTryllDialogComponent& Component : Interaction.Components)
		{
			::Tryll::PersistedComponent Type = ::Tryll::PersistedComponent_NONE;
			flatbuffers::Offset<void> Value;
			if (!PackComponent(Fbb, Component, Type, Value, OutError))
				return false;
			Entries.push_back(::Tryll::CreatePersistedComponentEntry(Fbb, Type, Value));
		}
		InteractionOffs.push_back(::Tryll::CreatePersistedInteraction(
			Fbb, Interaction.bInterrupted, Fbb.CreateVector(Entries)));
	}

	std::vector<flatbuffers::Offset<::Tryll::PersistedProjectionWindow>> WindowOffs;
	WindowOffs.reserve(static_cast<size_t>(Snapshot.ProjectionWindows.Num()));
	for (const FTryllDialogProjectionWindow& Window : Snapshot.ProjectionWindows)
	{
		const std::string NodeUtf8 = ToUtf8(Window.NodeName);
		WindowOffs.push_back(::Tryll::CreatePersistedProjectionWindow(
			Fbb, Fbb.CreateString(NodeUtf8), Window.WindowStart));
	}

	OutOffset = ::Tryll::CreatePersistedDialogSnapshot(
		Fbb,
		Snapshot.FormatVersion,
		MakeFStringOpt(Fbb, Snapshot.GraphSignature),
		Fbb.CreateVector(InteractionOffs),
		Snapshot.NextToolCallSeq,
		Snapshot.NextExchangeId,
		Fbb.CreateVector(WindowOffs));
	return true;
}

bool UnpackPersistedDialogSnapshot(
	const ::Tryll::PersistedDialogSnapshot& Table,
	FTryllDialogSnapshot& OutSnapshot,
	FTryllError& OutError)
{
	OutSnapshot = FTryllDialogSnapshot{};
	OutSnapshot.FormatVersion = Table.format_version();
	if (OutSnapshot.FormatVersion == 0)
	{
		Fail(OutError, ETryllErrorCode::DialogSnapshotInvalid, TEXT("format_version must not be 0"));
		return false;
	}
	if (OutSnapshot.FormatVersion > 1)
	{
		Fail(OutError, ETryllErrorCode::DialogFormatUnsupported, TEXT("Unsupported dialog snapshot format_version"));
		return false;
	}
	OutSnapshot.GraphSignature = FromUtf8(Table.graph_signature());
	OutSnapshot.NextToolCallSeq = Table.next_tool_call_seq();
	OutSnapshot.NextExchangeId = Table.next_exchange_id();
	if (OutSnapshot.NextExchangeId == 0)
	{
		Fail(OutError, ETryllErrorCode::DialogSnapshotInvalid, TEXT("next_exchange_id 0 is reserved"));
		return false;
	}
	if (const auto* Interactions = Table.interactions())
	{
		OutSnapshot.Interactions.Reserve(static_cast<int32>(Interactions->size()));
		for (const auto* Interaction : *Interactions)
		{
			if (Interaction == nullptr)
			{
				Fail(OutError, ETryllErrorCode::DialogSnapshotInvalid, TEXT("Null interaction"));
				return false;
			}
			FTryllDialogInteractionSnapshot Captured;
			Captured.bInterrupted = Interaction->interrupted();
			if (const auto* Entries = Interaction->components())
			{
				Captured.Components.Reserve(static_cast<int32>(Entries->size()));
				for (const auto* Entry : *Entries)
				{
					if (Entry == nullptr)
					{
						Fail(OutError, ETryllErrorCode::DialogSnapshotInvalid, TEXT("Null PersistedComponentEntry"));
						return false;
					}
					FTryllDialogComponent Component;
					if (!UnpackComponent(Entry->value_type(), Entry->value(), Component, OutError))
						return false;
					Captured.Components.Add(MoveTemp(Component));
				}
			}
			OutSnapshot.Interactions.Add(MoveTemp(Captured));
		}
	}
	if (const auto* Windows = Table.projection_windows())
	{
		OutSnapshot.ProjectionWindows.Reserve(static_cast<int32>(Windows->size()));
		for (const auto* Window : *Windows)
		{
			if (Window == nullptr || Window->node_name() == nullptr
				|| Window->node_name()->size() == 0)
			{
				Fail(OutError, ETryllErrorCode::DialogSnapshotInvalid, TEXT("Projection window missing node_name"));
				return false;
			}
			FTryllDialogProjectionWindow Decoded;
			Decoded.NodeName = FromUtf8(Window->node_name());
			Decoded.WindowStart = Window->window_start();
			OutSnapshot.ProjectionWindows.Add(MoveTemp(Decoded));
		}
	}
	return true;
}

} // namespace Tryll::Client

bool FTryllDialogSnapshot::ToFlatBufferBytes(TArray<uint8>& OutBytes, FTryllError& OutError) const
{
	OutBytes.Reset();
	OutError = FTryllError{};
	flatbuffers::FlatBufferBuilder Fbb;
	flatbuffers::Offset<::Tryll::PersistedDialogSnapshot> Offset;
	if (!Tryll::Client::PackPersistedDialogSnapshot(Fbb, *this, Offset, OutError))
		return false;
	::Tryll::FinishPersistedDialogSnapshotBuffer(Fbb, Offset);
	OutBytes.Append(Fbb.GetBufferPointer(), static_cast<int32>(Fbb.GetSize()));
	return true;
}

bool FTryllDialogSnapshot::FromFlatBufferBytes(
	TArrayView<const uint8> Bytes, FTryllDialogSnapshot& OutSnapshot, FTryllError& OutError)
{
	OutError = FTryllError{};
	if (Bytes.Num() < 8)
	{
		Fail(OutError, ETryllErrorCode::DialogSnapshotInvalid, TEXT("Dialog snapshot buffer is too short"));
		return false;
	}
	if (!::Tryll::PersistedDialogSnapshotBufferHasIdentifier(Bytes.GetData()))
	{
		Fail(OutError, ETryllErrorCode::DialogFormatUnsupported, TEXT("Dialog snapshot file identifier is not TDLG"));
		return false;
	}
	flatbuffers::Verifier Verifier{Bytes.GetData(), static_cast<size_t>(Bytes.Num())};
	if (!::Tryll::VerifyPersistedDialogSnapshotBuffer(Verifier))
	{
		Fail(OutError, ETryllErrorCode::DialogSnapshotInvalid, TEXT("Dialog snapshot failed verification"));
		return false;
	}
	const auto* Table = ::Tryll::GetPersistedDialogSnapshot(Bytes.GetData());
	if (Table == nullptr)
	{
		Fail(OutError, ETryllErrorCode::DialogSnapshotInvalid, TEXT("Dialog snapshot root is null"));
		return false;
	}
	return Tryll::Client::UnpackPersistedDialogSnapshot(*Table, OutSnapshot, OutError);
}
