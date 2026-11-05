// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogLabRetrieval.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

namespace
{
	const TCHAR* DefaultMode = TEXT("Dense");

	bool ReadNumber(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, double& OutValue)
	{
		const TSharedPtr<FJsonValue> Value = Object->TryGetField(Field);
		if (!Value.IsValid() || Value->Type != EJson::Number)
		{
			return false;
		}
		OutValue = Value->AsNumber();
		return true;
	}

	FString ModeOr(const FString& Mode)
	{
		return Mode.IsEmpty() ? FString(DefaultMode) : Mode;
	}

	/** Trims a fixed-decimal string the way .NET's "0.###" does. */
	FString TrimTrailingZeros(FString Text)
	{
		if (!Text.Contains(TEXT(".")))
		{
			return Text;
		}
		while (Text.EndsWith(TEXT("0"))) { Text.LeftChopInline(1); }
		if (Text.EndsWith(TEXT("."))) { Text.LeftChopInline(1); }
		return Text;
	}
}

FString FTryllLabRetrievalHit::FormatDistance() const
{
	return Distance.IsSet()
		? FString::Printf(TEXT("%.4f"), Distance.GetValue())
		: FString(TEXT("—"));
}

TArray<FTryllLabRetrievalHit> FTryllDialogLabRetrieval::ParseHits(
	const TSharedPtr<FJsonValue>& ResultsArray)
{
	TArray<FTryllLabRetrievalHit> Hits;
	if (!ResultsArray.IsValid() || ResultsArray->Type != EJson::Array)
	{
		return Hits;
	}

	for (const TSharedPtr<FJsonValue>& Entry : ResultsArray->AsArray())
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (!Entry.IsValid() || !Entry->TryGetObject(Object) || Object == nullptr)
		{
			continue;
		}

		FTryllLabRetrievalHit Hit;
		(*Object)->TryGetStringField(TEXT("id"), Hit.Id);
		(*Object)->TryGetStringField(TEXT("text"), Hit.Text);

		double Number = 0.0;
		if (ReadNumber(*Object, TEXT("distance"), Number))    { Hit.Distance   = Number; }
		if (ReadNumber(*Object, TEXT("bm25_score"), Number))  { Hit.Bm25Score  = Number; }
		if (ReadNumber(*Object, TEXT("rank_dense"), Number))  { Hit.RankDense  = static_cast<int32>(Number); }
		if (ReadNumber(*Object, TEXT("rank_sparse"), Number)) { Hit.RankSparse = static_cast<int32>(Number); }

		Hits.Add(MoveTemp(Hit));
	}
	return Hits;
}

FString FTryllDialogLabRetrieval::FormatHeader(
	int32 ResultCount, const FString& RetrievalMode, double Threshold, int32 RrfK)
{
	const FString Mode = ModeOr(RetrievalMode);
	FString Out = FString::Printf(TEXT("%d results · %s"), ResultCount, *Mode);
	if (Mode.Equals(TEXT("Hybrid"), ESearchCase::IgnoreCase))
	{
		Out += FString::Printf(TEXT(" · rrf_k %d"), RrfK);
	}
	Out += FString::Printf(TEXT(" · threshold %.2f"), Threshold);
	return Out;
}

FString FTryllDialogLabRetrieval::FormatEmptyMessage(const FString& RetrievalMode)
{
	const FString Mode = ModeOr(RetrievalMode);
	if (Mode.Equals(TEXT("Lexical"), ESearchCase::IgnoreCase))
	{
		return TEXT("No BM25 hits → not_found exit.");
	}
	if (Mode.Equals(TEXT("Hybrid"), ESearchCase::IgnoreCase))
	{
		return TEXT("No fused hits → not_found exit.");
	}
	return TEXT("No hit under threshold → not_found exit.");
}

FString FTryllDialogLabRetrieval::FormatHitMeta(const FTryllLabRetrievalHit& Hit)
{
	TArray<FString> Parts;
	if (Hit.Bm25Score.IsSet())
	{
		Parts.Add(TEXT("bm25 ") + TrimTrailingZeros(
			FString::Printf(TEXT("%.3f"), Hit.Bm25Score.GetValue())));
	}
	if (Hit.RankDense.IsSet())
	{
		Parts.Add(FString::Printf(TEXT("d#%d"), Hit.RankDense.GetValue()));
	}
	if (Hit.RankSparse.IsSet())
	{
		Parts.Add(FString::Printf(TEXT("s#%d"), Hit.RankSparse.GetValue()));
	}
	return FString::Join(Parts, TEXT(" · "));
}
