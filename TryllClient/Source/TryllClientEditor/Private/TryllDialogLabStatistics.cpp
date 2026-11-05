// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogLabStatistics.h"

#include "TryllDialogLabExport.h"
#include "TryllDialogLabRunData.h"

namespace
{
	const double StatsT95[] =
	{
		0.0, 12.706, 4.303, 3.182, 2.776, 2.571, 2.447, 2.365, 2.306, 2.262, 2.228,
		2.201, 2.179, 2.160, 2.145, 2.131, 2.120, 2.110, 2.101, 2.093, 2.086,
		2.080, 2.074, 2.069, 2.064, 2.060, 2.056, 2.052, 2.048, 2.045, 2.042,
	};

	void StatsAdd(FTryllLabRate& Rate, const int32 Passed, const int32 Total)
	{
		Rate.Passed += Passed;
		Rate.Total  += Total;
		if (Total > 0)
		{
			Rate.PerSeed.Add(static_cast<double>(Passed) / Total);
		}
	}

	/** Nearest-rank percentile; unset for no data. */
	TOptional<double> StatsPercentile(TArray<double> Values, const double P)
	{
		if (Values.Num() == 0)
		{
			return TOptional<double>();
		}
		Values.Sort();
		const int32 Rank = static_cast<int32>(FMath::CeilToDouble(P * Values.Num()));
		return Values[FMath::Clamp(Rank - 1, 0, Values.Num() - 1)];
	}
}

double FTryllLabRate::StandardError() const
{
	const int32 N = PerSeed.Num();
	if (N < 2)
	{
		return 0.0;
	}
	double Mean = 0.0;
	for (const double X : PerSeed) { Mean += X; }
	Mean /= N;
	double Var = 0.0;
	for (const double X : PerSeed) { Var += (X - Mean) * (X - Mean); }
	Var /= (N - 1);
	return FMath::Sqrt(Var / N);
}

double FTryllLabRate::HalfWidth() const
{
	return FTryllDialogLabStatistics::TCritical95(PerSeed.Num() - 1) * StandardError();
}

int32 FTryllDialogLabStatistics::MinSeeds() const
{
	if (Variants.Num() == 0)
	{
		return 0;
	}
	int32 Min = MAX_int32;
	for (const FTryllLabVariantStats& Variant : Variants)
	{
		Min = FMath::Min(Min, Variant.Seeds);
	}
	return Min;
}

double FTryllDialogLabStatistics::TCritical95(const int32 Df)
{
	if (Df < 1)
	{
		return 0.0;
	}
	return Df < static_cast<int32>(UE_ARRAY_COUNT(StatsT95)) ? StatsT95[Df] : 1.96 + 2.4 / Df;
}

FTryllDialogLabStatistics FTryllDialogLabStatistics::Compute(
	const FTryllDialogLabRunResult& Run, const TOptional<int32> InDialogue)
{
	FTryllDialogLabStatistics Stats;
	Stats.Dialogue = InDialogue;

	auto InFilter = [&InDialogue](const int32 D) { return !InDialogue.IsSet() || InDialogue.GetValue() == D; };
	const int32 VariantCount = Run.Snapshot.Variants.Num();
	TArray<TArray<const FTryllLabSampleResult*>> ByVariant;
	ByVariant.SetNum(VariantCount);
	for (const FTryllLabSampleResult& Sample : Run.Samples)
	{
		if (ByVariant.IsValidIndex(Sample.VariantIndex))
		{
			ByVariant[Sample.VariantIndex].Add(&Sample);
		}
	}

	for (int32 VI = 0; VI < VariantCount; ++VI)
	{
		const FTryllLabVariantSnapshot& Variant = Run.Snapshot.Variants[VI];
		FTryllLabVariantStats VS;
		VS.VariantIndex = VI;
		VS.Label        = Variant.Label;
		VS.Swatch       = Variant.Swatch;
		VS.Seeds        = ByVariant[VI].Num();
		TArray<double> Words;
		TArray<double> Ttft;

		for (const FTryllLabSampleResult* Sample : ByVariant[VI])
		{
			int32 TK = 0, TN = 0, CK = 0, CN = 0, DK = 0, DN = 0;
			// Group by dialogue in first-seen order, as Unity's GroupBy does.
			TArray<int32> Order;
			TMap<int32, TArray<const FTryllLabTurnRecord*>> Groups;
			for (const FTryllLabTurnRecord& Turn : Sample->Turns)
			{
				if (!InFilter(Turn.DialogueIndex))
				{
					continue;
				}
				if (!Groups.Contains(Turn.DialogueIndex))
				{
					Order.Add(Turn.DialogueIndex);
				}
				Groups.FindOrAdd(Turn.DialogueIndex).Add(&Turn);
			}
			for (const int32 D : Order)
			{
				bool bAny = false;
				bool bClean = true;
				for (const FTryllLabTurnRecord* Turn : Groups[D])
				{
					if (Turn->bSeeded)
					{
						continue;
					}
					bAny = true;
					++TN;
					if (Turn->Status == ETryllLabCellStatus::Ok) { ++TK; }
					else                                         { bClean = false; }
					if (Turn->Status == ETryllLabCellStatus::Error)   { ++VS.Errors; }
					if (Turn->Status == ETryllLabCellStatus::Skipped) { ++VS.NotRun; }
					for (const FTryllLabCheckOutcome& Outcome : Turn->CheckOutcomes)
					{
						if (Outcome.Scope == ETryllLabCheckScope::Gate)
						{
							continue;
						}
						++CN;
						if (Outcome.bPassed) { ++CK; }
					}
					if (Turn->Status == ETryllLabCellStatus::Ok || Turn->Status == ETryllLabCellStatus::CheckFail)
					{
						Words.Add(Turn->Words);
						if (Turn->TtftMs > 0)
						{
							Ttft.Add(Turn->TtftMs);
						}
					}
				}
				if (!bAny)
				{
					continue;
				}
				++DN;
				if (bClean) { ++DK; }
			}
			StatsAdd(VS.Turns, TK, TN);
			StatsAdd(VS.Checks, CK, CN);
			StatsAdd(VS.CleanDialogues, DK, DN);
		}

		VS.MedianWords  = StatsPercentile(Words, 0.5);
		VS.P90Words     = StatsPercentile(Words, 0.9);
		VS.MedianTtftMs = StatsPercentile(Ttft, 0.5);
		VS.P90TtftMs    = StatsPercentile(Ttft, 0.9);
		Stats.Variants.Add(MoveTemp(VS));
	}

	// Where turns fail: one row per generated turn in the filter, read from any sample (every
	// sample has the same rectangular turn list).
	if (Run.Samples.Num() > 0)
	{
		const FTryllLabSampleResult& Template = Run.Samples[0];
		for (int32 G = 0; G < Template.Turns.Num(); ++G)
		{
			const FTryllLabTurnRecord& Turn = Template.Turns[G];
			if (Turn.bSeeded || !InFilter(Turn.DialogueIndex))
			{
				continue;
			}
			FTryllLabTurnFailRow Row;
			Row.DialogueIndex = Turn.DialogueIndex;
			Row.StepIndex     = Turn.TurnIndex;
			Row.FlatIndex     = G;
			Row.User          = Turn.UserText;
			Row.Failed.SetNumZeroed(VariantCount);
			Row.Seeds.SetNumZeroed(VariantCount);
			for (int32 VI = 0; VI < VariantCount; ++VI)
			{
				for (const FTryllLabSampleResult* Sample : ByVariant[VI])
				{
					if (G >= Sample->Turns.Num())
					{
						continue;
					}
					++Row.Seeds[VI];
					if (Sample->Turns[G].Status != ETryllLabCellStatus::Ok)
					{
						++Row.Failed[VI];
					}
				}
			}
			Stats.WhereTurnsFail.Add(MoveTemp(Row));
		}
	}

	// Failing checks by type, the gate included: it is the one failure that has no authored row,
	// so this is the only place its count shows.
	TMap<FString, FTryllLabCheckTypeRow> Types;
	for (int32 VI = 0; VI < VariantCount; ++VI)
	{
		for (const FTryllLabSampleResult* Sample : ByVariant[VI])
		{
			for (const FTryllLabTurnRecord& Turn : Sample->Turns)
			{
				if (Turn.bSeeded || !InFilter(Turn.DialogueIndex))
				{
					continue;
				}
				for (const FTryllLabCheckOutcome& Outcome : Turn.CheckOutcomes)
				{
					const FString Type = Outcome.CheckType.IsEmpty() ? Outcome.Kind : Outcome.CheckType;
					FTryllLabCheckTypeRow* Row = Types.Find(Type);
					if (Row == nullptr)
					{
						Row = &Types.Add(Type);
						Row->CheckType = Type;
						Row->Failed.SetNumZeroed(VariantCount);
						Row->Evaluated.SetNumZeroed(VariantCount);
					}
					++Row->Evaluated[VI];
					if (!Outcome.bPassed)
					{
						++Row->Failed[VI];
					}
				}
			}
		}
	}
	Types.GenerateValueArray(Stats.ByCheckType);
	Stats.ByCheckType.Sort([](const FTryllLabCheckTypeRow& A, const FTryllLabCheckTypeRow& B)
	{
		return A.CheckType.Compare(B.CheckType, ESearchCase::CaseSensitive) < 0;
	});
	return Stats;
}

TPair<double, double> FTryllDialogLabStatistics::DifferenceFromBaseline(
	TFunctionRef<const FTryllLabRate&(const FTryllLabVariantStats&)> Metric, const int32 VariantIndex) const
{
	const FTryllLabRate& A = Metric(Variants[VariantIndex]);
	const FTryllLabRate& B = Metric(Variants[0]);
	const double SeA = A.StandardError();
	const double SeB = B.StandardError();
	const double Se  = FMath::Sqrt(SeA * SeA + SeB * SeB);
	const int32 Df   = FMath::Min(A.PerSeed.Num(), B.PerSeed.Num()) - 1;
	return TPair<double, double>(A.Rate() - B.Rate(), TCritical95(Df) * Se);
}

FString FTryllDialogLabStatistics::FormatNumber(const double Value)
{
	FString Out = FString::Printf(TEXT("%.4f"), Value);
	int32 Dot = INDEX_NONE;
	if (Out.FindChar(TEXT('.'), Dot))
	{
		int32 End = Out.Len();
		while (End > Dot + 1 && Out[End - 1] == TEXT('0'))
		{
			--End;
		}
		if (End == Dot + 1)
		{
			End = Dot;
		}
		Out.LeftInline(End);
	}
	if (Out == TEXT("-0"))
	{
		Out = TEXT("0");
	}
	return Out;
}

FString FTryllDialogLabStatistics::ToJson() const
{
	const bool bIntervals = IntervalsAvailable();
	auto Opt = [](const TOptional<double>& X) { return X.IsSet() ? FormatNumber(X.GetValue()) : FString(TEXT("null")); };
	auto RateJson = [bIntervals](const FTryllLabRate& R)
	{
		FString Out = FString::Printf(TEXT("{\"passed\":%d,\"total\":%d,\"rate\":%s"),
			R.Passed, R.Total, *FormatNumber(R.Rate()));
		if (bIntervals)
		{
			Out += FString::Printf(TEXT(",\"ci95\":[%s,%s]"), *FormatNumber(R.Low()), *FormatNumber(R.High()));
		}
		return Out + TEXT("}");
	};

	TArray<FString> Items;
	for (const FTryllLabVariantStats& V : Variants)
	{
		Items.Add(FString::Printf(
			TEXT("{\"index\":%d,\"label\":\"%s\",\"seeds\":%d,")
			TEXT("\"turns_passed\":%s,\"checks_passed\":%s,\"clean_dialogues\":%s,")
			TEXT("\"errors\":%d,\"not_run\":%d,")
			TEXT("\"words_median\":%s,\"words_p90\":%s,\"ttft_ms_median\":%s,\"ttft_ms_p90\":%s}"),
			V.VariantIndex, *FTryllDialogLabExport::EscapeJson(V.Label), V.Seeds,
			*RateJson(V.Turns), *RateJson(V.Checks), *RateJson(V.CleanDialogues),
			V.Errors, V.NotRun,
			*Opt(V.MedianWords), *Opt(V.P90Words), *Opt(V.MedianTtftMs), *Opt(V.P90TtftMs)));
	}
	return FString::Printf(
		TEXT("{\"interval\":\"t95 over per-seed rates\",\"min_seeds_for_interval\":%d,\"variants\":[%s]}"),
		MinSeedsForInterval, *FString::Join(Items, TEXT(",")));
}
