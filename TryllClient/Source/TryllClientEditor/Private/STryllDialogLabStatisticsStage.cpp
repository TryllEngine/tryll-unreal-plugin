// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// The Dialog Lab's Statistics view: STryllDialogLabPanel members kept apart from the panel's
// main file, which is long enough already. A port of Unity's TryllDialogLabWindow.BuildStatistics.

#include "STryllDialogLabPanel.h"

#include "TryllDialogLabChecks.h"
#include "TryllDialogLabDataset.h"
#include "TryllDialogLabRunData.h"
#include "TryllDialogLabStatistics.h"
#include "UI/TryllEditorStyle.h"

#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "TryllDialogLab"

namespace
{
	constexpr float StatsNameWidth  = 170.0f;
	constexpr float StatsValueWidth = 210.0f;
	constexpr float StatsTrackHeight = 14.0f;
	constexpr float StatsColumnWidth = 76.0f;

	/** A solid block; inside a Fill slot it stretches to the slot. */
	TSharedRef<SWidget> StatsSolid(const FLinearColor& Color, const float Width, const float Height)
	{
		return SNew(SColorBlock).Color(Color).Size(FVector2D(Width, Height));
	}

	/**
	 * Places Content across [From, To) of the track's width, Top px down: the Slate stand-in for
	 * an absolutely positioned element with percentage left and width.
	 */
	TSharedRef<SWidget> StatsSpan(const float From, const float To, const float Top, TSharedRef<SWidget> Content)
	{
		const float A = FMath::Clamp(From, 0.0f, 1.0f);
		const float B = FMath::Clamp(To, A, 1.0f);
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		if (A > 0.0f)
		{
			Row->AddSlot().FillWidth(A)[SNew(SSpacer)];
		}
		if (B > A)
		{
			Row->AddSlot().FillWidth(B - A)[Content];
		}
		if (B < 1.0f)
		{
			Row->AddSlot().FillWidth(1.0f - B)[SNew(SSpacer)];
		}
		return SNew(SBox).Padding(FMargin(0.0f, Top, 0.0f, 0.0f)).VAlign(VAlign_Top)[Row];
	}

	/** A fixed-width mark (tick, dot) whose left edge sits at At of the track's width. */
	TSharedRef<SWidget> StatsMark(const float At, const float Top, TSharedRef<SWidget> Content)
	{
		const float X = FMath::Clamp(At, 0.0f, 1.0f);
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		if (X > 0.0f)
		{
			Row->AddSlot().FillWidth(X)[SNew(SSpacer)];
		}
		Row->AddSlot().AutoWidth()[Content];
		if (X < 1.0f)
		{
			Row->AddSlot().FillWidth(1.0f - X)[SNew(SSpacer)];
		}
		return SNew(SBox).Padding(FMargin(0.0f, Top, 0.0f, 0.0f)).VAlign(VAlign_Top)[Row];
	}

	/** An interval: a 2px line from Low to High with 10px end caps. */
	TSharedRef<SWidget> StatsWhisker(const float Low, const float High, const FLinearColor& Color)
	{
		return StatsSpan(Low, High, 2.0f,
			SNew(SOverlay)
			+ SOverlay::Slot().VAlign(VAlign_Top).Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				StatsSolid(Color, 1.0f, 2.0f)
			]
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top)[StatsSolid(Color, 2.0f, 10.0f)]
			+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top)[StatsSolid(Color, 2.0f, 10.0f)]);
	}

	TSharedRef<SWidget> StatsHint(const FString& Text)
	{
		return SNew(STextBlock)
			.Text(FText::FromString(Text))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			.AutoWrapText(true);
	}

	TSharedRef<SWidget> StatsSectionTitle(const FString& Title, const FString& Hint)
	{
		return SNew(SBox).Padding(FMargin(0.0f, 14.0f, 0.0f, 2.0f))
		[
			SNew(SWrapBox).UseAllottedSize(true)
			+ SWrapBox::Slot().Padding(0.0f, 0.0f, 8.0f, 0.0f).VAlign(VAlign_Bottom)
			[
				SNew(STextBlock)
					.Text(FText::FromString(Title))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
			]
			+ SWrapBox::Slot().VAlign(VAlign_Bottom)
			[
				StatsHint(Hint)
			]
		];
	}

	/** "–" for no data, else the value rounded to a whole number. */
	FString StatsWhole(const TOptional<double>& Value)
	{
		return Value.IsSet() ? FString::Printf(TEXT("%.0f"), Value.GetValue()) : FString(TEXT("–"));
	}

	/** A table body row: a hairline under it so the eye can follow a turn across the columns. */
	TSharedRef<SWidget> StatsTableRow(TSharedRef<SHorizontalBox> Content)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).MinDesiredHeight(21.0f)[Content]]
			+ SVerticalBox::Slot().AutoHeight()[StatsSolid(FTryllEditorStyle::Line, 1.0f, 1.0f)];
	}

	/** A failed-of-total cell with a column rule on its left; its tint grows with the failed share. */
	TSharedRef<SWidget> StatsRatioCell(const int32 Failed, const int32 Of, const FString& Tooltip)
	{
		const float Share = Of > 0 ? static_cast<float>(Failed) / Of : 0.0f;
		const FLinearColor Fill = Failed > 0
			? FTryllEditorStyle::Wash(FTryllEditorStyle::ColWarn, 0.15f + 0.6f * Share)
			: FLinearColor::Transparent;
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[StatsSolid(FTryllEditorStyle::Line, 1.0f, 21.0f)]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(StatsColumnWidth - 1.0f)
				[
					SNew(SBorder)
						.BorderImage(FTryllEditorStyle::Rounded(0.0f))
						.BorderBackgroundColor(FSlateColor(Fill))
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.ToolTipText(FText::FromString(Tooltip))
						[
							SNew(STextBlock)
								.Text(FText::FromString(Failed > 0 ? FString::Printf(TEXT("%d/%d"), Failed, Of) : FString(TEXT("·"))))
								.ColorAndOpacity(FSlateColor(Failed > 0 ? FTryllEditorStyle::Txt : FTryllEditorStyle::Faint))
						]
				]
			];
	}

	/** A copy of a partial run keeping only the dialogues already shown, so the statistics
	 *  never count turns of a dialogue that is still running. */
	FTryllDialogLabRunResult StatsTrimToShown(const FTryllDialogLabRunResult& Run, const int32 Shown)
	{
		FTryllDialogLabRunResult Out;
		Out.Snapshot = Run.Snapshot;
		for (const FTryllLabSampleResult& Sample : Run.Samples)
		{
			FTryllLabSampleResult Copy = Sample;
			Copy.Turns.RemoveAll([Shown](const FTryllLabTurnRecord& Turn) { return Turn.DialogueIndex >= Shown; });
			Out.Samples.Add(MoveTemp(Copy));
		}
		return Out;
	}
}

void STryllDialogLabPanel::BuildStatisticsStage()
{
	const FTryllDialogLabRunResult* Run = CompletedRun.IsSet() ? &CompletedRun.GetValue()
		: PartialRun.IsSet() ? &PartialRun.GetValue() : nullptr;
	if (Run == nullptr)
	{
		return;
	}
	const TOptional<int32> Filter = Dataset->Dialogues.IsValidIndex(StatsDialogue)
		? TOptional<int32>(StatsDialogue)
		: TOptional<int32>();
	// While a run is still publishing, count only the dialogues already shown.
	const FTryllDialogLabStatistics Stats = Dataset->Dialogues.Num() < Dataset->TotalDialogues
		? FTryllDialogLabStatistics::Compute(StatsTrimToShown(*Run, Dataset->Dialogues.Num()), Filter)
		: FTryllDialogLabStatistics::Compute(*Run, Filter);
	const bool bIntervals = bStatsIntervals && Stats.IntervalsAvailable();
	const bool bDelta     = bStatsDelta && Stats.Variants.Num() > 1;

	TSharedRef<SScrollBox> Vertical = MakeVerticalScroller();
	TSharedRef<SVerticalBox> Wrap = SNew(SVerticalBox);
	Vertical->AddSlot().Padding(FMargin(10.0f, 8.0f, 10.0f, 24.0f))[Wrap];

	// Controls: dialogue filter, absolute vs baseline, intervals.
	TSharedRef<SWrapBox> Controls = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(8.0f, 4.0f));
	if (Dataset->Dialogues.Num() > 1)
	{
		TArray<FText> Names;
		Names.Add(FText::FromString(FString::Printf(TEXT("All (%d)"), Dataset->Dialogues.Num())));
		for (const FTryllLabDialogueView& Dialogue : Dataset->Dialogues)
		{
			Names.Add(FText::FromString(Dialogue.Name));
		}
		Controls->AddSlot().VAlign(VAlign_Center)
		[
			BuildSegmented(Names,
				[this] { return StatsDialogue + 1; },
				[this](int32 Index) { StatsDialogue = Index - 1; RebuildStage(); },
				[] { return true; })
		];
	}
	if (Stats.Variants.Num() > 1)
	{
		Controls->AddSlot().VAlign(VAlign_Center)
		[
			BuildSegmented(
				{LOCTEXT("StatsAbsolute", "Absolute"),
				 FText::FromString(TEXT("vs ") + Stats.Variants[0].Label)},
				[this] { return bStatsDelta ? 1 : 0; },
				[this](int32 Index) { bStatsDelta = Index == 1; RebuildStage(); },
				[] { return true; })
		];
	}
	const bool bAvailable = Stats.IntervalsAvailable();
	Controls->AddSlot().VAlign(VAlign_Center)
	[
		SNew(SBox)
			.ToolTipText(bAvailable
				? LOCTEXT("IntervalTip", "Computed across seeds: each seed's rate is one sample.")
				: FText::FromString(FString::Printf(TEXT("Needs at least %d seeds per variant (Repeats)."),
					FTryllDialogLabStatistics::MinSeedsForInterval)))
			[
				BuildSegmented({LOCTEXT("StatsInterval", "95% interval")},
					[bIntervals] { return bIntervals ? 0 : -1; },
					[this](int32) { bStatsIntervals = !bStatsIntervals; RebuildStage(); },
					[bAvailable] { return bAvailable; })
			]
	];
	if (!bAvailable)
	{
		Controls->AddSlot().VAlign(VAlign_Center)
		[
			StatsHint(FString::Printf(TEXT("Intervals need %d+ seeds per variant; this run has %d."),
				FTryllDialogLabStatistics::MinSeedsForInterval, Stats.MinSeeds()))
		];
	}
	Wrap->AddSlot().AutoHeight()[Controls];

	Wrap->AddSlot().AutoHeight()
	[
		BuildStatsMetric(Stats, TEXT("Turns passed"), TEXT("generated turns with every check passing"),
			[](const FTryllLabVariantStats& V) -> const FTryllLabRate& { return V.Turns; }, bIntervals, bDelta)
	];
	Wrap->AddSlot().AutoHeight()
	[
		BuildStatsMetric(Stats, TEXT("Checks passed"), TEXT("authored checks on generated turns"),
			[](const FTryllLabVariantStats& V) -> const FTryllLabRate& { return V.Checks; }, bIntervals, bDelta)
	];
	Wrap->AddSlot().AutoHeight()
	[
		BuildStatsMetric(Stats, TEXT("Clean dialogues"), TEXT("dialogue × seed runs with no failed turn"),
			[](const FTryllLabVariantStats& V) -> const FTryllLabRate& { return V.CleanDialogues; }, bIntervals, bDelta)
	];

	TArray<FString> Shape;
	for (const FTryllLabVariantStats& V : Stats.Variants)
	{
		Shape.Add(FString::Printf(TEXT("%s: %d error, %d not run, %s words median (p90 %s), TTFT %s ms (p90 %s)"),
			*V.Label, V.Errors, V.NotRun, *StatsWhole(V.MedianWords), *StatsWhole(V.P90Words),
			*StatsWhole(V.MedianTtftMs), *StatsWhole(V.P90TtftMs)));
	}
	Wrap->AddSlot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
	[
		StatsHint(FString::Join(Shape, TEXT("   ·   ")))
	];

	Wrap->AddSlot().AutoHeight()
	[
		StatsSectionTitle(TEXT("Where turns fail"),
			TEXT("failed seeds / seeds per generated turn · click a cell to open it in the Grid"))
	];
	Wrap->AddSlot().AutoHeight()[BuildWhereTurnsFailTable(Stats)];
	Wrap->AddSlot().AutoHeight()[StatsSectionTitle(TEXT("Failing checks by type"), TEXT("failed / evaluated"))];
	Wrap->AddSlot().AutoHeight()[BuildCheckTypeTable(Stats)];

	StageHost->AddSlot().FillHeight(1.0f)[Vertical];
}

TSharedRef<SWidget> STryllDialogLabPanel::BuildStatsMetric(const FTryllDialogLabStatistics& Stats,
	const FString& Title, const FString& Hint,
	TFunction<const FTryllLabRate&(const FTryllLabVariantStats&)> Pick,
	const bool bIntervals, const bool bDelta)
{
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	Box->AddSlot().AutoHeight()
	[
		StatsSectionTitle(Title, bDelta ? TEXT("points vs ") + Stats.Variants[0].Label : Hint)
	];

	for (int32 VI = 0; VI < Stats.Variants.Num(); ++VI)
	{
		if (bDelta && VI == 0)
		{
			continue;
		}
		const FTryllLabVariantStats& V = Stats.Variants[VI];
		const FTryllLabRate& Rate = Pick(V);

		TSharedRef<SOverlay> Track = SNew(SOverlay);
		FString Value;
		FString Tooltip;
		FLinearColor ValueColor = FTryllEditorStyle::Txt;
		if (!bDelta)
		{
			for (const float Gridline : {0.25f, 0.5f, 0.75f})
			{
				Track->AddSlot()[StatsMark(Gridline, 0.0f, StatsSolid(FTryllEditorStyle::Line, 1.0f, StatsTrackHeight))];
			}
			Track->AddSlot()
			[
				StatsSpan(0.0f, static_cast<float>(Rate.Rate()), 3.0f,
					SNew(SBorder)
						.BorderImage(FTryllEditorStyle::Rounded(FTryllEditorStyle::RadiusBar))
						.BorderBackgroundColor(FSlateColor(V.Swatch))
						.Padding(0.0f)
						[
							SNew(SBox).HeightOverride(8.0f)
						])
			];
			if (bIntervals)
			{
				Track->AddSlot()[StatsWhisker(static_cast<float>(Rate.Low()), static_cast<float>(Rate.High()), FTryllEditorStyle::Txt)];
			}
			Value = Rate.Total == 0
				? FString(TEXT("no data"))
				: FString::Printf(TEXT("%.1f%% · %d/%d"), Rate.Rate() * 100.0, Rate.Passed, Rate.Total)
					+ (bIntervals ? FString::Printf(TEXT(" ± %.1f"), Rate.HalfWidth() * 100.0) : FString());
			Tooltip = FString::Printf(TEXT("%s — %s: %d of %d"), *V.Label, *Title, Rate.Passed, Rate.Total)
				+ (bIntervals ? FString::Printf(TEXT(", 95%% interval %.1f–%.1f%%"), Rate.Low() * 100.0, Rate.High() * 100.0) : FString())
				+ FString::Printf(TEXT(" over %d seeds"), V.Seeds);
		}
		else
		{
			constexpr double Range = 0.4; // +/- 40 points
			const TPair<double, double> Difference = Stats.DifferenceFromBaseline(
				[&Pick](const FTryllLabVariantStats& S) -> const FTryllLabRate& { return Pick(S); }, VI);
			const double Diff = Difference.Key;
			const double Half = Difference.Value;
			auto X = [Range](const double D) { return static_cast<float>(0.5 + FMath::Clamp(D, -Range, Range) / Range * 0.5); };
			Track->AddSlot()[StatsMark(0.5f, 0.0f, StatsSolid(FTryllEditorStyle::Dim, 1.0f, StatsTrackHeight))];
			if (bIntervals)
			{
				Track->AddSlot()[StatsWhisker(X(Diff - Half), X(Diff + Half), V.Swatch)];
			}
			Track->AddSlot()
			[
				StatsMark(X(Diff), 2.0f,
					SNew(SBorder)
						.BorderImage(FTryllEditorStyle::Rounded(5.0f))
						.BorderBackgroundColor(FSlateColor(V.Swatch))
						.Padding(0.0f)
						[
							SNew(SBox).WidthOverride(10.0f).HeightOverride(10.0f)
						])
			];
			const bool bClear = bIntervals && (Diff - Half > 0.0 || Diff + Half < 0.0);
			const TCHAR* Verdict = !bIntervals ? TEXT("")
				: bClear ? (Diff > 0.0 ? TEXT(" · higher") : TEXT(" · lower"))
				: TEXT(" · no clear difference");
			Value = FString::Printf(TEXT("%s%.1f pts"), Diff >= 0.0 ? TEXT("+") : TEXT(""), Diff * 100.0)
				+ (bIntervals ? FString::Printf(TEXT(" ± %.1f"), Half * 100.0) : FString())
				+ Verdict;
			ValueColor = bClear ? FTryllEditorStyle::Txt : FTryllEditorStyle::Dim;
			Tooltip = FString::Printf(TEXT("%s minus %s: %.1f points"), *V.Label, *Stats.Variants[0].Label, Diff * 100.0);
		}

		Box->AddSlot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
		[
			SNew(SBox).ToolTipText(FText::FromString(Tooltip))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(StatsNameWidth)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
						[
							SNew(SColorBlock).Color(V.Swatch).Size(FVector2D(10.0f, 10.0f))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(STextBlock)
								.Text(FText::FromString(FString::Printf(TEXT("%s  (%d seeds)"), *V.Label, V.Seeds)))
								.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(6.0f, 0.0f)
				[
					SNew(SBox).HeightOverride(StatsTrackHeight).MinDesiredWidth(60.0f)[Track]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(StatsValueWidth).HAlign(HAlign_Right)
					[
						SNew(STextBlock)
							.Text(FText::FromString(Value))
							.ColorAndOpacity(FSlateColor(ValueColor))
					]
				]
			]
		];
	}
	return Box;
}

TSharedRef<SWidget> STryllDialogLabPanel::BuildStatsTableHeader(const FTryllDialogLabStatistics& Stats)
{
	TSharedRef<SHorizontalBox> Head = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)];
	for (const FTryllLabVariantStats& V : Stats.Variants)
	{
		Head->AddSlot().AutoWidth()
		[
			SNew(SBox).WidthOverride(StatsColumnWidth)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock)
						.Text(FText::FromString(V.Label))
						.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
						.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
				]
				+ SVerticalBox::Slot().AutoHeight()[StatsSolid(V.Swatch, 1.0f, 2.0f)]
			]
		];
	}
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)[Head]
		+ SVerticalBox::Slot().AutoHeight()[StatsSolid(FTryllEditorStyle::Line2, 1.0f, 1.0f)];
}

TSharedRef<SWidget> STryllDialogLabPanel::BuildWhereTurnsFailTable(const FTryllDialogLabStatistics& Stats)
{
	TSharedRef<SVerticalBox> Table = SNew(SVerticalBox);
	Table->AddSlot().AutoHeight()[BuildStatsTableHeader(Stats)];
	int32 LastDialogue = -1;
	for (const FTryllLabTurnFailRow& Row : Stats.WhereTurnsFail)
	{
		if (Dataset->Dialogues.Num() > 1 && !Stats.Dialogue.IsSet() && Row.DialogueIndex != LastDialogue
			&& Dataset->Dialogues.IsValidIndex(Row.DialogueIndex))
		{
			Table->AddSlot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
			[
				StatsHint(Dataset->Dialogues[Row.DialogueIndex].Name.ToUpper())
			];
		}
		LastDialogue = Row.DialogueIndex;

		TSharedRef<SHorizontalBox> Line = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(4.0f, 0.0f)
			[
				SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("%d. %s"), Row.StepIndex + 1, *Row.User)))
					.ToolTipText(FText::FromString(Row.User))
					.ColorAndOpacity(FSlateColor(FTryllEditorStyle::SummaryUser))
					.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
			];
		for (int32 VI = 0; VI < Stats.Variants.Num(); ++VI)
		{
			const int32 Failed = Row.Failed[VI];
			const int32 Seeds  = Row.Seeds[VI];
			TSharedRef<SWidget> Cell = StatsRatioCell(Failed, Seeds,
				FString::Printf(TEXT("%s · turn %d: %d of %d seeds failed"),
					*Stats.Variants[VI].Label, Row.StepIndex + 1, Failed, Seeds));
			if (Failed > 0)
			{
				const int32 Flat = Row.FlatIndex;
				Cell = SNew(SBorder)
					.BorderImage(FTryllEditorStyle::Rounded(0.0f))
					.BorderBackgroundColor(FSlateColor(FLinearColor::Transparent))
					.Padding(0.0f)
					.Cursor(EMouseCursor::Hand)
					.OnMouseButtonDown_Lambda([this, Flat, VI](const FGeometry&, const FPointerEvent&)
					{
						JumpToTurn(Flat, VI);
						return FReply::Handled();
					})
					[
						Cell
					];
			}
			Line->AddSlot().AutoWidth()[Cell];
		}
		Table->AddSlot().AutoHeight()[StatsTableRow(Line)];
	}
	if (Stats.WhereTurnsFail.Num() == 0)
	{
		Table->AddSlot().AutoHeight()[StatsHint(TEXT("No generated turns."))];
	}
	return Table;
}

TSharedRef<SWidget> STryllDialogLabPanel::BuildCheckTypeTable(const FTryllDialogLabStatistics& Stats)
{
	TSharedRef<SVerticalBox> Table = SNew(SVerticalBox);
	Table->AddSlot().AutoHeight()[BuildStatsTableHeader(Stats)];
	for (const FTryllLabCheckTypeRow& Row : Stats.ByCheckType)
	{
		const FString Name = Row.CheckType == FTryllDialogLabChecks::GateUnexpectedToolCall
			? FString(TEXT("unexpected tool call (built-in)"))
			: Row.CheckType.Replace(TEXT("_"), TEXT(" "));
		TSharedRef<SHorizontalBox> Line = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(4.0f, 0.0f)
			[
				SNew(STextBlock).Text(FText::FromString(Name))
			];
		for (int32 VI = 0; VI < Stats.Variants.Num(); ++VI)
		{
			Line->AddSlot().AutoWidth()
			[
				StatsRatioCell(Row.Failed[VI], Row.Evaluated[VI],
					FString::Printf(TEXT("%s · %s: %d failed of %d"),
						*Stats.Variants[VI].Label, *Name, Row.Failed[VI], Row.Evaluated[VI]))
			];
		}
		Table->AddSlot().AutoHeight()[StatsTableRow(Line)];
	}
	if (Stats.ByCheckType.Num() == 0)
	{
		Table->AddSlot().AutoHeight()[StatsHint(TEXT("No checks ran."))];
	}
	return Table;
}

void STryllDialogLabPanel::JumpToTurn(const int32 FlatIndex, const int32 VariantIndex)
{
	if (!HasResults() || !Dataset->Turns.IsValidIndex(FlatIndex) || !Dataset->Variants.IsValidIndex(VariantIndex))
	{
		return;
	}
	ViewMode = EViewMode::Grid;
	CollapsedDialogues.Remove(Dataset->Turns[FlatIndex].DialogueIndex);
	JumpTurn = FlatIndex;
	const FString VariantId = Dataset->Variants[VariantIndex].Id;
	for (int32 Column = 0; Column < Dataset->Samples.Num(); ++Column)
	{
		const FTryllLabSampleView& Sample = Dataset->Samples[Column];
		if (Sample.VariantId != VariantId)
		{
			continue;
		}
		const FTryllLabCellView* Cell = Dataset->Cells.Find(FTryllDialogLabDataset::CellKey(Sample.Id, FlatIndex));
		if (Cell != nullptr && FTryllDialogLabDataset::StatusOf(*Cell) != ETryllLabCellStatus::Ok)
		{
			SelectedKey = Cell->Key;
			// Bring the column into view too, when it sits outside the current one. The stage keeps
			// its width across the rebuild, so the outgoing scroller's width is the one to test.
			const float ColumnX = Column * (CellWidth + FTryllEditorStyle::Gap);
			const float View = VerticalScroller.IsValid()
				? VerticalScroller->GetCachedGeometry().GetLocalSize().X - 20.0f // row padding
				: 0.0f;
			if (View > 0.0f && (ColumnX < HorizontalOffset || ColumnX + CellWidth > HorizontalOffset + View))
			{
				HorizontalOffset = ColumnX;
			}
			break;
		}
	}
	RebuildStage();
}

#undef LOCTEXT_NAMESPACE
