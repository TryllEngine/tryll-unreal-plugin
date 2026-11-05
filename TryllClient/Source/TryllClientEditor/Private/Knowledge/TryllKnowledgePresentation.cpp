// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "Knowledge/TryllKnowledgePresentation.h"

TArray<FTryllKnowledgeVisibleRow> FTryllKnowledgePresentation::VisibleRows() const
{
	TArray<FTryllKnowledgeVisibleRow> Rows;
	if (Mode == ETryllKnowledgeBrowseMode::Query)
	{
		for (const FTryllMutableKbHit& Hit : QueryHits)
		{
			FTryllKnowledgeVisibleRow Row;
			Row.Id = Hit.Id;
			Row.Text = Hit.Text;
			Row.Distance = Hit.Distance;
			Rows.Add(MoveTemp(Row));
		}
		return Rows;
	}

	for (const FTryllMutableKbRecord& Rec : Page.Records)
	{
		if (!Filter.IsEmpty())
		{
			const bool bIdHit = Rec.Id.Contains(Filter, ESearchCase::IgnoreCase);
			const bool bTextHit = Rec.Text.Contains(Filter, ESearchCase::IgnoreCase);
			if (!bIdHit && !bTextHit)
			{
				continue;
			}
		}
		FTryllKnowledgeVisibleRow Row;
		Row.Id = Rec.Id;
		Row.Text = Rec.Text;
		Rows.Add(MoveTemp(Row));
	}
	return Rows;
}

int32 FTryllKnowledgePresentation::BeginRefresh()
{
	return ++RefreshGeneration;
}

bool FTryllKnowledgePresentation::IsCurrent(const int32 Generation) const
{
	return Generation == RefreshGeneration;
}

void FTryllKnowledgePresentation::ApplyEnumerate(const TArray<FTryllMutableKbInfo>& InStores)
{
	Stores = InStores;
	bHasLastGoodView = true;
	StaleBanner.Reset();
	if (!SelectedName.IsEmpty())
	{
		bool bFound = false;
		for (const FTryllMutableKbInfo& Store : Stores)
		{
			if (Store.Name == SelectedName)
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			AddWarning(FString::Printf(TEXT("Store '%s' is no longer present."), *SelectedName));
		}
	}
	else if (Stores.Num() > 0 && !Stores[0].Name.IsEmpty())
	{
		SelectedName = Stores[0].Name;
	}
}

void FTryllKnowledgePresentation::SelectStore(const FString& Name)
{
	if (SelectedName == Name)
	{
		return;
	}
	SelectedName = Name;
	SelectedRecordId.Reset();
	Page = FTryllMutableKbListPage();
	Stats.Reset();
	bListTruncated = false;
	QueryHits.Reset();
}

void FTryllKnowledgePresentation::ApplyList(
	const FTryllMutableKbListPage& InPage, const FTryllMutableKbStats& InStats)
{
	Page = InPage;
	Stats = InStats;
	bListTruncated = Page.bHasMore;
	bHasLastGoodView = true;
	StaleBanner.Reset();
	PruneSelectionAgainstVisible();
}

void FTryllKnowledgePresentation::SetMode(const ETryllKnowledgeBrowseMode InMode)
{
	if (Mode == InMode)
	{
		return;
	}
	Mode = InMode;
	if (Mode == ETryllKnowledgeBrowseMode::Filter)
	{
		QueryHits.Reset();
	}
	PruneSelectionAgainstVisible();
}

void FTryllKnowledgePresentation::ApplyQueryHits(const TArray<FTryllMutableKbHit>& Hits)
{
	Mode = ETryllKnowledgeBrowseMode::Query;
	QueryHits = Hits;
	bHasLastGoodView = true;
	StaleBanner.Reset();
	PruneSelectionAgainstVisible();
}

void FTryllKnowledgePresentation::MarkStale(const FString& Message)
{
	StaleBanner = Message.IsEmpty() ? FString(TEXT("Refresh failed")) : Message;
}

void FTryllKnowledgePresentation::AddWarning(const FString& Warning)
{
	if (Warning.IsEmpty())
	{
		return;
	}
	Warnings.Add(Warning);
}

void FTryllKnowledgePresentation::ClearWarnings()
{
	Warnings.Reset();
}

bool FTryllKnowledgePresentation::IsDestructiveClearConfirmed(const bool bUserConfirmed)
{
	return bUserConfirmed;
}

bool FTryllKnowledgePresentation::ShouldSurfaceBusy(const FTryllError& Error)
{
	return Error.Code == static_cast<int32>(ETryllErrorCode::StorageBusy);
}

FString FTryllKnowledgePresentation::FormatWarning(const FTryllError& Error, const TArray<FString>& Extra)
{
	TArray<FString> Parts;
	if (!Error.IsOk() && !Error.Message.IsEmpty())
	{
		Parts.Add(Error.Message);
	}
	for (const FString& W : Extra)
	{
		if (!W.IsEmpty())
		{
			Parts.Add(W);
		}
	}
	return FString::Join(Parts, TEXT("\n"));
}

void FTryllKnowledgePresentation::PruneSelectionAgainstVisible()
{
	if (SelectedRecordId.IsEmpty())
	{
		return;
	}
	for (const FTryllKnowledgeVisibleRow& Row : VisibleRows())
	{
		if (Row.Id == SelectedRecordId)
		{
			return;
		}
	}
	SelectedRecordId.Reset();
}
