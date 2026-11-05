// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllError.h"
#include "TryllMutableKbTypes.h"

inline constexpr uint32 TryllGameMemoryFormatVersion = 1;

struct TRYLLCLIENT_API FTryllMutableKbSnapshot
{
	FString Name;
	FString SchemaName;
	FString SchemaFingerprint;
	FString EmbeddingModel;
	uint64 WriteGeneration = 0;
	uint64 NextGeneratedId = 1;
	TArray<FTryllMutableKbRecord> Records;

	[[nodiscard]] bool ToFlatBufferBytes(TArray<uint8>& OutBytes, FTryllError& OutError) const;
	static bool FromFlatBufferBytes(
		TArrayView<const uint8> Bytes, FTryllMutableKbSnapshot& OutSnapshot, FTryllError& OutError);
};

struct TRYLLCLIENT_API FTryllGameMemorySnapshot
{
	uint32 FormatVersion = TryllGameMemoryFormatVersion;
	uint64 SavedAtMs = 0;
	FString SaveId;
	TArray<FTryllMutableKbSnapshot> Stores;

	[[nodiscard]] bool ToFlatBufferBytes(TArray<uint8>& OutBytes, FTryllError& OutError) const;
	static bool FromFlatBufferBytes(
		TArrayView<const uint8> Bytes, FTryllGameMemorySnapshot& OutSnapshot, FTryllError& OutError);
};
