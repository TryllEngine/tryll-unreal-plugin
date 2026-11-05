// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllError.h"
#include "TryllMutableKbTypes.h"

enum class ETryllKnowledgeBrowseMode : uint8
{
	Filter = 0,
	Query = 1,
};

struct TRYLLCLIENTEDITOR_API FTryllKnowledgeVisibleRow
{
	FString Id;
	FString Text;
	TOptional<float> Distance;
};

/**
 * Testable Knowledge-window reducer. Owns the last-good store list, records,
 * query hits, and banners. Network/UI stay outside.
 */
struct TRYLLCLIENTEDITOR_API FTryllKnowledgePresentation
{
	static constexpr uint32 ListLimit = 1024;

	int32 RefreshGeneration = 0;
	TArray<FTryllMutableKbInfo> Stores;
	FString SelectedName;
	FTryllMutableKbListPage Page;
	TOptional<FTryllMutableKbStats> Stats;
	FString Filter;
	FString SelectedRecordId;
	FString StaleBanner;
	TArray<FString> Warnings;
	bool bHasLastGoodView = false;
	bool bListTruncated = false;
	ETryllKnowledgeBrowseMode Mode = ETryllKnowledgeBrowseMode::Filter;
	TArray<FTryllMutableKbHit> QueryHits;

	[[nodiscard]] TArray<FTryllKnowledgeVisibleRow> VisibleRows() const;

	int32 BeginRefresh();
	[[nodiscard]] bool IsCurrent(int32 Generation) const;

	void ApplyEnumerate(const TArray<FTryllMutableKbInfo>& InStores);
	void SelectStore(const FString& Name);
	void ApplyList(const FTryllMutableKbListPage& InPage, const FTryllMutableKbStats& InStats);
	void SetMode(ETryllKnowledgeBrowseMode InMode);
	void ApplyQueryHits(const TArray<FTryllMutableKbHit>& Hits);
	void MarkStale(const FString& Message);
	void AddWarning(const FString& Warning);
	void ClearWarnings();

	static bool IsDestructiveClearConfirmed(bool bUserConfirmed);
	static bool ShouldSurfaceBusy(const FTryllError& Error);
	static FString FormatWarning(const FTryllError& Error, const TArray<FString>& Extra = {});

private:
	void PruneSelectionAgainstVisible();
};
