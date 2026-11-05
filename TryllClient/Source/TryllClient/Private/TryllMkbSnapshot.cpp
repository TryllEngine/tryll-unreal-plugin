// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllMkbSnapshot.h"

#include "TryllSnapshotHelpers.h"
#include "Win32Undef.h"
#include "mkb_snapshot_generated.h"
#include "tryll/ErrorCodes.h"
#include "tryll/Protocol.h"

#include <flatbuffers/flatbuffers.h>
#include <string>
#include <unordered_set>

namespace
{

// Shared with TryllDialogSnapshot.cpp through TryllSnapshotHelpers.h (td-042).
using Tryll::Client::Snapshot::Fail;
using Tryll::Client::Snapshot::FromUtf8;
using Tryll::Client::Snapshot::MakeFStringOpt;
using Tryll::Client::Snapshot::ToUtf8;

} // namespace

namespace Tryll::Client
{

bool PackPersistedMutableKb(
	flatbuffers::FlatBufferBuilder& Fbb,
	const FTryllMutableKbSnapshot& Snapshot,
	flatbuffers::Offset<::Tryll::PersistedMutableKb>& OutOffset,
	FTryllError& OutError)
{
	if (Snapshot.Name.IsEmpty())
	{
		Fail(OutError, ETryllErrorCode::MutableKbSnapshotInvalid, TEXT("PersistedMutableKb.name is required"));
		return false;
	}
	if (Snapshot.SchemaName.IsEmpty())
	{
		Fail(OutError, ETryllErrorCode::MutableKbSnapshotInvalid, TEXT("PersistedMutableKb.schema_name is required"));
		return false;
	}

	std::vector<flatbuffers::Offset<::Tryll::PersistedKbRecord>> RecOffs;
	RecOffs.reserve(static_cast<size_t>(Snapshot.Records.Num()));
	std::unordered_set<std::string> SeenIds;
	for (const FTryllMutableKbRecord& Rec : Snapshot.Records)
	{
		if (Rec.Id.IsEmpty())
		{
			Fail(OutError, ETryllErrorCode::MutableKbSnapshotInvalid, TEXT("PersistedKbRecord.id is required"));
			return false;
		}
		const std::string IdUtf8 = ToUtf8(Rec.Id);
		if (!SeenIds.insert(IdUtf8).second)
		{
			Fail(OutError, ETryllErrorCode::MutableKbSnapshotInvalid, TEXT("duplicate record id"));
			OutError.Message = FString::Printf(TEXT("duplicate record id '%s'"), *Rec.Id);
			return false;
		}
		RecOffs.push_back(::Tryll::CreatePersistedKbRecord(
			Fbb,
			Fbb.CreateString(IdUtf8),
			MakeFStringOpt(Fbb, Rec.Text),
			Rec.CreatedAtMs,
			Rec.UpdatedAtMs));
	}

	OutOffset = ::Tryll::CreatePersistedMutableKb(
		Fbb,
		Fbb.CreateString(ToUtf8(Snapshot.Name)),
		Fbb.CreateString(ToUtf8(Snapshot.SchemaName)),
		MakeFStringOpt(Fbb, Snapshot.SchemaFingerprint),
		MakeFStringOpt(Fbb, Snapshot.EmbeddingModel),
		Snapshot.WriteGeneration,
		Snapshot.NextGeneratedId,
		Fbb.CreateVector(RecOffs));
	OutError = FTryllError{};
	return true;
}

bool UnpackPersistedMutableKb(
	const ::Tryll::PersistedMutableKb& Table,
	FTryllMutableKbSnapshot& OutSnapshot,
	FTryllError& OutError)
{
	if (Table.name() == nullptr || Table.name()->size() == 0)
	{
		Fail(OutError, ETryllErrorCode::MutableKbSnapshotInvalid, TEXT("PersistedMutableKb.name is required"));
		return false;
	}
	if (Table.schema_name() == nullptr || Table.schema_name()->size() == 0)
	{
		Fail(OutError, ETryllErrorCode::MutableKbSnapshotInvalid, TEXT("PersistedMutableKb.schema_name is required"));
		return false;
	}

	FTryllMutableKbSnapshot Snapshot;
	Snapshot.Name = FromUtf8(Table.name());
	Snapshot.SchemaName = FromUtf8(Table.schema_name());
	Snapshot.SchemaFingerprint = FromUtf8(Table.schema_fingerprint());
	Snapshot.EmbeddingModel = FromUtf8(Table.embedding_model());
	Snapshot.WriteGeneration = Table.write_generation();
	Snapshot.NextGeneratedId = Table.next_generated_id();
	if (const auto* Records = Table.records())
	{
		std::unordered_set<std::string> SeenIds;
		Snapshot.Records.Reserve(static_cast<int32>(Records->size()));
		for (const auto* Rec : *Records)
		{
			if (Rec == nullptr || Rec->id() == nullptr || Rec->id()->size() == 0)
			{
				Fail(OutError, ETryllErrorCode::MutableKbSnapshotInvalid, TEXT("PersistedKbRecord.id is required"));
				return false;
			}
			FTryllMutableKbRecord Owned;
			Owned.Id = FromUtf8(Rec->id());
			Owned.Text = FromUtf8(Rec->text());
			Owned.CreatedAtMs = Rec->created_at_ms();
			Owned.UpdatedAtMs = Rec->updated_at_ms();
			if (!SeenIds.insert(ToUtf8(Owned.Id)).second)
			{
				OutError = FTryllError(
					ETryllErrorCode::MutableKbSnapshotInvalid,
					FString::Printf(TEXT("duplicate record id '%s'"), *Owned.Id));
				return false;
			}
			Snapshot.Records.Add(MoveTemp(Owned));
		}
	}
	OutSnapshot = MoveTemp(Snapshot);
	OutError = FTryllError{};
	return true;
}

} // namespace Tryll::Client

bool FTryllMutableKbSnapshot::ToFlatBufferBytes(TArray<uint8>& OutBytes, FTryllError& OutError) const
{
	flatbuffers::FlatBufferBuilder Fbb;
	flatbuffers::Offset<::Tryll::PersistedMutableKb> Offset;
	if (!Tryll::Client::PackPersistedMutableKb(Fbb, *this, Offset, OutError))
	{
		return false;
	}
	Fbb.Finish(Offset, "TMKB");
	if (Fbb.GetSize() > Tryll::MaxMutableKbSnapshotBytes)
	{
		Fail(OutError, ETryllErrorCode::StorageSnapshotTooLarge,
			TEXT("Mutable KB snapshot exceeds MaxMutableKbSnapshotBytes"));
		return false;
	}
	const uint8* Buf = Fbb.GetBufferPointer();
	OutBytes.SetNumUninitialized(static_cast<int32>(Fbb.GetSize()));
	FMemory::Memcpy(OutBytes.GetData(), Buf, Fbb.GetSize());
	OutError = FTryllError{};
	return true;
}

bool FTryllMutableKbSnapshot::FromFlatBufferBytes(
	TArrayView<const uint8> Bytes, FTryllMutableKbSnapshot& OutSnapshot, FTryllError& OutError)
{
	if (Bytes.Num() < 8)
	{
		Fail(OutError, ETryllErrorCode::MutableKbSnapshotInvalid,
			TEXT("Buffer is not a valid TMKB store snapshot"));
		return false;
	}
	flatbuffers::Verifier Verifier{Bytes.GetData(), static_cast<size_t>(Bytes.Num())};
	if (!::Tryll::PersistedMutableKbBufferHasIdentifier(Bytes.GetData())
		|| !::Tryll::VerifyPersistedMutableKbBuffer(Verifier))
	{
		Fail(OutError, ETryllErrorCode::MutableKbSnapshotInvalid,
			TEXT("Buffer is not a valid TMKB store snapshot"));
		return false;
	}
	const auto* Table = ::Tryll::GetPersistedMutableKb(Bytes.GetData());
	if (Table == nullptr)
	{
		Fail(OutError, ETryllErrorCode::MutableKbSnapshotInvalid, TEXT("Null PersistedMutableKb root"));
		return false;
	}
	return Tryll::Client::UnpackPersistedMutableKb(*Table, OutSnapshot, OutError);
}

bool FTryllGameMemorySnapshot::ToFlatBufferBytes(TArray<uint8>& OutBytes, FTryllError& OutError) const
{
	if (FormatVersion == 0)
	{
		Fail(OutError, ETryllErrorCode::MutableKbSnapshotInvalid, TEXT("format_version must not be 0"));
		return false;
	}

	flatbuffers::FlatBufferBuilder Fbb;
	std::vector<flatbuffers::Offset<::Tryll::PersistedMutableKb>> StoreOffs;
	StoreOffs.reserve(static_cast<size_t>(Stores.Num()));
	std::unordered_set<std::string> SeenNames;
	for (const FTryllMutableKbSnapshot& Store : Stores)
	{
		if (!Store.Name.IsEmpty() && !SeenNames.insert(ToUtf8(Store.Name)).second)
		{
			OutError = FTryllError(
				ETryllErrorCode::MutableKbSnapshotInvalid,
				FString::Printf(TEXT("duplicate store name '%s'"), *Store.Name));
			return false;
		}
		flatbuffers::Offset<::Tryll::PersistedMutableKb> StoreOff;
		if (!Tryll::Client::PackPersistedMutableKb(Fbb, Store, StoreOff, OutError))
		{
			return false;
		}
		StoreOffs.push_back(StoreOff);
	}

	const auto Offset = ::Tryll::CreateGameMemory(
		Fbb,
		FormatVersion,
		SavedAtMs,
		MakeFStringOpt(Fbb, SaveId),
		Fbb.CreateVector(StoreOffs));
	Fbb.Finish(Offset, "TMKB");
	if (Fbb.GetSize() > Tryll::MaxGameMemorySnapshotBytes)
	{
		Fail(OutError, ETryllErrorCode::StorageSnapshotTooLarge,
			TEXT("GameMemory snapshot exceeds MaxGameMemorySnapshotBytes"));
		return false;
	}
	const uint8* Buf = Fbb.GetBufferPointer();
	OutBytes.SetNumUninitialized(static_cast<int32>(Fbb.GetSize()));
	FMemory::Memcpy(OutBytes.GetData(), Buf, Fbb.GetSize());
	OutError = FTryllError{};
	return true;
}

bool FTryllGameMemorySnapshot::FromFlatBufferBytes(
	TArrayView<const uint8> Bytes, FTryllGameMemorySnapshot& OutSnapshot, FTryllError& OutError)
{
	if (Bytes.Num() < 8)
	{
		Fail(OutError, ETryllErrorCode::MutableKbSnapshotInvalid,
			TEXT("Buffer is not a valid TMKB GameMemory snapshot"));
		return false;
	}
	flatbuffers::Verifier Verifier{Bytes.GetData(), static_cast<size_t>(Bytes.Num())};
	if (!::Tryll::PersistedMutableKbBufferHasIdentifier(Bytes.GetData())
		|| !Verifier.VerifyBuffer<::Tryll::GameMemory>(::Tryll::PersistedMutableKbIdentifier()))
	{
		Fail(OutError, ETryllErrorCode::MutableKbSnapshotInvalid,
			TEXT("Buffer is not a valid TMKB GameMemory snapshot"));
		return false;
	}
	const auto* Table = flatbuffers::GetRoot<::Tryll::GameMemory>(Bytes.GetData());
	if (Table == nullptr)
	{
		Fail(OutError, ETryllErrorCode::MutableKbSnapshotInvalid, TEXT("Null GameMemory root"));
		return false;
	}

	FTryllGameMemorySnapshot Snapshot;
	Snapshot.FormatVersion = Table->format_version();
	if (Snapshot.FormatVersion == 0)
	{
		Fail(OutError, ETryllErrorCode::MutableKbSnapshotInvalid, TEXT("format_version must not be 0"));
		return false;
	}
	if (Snapshot.FormatVersion > TryllGameMemoryFormatVersion)
	{
		Fail(OutError, ETryllErrorCode::MutableKbSnapshotInvalid, TEXT("Unsupported GameMemory format_version"));
		return false;
	}
	Snapshot.SavedAtMs = Table->saved_at_ms();
	Snapshot.SaveId = FromUtf8(Table->save_id());
	if (const auto* Stores = Table->stores())
	{
		std::unordered_set<std::string> SeenNames;
		Snapshot.Stores.Reserve(static_cast<int32>(Stores->size()));
		for (const auto* Store : *Stores)
		{
			if (Store == nullptr)
			{
				Fail(OutError, ETryllErrorCode::MutableKbSnapshotInvalid, TEXT("Null PersistedMutableKb"));
				return false;
			}
			FTryllMutableKbSnapshot Captured;
			if (!Tryll::Client::UnpackPersistedMutableKb(*Store, Captured, OutError))
			{
				return false;
			}
			if (!SeenNames.insert(ToUtf8(Captured.Name)).second)
			{
				OutError = FTryllError(
					ETryllErrorCode::MutableKbSnapshotInvalid,
					FString::Printf(TEXT("duplicate store name '%s'"), *Captured.Name));
				return false;
			}
			Snapshot.Stores.Add(MoveTemp(Captured));
		}
	}
	OutSnapshot = MoveTemp(Snapshot);
	OutError = FTryllError{};
	return true;
}
