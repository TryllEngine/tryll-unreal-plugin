// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllMemorySnapshot.h"

/** Feature-local Memory presentation state. The panel owns host lifecycle around it. */
struct TRYLLCLIENTEDITOR_API FTryllMemoryPresentationState
{
	TOptional<FTryllMemorySnapshot> Snapshot;
	FString StaleError;
	bool bRefreshInFlight = false;
};

/**
 * Feature-local Memory presentation core: formatters, stale-state reducers and
 * row projections. No Slate or editor-lifecycle dependency.
 */
struct TRYLLCLIENTEDITOR_API FTryllMemoryPresentation
{
	static FString FmtBinaryBytes(int64 Bytes);
	static FString FmtClock(int64 UnixMs);
	static FString FmtIdle(int64 LastTurnAtMs, int64 NowMs);
	static int32 ClassOrder(ETryllInferenceConsumerClass Cls);
	static FString ConsumerClassLabel(ETryllInferenceConsumerClass Cls, bool bResident);
	static FString FormatRamRange(int64 RamBytes, int64 RamBytesHigh, ETryllMemoryTier Tier);
	static FString ContextCaption(int32 UsedTokens, int32 CapacityTokens, bool bOccupancyStale);
	static FString StaleBannerText(const TOptional<FTryllMemorySnapshot>& Snapshot, const FString& StaleError);

	static FTryllMemoryPresentationState ApplyRefreshStarted(FTryllMemoryPresentationState State);
	static FTryllMemoryPresentationState ApplyRefreshSucceeded(
		FTryllMemoryPresentationState State, FTryllMemorySnapshot Snapshot);
	static FTryllMemoryPresentationState ApplyRefreshFailed(
		FTryllMemoryPresentationState State, const FString& Error);
	static FTryllMemoryPresentationState ApplyDisconnected(
		FTryllMemoryPresentationState State, const FString& Reason);

	enum class ETone : uint8 { Dim, Ok, Accent, Warn, Err };
	enum class EDeviceSection : uint8 { Uninitialized, Empty, Table };

	static const TCHAR* UnattributedMappedNote;
	static const TCHAR* DevicesUninitializedNote;
	static const TCHAR* DevicesEmptyNote;
	static const TCHAR* DevicesFooterNote;
	static const TCHAR* ContextHeadroomNote;
	static const TCHAR* EmptyKvNote;

	struct FDeviceRow
	{
		FString Label;
		FString Kind;
		FString Self;
		FString Other;
		FString Free;
		FString Total;
	};

	struct FKvRow
	{
		FString Name;
		FString ClassLabel;
		ETone ClassTone = ETone::Dim;
		bool bBackground = false;
		bool bOccupancyStale = false;
		int32 UsedTokens = 0;
		int32 CapacityTokens = 0;
		float OccupancyFrac = 0.f;
		FString ContextCaption;
		FString When;
		FString Reserved;
	};

	struct FComponentRow
	{
		FString Kind;
		FString Name;
		FString TierLabel;
		ETone TierTone = ETone::Dim;
		FString Ram;
		bool bRamWarn = false;
		FString Vram;
		FString Tooltip;
	};

	struct FTables
	{
		TArray<FString> SystemNotes;
		EDeviceSection DevicesKind = EDeviceSection::Uninitialized;
		TArray<FDeviceRow> Devices;
		FString KvTotalLabel = TEXT("0 B");
		FString BackgroundKvNote;
		TArray<FKvRow> KvRows;
		TArray<FComponentRow> ComponentRows;
	};

	static ETone ClassTone(ETryllInferenceConsumerClass Cls, bool bResident);
	static void TierPresentation(ETryllMemoryTier Tier, FString& OutLabel, ETone& OutTone);
	static FString DisplayName(const FString& Name);
	static FString DeviceLabel(const FString& Description, const FString& Name);
	static FTables Project(const FTryllMemorySnapshot& Snapshot, int64 NowMs);
};
