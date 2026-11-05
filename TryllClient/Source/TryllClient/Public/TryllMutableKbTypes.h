// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllGraphDescription.h"

enum class ETryllMutableKbCompatibility : uint8
{
	Strict = 0,
	AllowSchemaDrift = 1,
};

enum class ETryllMutableKbEmbedTiming : uint8
{
	Unspecified = 0,
	Now = 1,
	Lazy = 2,
};

struct TRYLLCLIENT_API FTryllMutableKbUpsert
{
	FString Id;
	FString Text;
};

struct TRYLLCLIENT_API FTryllMutableKbWriteResult
{
	FString Id;
	bool bInserted = false;
	bool bAccepted = false;
	FString Error;
};

struct TRYLLCLIENT_API FTryllMutableKbHit
{
	FString Id;
	FString Text;
	float Distance = 0.0f;
};

struct TRYLLCLIENT_API FTryllMutableKbRecord
{
	FString Id;
	FString Text;
	uint64 CreatedAtMs = 0;
	uint64 UpdatedAtMs = 0;
};

struct TRYLLCLIENT_API FTryllMutableKbListPage
{
	TArray<FTryllMutableKbRecord> Records;
	uint32 NextOffset = 0;
	bool bHasMore = false;
};

struct TRYLLCLIENT_API FTryllMutableKbStats
{
	uint32 LiveCount = 0;
	uint32 TombstonedCount = 0;
	uint32 SlotCount = 0;
	uint64 WriteGeneration = 0;
	uint64 BytesEstimate = 0;
	bool bMaterialized = false;
};

struct TRYLLCLIENT_API FTryllMutableKbInfo
{
	FString Name;
	FString Schema;
	uint32 LiveCount = 0;
	uint64 WriteGeneration = 0;
	bool bMaterialized = false;
	uint64 BytesEstimate = 0;
};

struct TRYLLCLIENT_API FTryllMutableKbImportResult
{
	FString Name;
	TArray<FString> Warnings;
};

struct TRYLLCLIENT_API FTryllGameMemorySaveResult
{
	uint64 SavedAtMs = 0;
	uint32 StoreCount = 0;
};

struct TRYLLCLIENT_API FTryllGameMemoryLoadResult
{
	uint32 StoreCount = 0;
	TArray<FString> Warnings;
};
