// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "STryllTurnInspectorView.h"

#include "TryllDialogLabChecks.h"
#include "TryllTurnInspectorModels.h"
#include "TryllDialogLabIntent.h"
#include "TryllDialogLabRetrieval.h"
#include "TryllLabJsonRender.h"
#include "UI/TryllEditorStyle.h"
#include "TryllEditorUiContract.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "TryllTurnInspector"

namespace
{
	/** The node object of the first route entry of this type, or null. */
	const TSharedPtr<FJsonObject>* FindNodeOfType(
		const TSharedPtr<FJsonObject>& DebugInfo, const TCHAR* Type)
	{
		if (!DebugInfo.IsValid())
		{
			return nullptr;
		}
		const TArray<TSharedPtr<FJsonValue>>* Nodes = nullptr;
		if (!DebugInfo->TryGetArrayField(TEXT("nodes"), Nodes) || Nodes == nullptr)
		{
			return nullptr;
		}
		for (const TSharedPtr<FJsonValue>& Entry : *Nodes)
		{
			const TSharedPtr<FJsonObject>* Node = nullptr;
			if (!Entry.IsValid() || !Entry->TryGetObject(Node) || Node == nullptr)
			{
				continue;
			}
			FString NodeType;
			if ((*Node)->TryGetStringField(TEXT("_type"), NodeType) && NodeType == Type)
			{
				return Node;
			}
		}
		return nullptr;
	}

	const TSharedPtr<FJsonObject>* GetObject(
		const TSharedPtr<FJsonObject>& Parent, const TCHAR* Field)
	{
		if (!Parent.IsValid())
		{
			return nullptr;
		}
		const TSharedPtr<FJsonObject>* Out = nullptr;
		return Parent->TryGetObjectField(Field, Out) ? Out : nullptr;
	}

	TArray<TSharedPtr<FJsonValue>> GetArray(
		const TSharedPtr<FJsonObject>& Parent, const TCHAR* Field)
	{
		if (!Parent.IsValid())
		{
			return {};
		}
		const TArray<TSharedPtr<FJsonValue>>* Out = nullptr;
		return Parent->TryGetArrayField(Field, Out) && Out != nullptr ? *Out : TArray<TSharedPtr<FJsonValue>>();
	}

	FString ComponentType(const TSharedPtr<FJsonValue>& Value)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (!Value.IsValid() || !Value->TryGetObject(Object) || Object == nullptr)
		{
			return FString();
		}
		FString Type;
		(*Object)->TryGetStringField(TEXT("_type"), Type);
		return Type;
	}

	/**
	 * An array value rendered as the bare count "[n]" is useless for a ToolCall node's `tools`
	 * signatures, which is the whole point of that key. Stack the elements down the value column
	 * instead; the label already wraps, so long entries behave like any other value.
	 */
	FString ParameterValueText(const TSharedPtr<FJsonValue>& Value)
	{
		if (Value.IsValid() && Value->Type == EJson::Array)
		{
			TArray<FString> Parts;
			for (const TSharedPtr<FJsonValue>& Element : Value->AsArray())
			{
				Parts.Add(FTryllLabJsonRender::JsonScalarToString(Element));
			}
			return FString::Join(Parts, TEXT("\n"));
		}
		return FTryllLabJsonRender::JsonScalarToString(Value);
	}

	FString StatusName(ETryllTurnTerminalStatus Status)
	{
		switch (Status)
		{
			case ETryllTurnTerminalStatus::Success:   return TEXT("Success");
			case ETryllTurnTerminalStatus::Cancelled: return TEXT("Cancelled");
			case ETryllTurnTerminalStatus::Error:     return TEXT("Error");
			default:                                  return TEXT("Skipped");
		}
	}

	TSharedRef<SWidget> IntentChip(const FString& Text, bool bOk)
	{
		return SNew(SBorder)
			.BorderImage(FTryllEditorStyle::Rounded(
				FTryllEditorStyle::RadiusChip,
				bOk ? FTryllEditorStyle::OkBorder : FTryllEditorStyle::WarnBorder, 1.0f))
			.BorderBackgroundColor(FSlateColor(bOk ? FTryllEditorStyle::OkBg : FTryllEditorStyle::WarnBg))
			.Padding(FMargin(5.0f, 0.0f, 5.0f, 1.0f))
			[
				SNew(STextBlock)
					.Text(FText::FromString(Text))
					.ColorAndOpacity(FSlateColor(bOk ? FTryllEditorStyle::ColOk : FTryllEditorStyle::ColWarn))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			];
	}

	TSharedRef<SWidget> IntentProbBar(
		const FString& Label, double Prob, double Max, TOptional<double> Threshold)
	{
		const float Frac = Max > 0.0 ? static_cast<float>(Prob / Max) : 0.0f;
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		Row->AddSlot().AutoWidth()
		[
			SNew(SBox).WidthOverride(88.0f)
			[
				SNew(STextBlock)
					.Text(FText::FromString(Label.IsEmpty() ? FString(TEXT("—")) : Label))
					.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			]
		];
		const float ThresholdFrac = Threshold.IsSet() && Max > 0.0
			? FMath::Clamp(static_cast<float>(Threshold.GetValue() / Max), 0.0f, 1.0f)
			: -1.0f;
		TSharedRef<SWidget> BarFill =
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(Frac <= 0.0f ? 0.0001f : Frac)
			[
				SNew(SBorder)
					.BorderImage(FTryllEditorStyle::Rounded(2.0f))
					.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::Accent))
					.Padding(0.0f)
			]
			+ SHorizontalBox::Slot().FillWidth(Frac >= 1.0f ? 0.0001f : (1.0f - Frac))
			[
				SNew(SBorder)
					.BorderImage(FTryllEditorStyle::Rounded(2.0f))
					.BorderBackgroundColor(FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.22f)))
					.Padding(0.0f)
			];
		TSharedRef<SWidget> Track = ThresholdFrac < 0.0f
			? BarFill
			: StaticCastSharedRef<SWidget>(
				SNew(SOverlay)
				+ SOverlay::Slot()[BarFill]
				+ SOverlay::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(ThresholdFrac <= 0.0f ? 0.0001f : ThresholdFrac)
					[SNew(SSpacer)]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(2.0f)
						[
							SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
								.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::ColWarn))
								.Padding(0.0f)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(
						ThresholdFrac >= 1.0f ? 0.0001f : (1.0f - ThresholdFrac))
					[SNew(SSpacer)]
				]);
		Row->AddSlot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(4.0f, 0.0f)
		[
			SNew(SBox).HeightOverride(8.0f).MinDesiredWidth(40.0f)
			[
				Track
			]
		];
		if (Threshold.IsSet() && Max > 0.0)
		{
			Row->SetToolTipText(FText::FromString(FString::Printf(
				TEXT("threshold %.2f"), Threshold.GetValue())));
		}
		Row->AddSlot().AutoWidth().Padding(6.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SBox).WidthOverride(42.0f)
			[
				SNew(STextBlock)
					.Text(FText::FromString(FTryllDialogLabIntent::FormatProb(TOptional<double>(Prob))))
					.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Txt))
					.Justification(ETextJustify::Right)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			]
		];
		return Row;
	}
}

void STryllTurnInspectorView::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		// The canvas: the same value as the window, so the run-out below the last section reads as
		// the same surface the section bodies sit on rather than a lighter panel behind them.
		SNew(SBorder)
			.BorderImage(FTryllEditorStyle::Rounded(0.0f))
			.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::SectionBody))
			.Padding(0.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot().AutoHeight()
				[
					// The panel's own title strip, one step above the section headers.
					SNew(SBorder)
						.BorderImage(FTryllEditorStyle::Rounded(0.0f))
						.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::Panel2))
						.Padding(FMargin(10.0f, 6.0f, 10.0f, 7.0f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(STextBlock)
										.Text(LOCTEXT("Title", "Turn diagnostics"))
										.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Txt))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								]
								+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(8.0f, 0.0f, 0.0f, 0.0f)
								[
									SAssignNew(HintText, STextBlock)
										.Text(LOCTEXT("NoRun", "no run yet"))
										.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
							[
								SAssignNew(PinRow, SVerticalBox)
							]
						]
				]

				// Everything in this panel wraps; the horizontal scroller stays off so one
				// over-wide block cannot add a bar to the whole panel.
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					SAssignNew(Body, SScrollBox).Orientation(Orient_Vertical)
				]
			]
	];

	Render(FTryllTurnInspectionFactory::Empty());
}

void STryllTurnInspectorView::Render(const FTryllTurnInspection& InTurn)
{
	Turn = InTurn;

	if (ScopeKey != Turn.TurnKey)
	{
		NodeScope = -1;
		ScopeKey  = Turn.TurnKey;
	}

	RebuildInspector();
}

void STryllTurnInspectorView::RebuildInspector()
{
	if (!Body.IsValid() || !PinRow.IsValid())
	{
		return;
	}

	Body->ClearChildren();
	PinRow->ClearChildren();

	const FTryllTurnLabContext* Lab = Turn.Lab.IsSet() ? &Turn.Lab.GetValue() : nullptr;
	const bool bShowPins = Lab != nullptr;
	if (bShowPins)
	{
		PinRow->AddSlot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				BuildPinSlot(TEXT("A"), Lab->PinA, FTryllEditorStyle::Accent)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				BuildPinSlot(TEXT("B"), Lab->PinB, FTryllEditorStyle::ColWarn)
			]
		];
	}

	if (Turn.TurnKey.IsEmpty())
	{
		HintText->SetText(FText::FromString(Turn.Hint));
		const FText Placeholder = (Lab != nullptr && Lab->bShowLabEmptyPlaceholder)
			? FText::FromString(FTryllEditorUiContract::InspectorEmptyLab)
			: FText::FromString(FTryllEditorUiContract::InspectorEmptyChat);
		Body->AddSlot().Padding(10.0f, 60.0f, 10.0f, 10.0f)
		[
			SNew(STextBlock)
				.Text(Placeholder)
				.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
				.Justification(ETextJustify::Center)
				.AutoWrapText(true)
		];
		return;
	}

	HintText->SetText(FText::FromString(Turn.Hint.IsEmpty() ? Turn.TurnKey : Turn.Hint));

	if (Turn.AbsenceReason == ETryllDiagnosticsAbsenceReason::UnsupportedSchema)
	{
		Body->AddSlot()
		[
			BuildSection(TEXT("Diagnostics"), TEXT("unsupported schema"), true,
				BuildMuted(FTryllLabJsonRender::NoDiagnosticsNote(Turn.AbsenceReason)))
		];
		if (Turn.DebugInfo.IsValid() || !Turn.RawDebugInfoText.IsEmpty())
		{
			AddRawJsonSection();
		}
		return;
	}

	const bool bHasDiag = FTryllTurnInspectionFactory::HasUsableDiagnostics(Turn);
	const FTryllTurnRouteNode* Scoped = ScopedRouteNode();

	Body->AddSlot()
	[
		BuildSection(TEXT("Route"),
			Scoped != nullptr
				? FString::Printf(TEXT("%s only"), *Scoped->Name)
				: Turn.RouteTrailing,
			true, BuildRouteBody())
	];

	Body->AddSlot()
	[
		BuildSection(TEXT("Timing"), FString::Printf(TEXT("%.3fs"), Turn.DurationMs / 1000.0f), true,
			BuildTimingBody())
	];

	if (bHasDiag)
	{
		Body->AddSlot()
		[
			BuildSection(TEXT("Parameters"), TEXT("diagnostics.parameters"), false,
				BuildParametersBody())
		];
	}

	if (Lab != nullptr && Lab->AuthoredChecks.Num() > 0)
	{
		Body->AddSlot()
		[
			BuildSection(TEXT("Checks"),
				Lab->Failed.Num() > 0
					? FString::Printf(TEXT("%d failed"), Lab->Failed.Num())
					: FString(TEXT("all passed")),
				true, BuildChecksBody())
		];
	}

	const FTryllTurnInspection* Peer = (Lab != nullptr && Lab->DiffPeer.IsValid())
		? Lab->DiffPeer.Get()
		: nullptr;
	const bool bOtherHasDiag = Peer != nullptr
		&& FTryllTurnInspectionFactory::HasUsableDiagnostics(*Peer);

	const TArray<FTryllLabPromptSegment> MySegments = bHasDiag
		? ScopedSegments(FTryllLabJsonRender::PromptSegments(Turn.DebugInfo))
		: TArray<FTryllLabPromptSegment>();

	const FString Mine = bHasDiag
		? FTryllLabJsonRender::RenderSegments(MySegments)
		: FTryllLabJsonRender::NoDiagnosticsNote(Turn.AbsenceReason);

	const FString Trailing = bOtherHasDiag
		? FString(TEXT("diff vs peer"))
		: (Scoped != nullptr
			? Scoped->Name
			: FString::Printf(TEXT("%d chars"), Mine.Len()));

	TFunction<FString()> Copy = nullptr;
	if (bHasDiag)
	{
		Copy = [Mine] { return Mine; };
	}

	Body->AddSlot()
	[
		BuildSection(TEXT("Prompt & output"), Trailing, true,
			BuildPromptBody(Mine, Peer), Copy)
	];

	if (bHasDiag)
	{
		TArray<FTryllLabRetrievalExecution> Executions;
		for (const TPair<int32, TSharedPtr<FJsonObject>>& Node : ScopedNodes(Turn.DebugInfo))
		{
			FString NodeType;
			Node.Value->TryGetStringField(TEXT("_type"), NodeType);
			if (NodeType != TEXT("RetrieveNode"))
			{
				continue;
			}
			Executions.Add(MakeRetrievalExecution(Node.Key, Node.Value));
		}

		if (Executions.Num() > 0)
		{
			int32 TotalHits = 0;
			for (const FTryllLabRetrievalExecution& Execution : Executions)
			{
				TotalHits += Execution.Hits.Num();
			}
			const FString RetrievalTrailing = Executions.Num() == 1
				? FTryllDialogLabRetrieval::FormatHeader(
					Executions[0].Hits.Num(), Executions[0].Mode,
					Executions[0].Threshold, Executions[0].RrfK)
				: FString::Printf(TEXT("%d nodes · %d hits"), Executions.Num(), TotalHits);

			Body->AddSlot()
			[
				BuildSection(TEXT("Retrieval"), RetrievalTrailing, true,
					BuildRetrievalBody(Executions))
			];
		}

		const TArray<FTryllIntentExecution> IntentExecs =
			FTryllDialogLabIntent::ParseExecutions(Turn.DebugInfo, ScopedNodes(Turn.DebugInfo));
		if (IntentExecs.Num() > 0)
		{
			Body->AddSlot()
			[
				BuildSection(TEXT("Intent"),
					FTryllDialogLabIntent::FormatHeader(IntentExecs), true,
					BuildIntentBody(IntentExecs))
			];
		}
	}

	if (bHasDiag)
	{
		TArray<int32> ToolNodeIndices;
		TArray<TSharedPtr<FJsonObject>> ToolOutputs;
		for (const TPair<int32, TSharedPtr<FJsonObject>>& Entry : ScopedNodes(Turn.DebugInfo))
		{
			FString NodeType;
			Entry.Value->TryGetStringField(TEXT("_type"), NodeType);
			if (NodeType != TEXT("ToolCallNode"))
			{
				continue;
			}
			ToolNodeIndices.Add(Entry.Key);
			if (const TSharedPtr<FJsonObject>* Diag = GetObject(Entry.Value, TEXT("diagnostics")))
			{
				if (const TSharedPtr<FJsonObject>* Output = GetObject(*Diag, TEXT("output")))
				{
					ToolOutputs.Add(*Output);
				}
			}
		}

		if (ToolNodeIndices.Num() > 0)
		{
			int32 NodeCallCount = 0;
			for (const TSharedPtr<FJsonObject>& Output : ToolOutputs)
			{
				NodeCallCount += GetArray(Output, TEXT("tool_calls")).Num();
			}

			const TSharedPtr<FJsonObject>* Interaction = GetObject(Turn.DebugInfo, TEXT("interaction"));
			const TArray<TSharedPtr<FJsonValue>> Components = Interaction != nullptr
				? GetArray(*Interaction, TEXT("components")) : TArray<TSharedPtr<FJsonValue>>();
			int32 Records = 0;
			for (const TSharedPtr<FJsonValue>& Component : Components)
			{
				if (ComponentType(Component) == TEXT("ToolCallRecord")) { ++Records; }
			}

			const int32 CallCount = Records > 0 ? Records : NodeCallCount;

			TArray<FString> Exits;
			for (int32 NodeIndex : ToolNodeIndices)
			{
				if (Turn.Route.IsValidIndex(NodeIndex) && !Turn.Route[NodeIndex].Exit.IsEmpty())
				{
					Exits.AddUnique(Turn.Route[NodeIndex].Exit);
				}
			}
			const FString Exit = FString::Join(Exits, TEXT(", "));

			Body->AddSlot()
			[
				BuildSection(TEXT("Tool calls"),
					CallCount > 0 ? FString::Printf(TEXT("%d · %s"), CallCount, *Exit) : Exit,
					true, BuildToolCallsBody())
			];
		}
	}

	if (Turn.Status == ETryllTurnTerminalStatus::Error
		|| Turn.Status == ETryllTurnTerminalStatus::Skipped)
	{
		Body->AddSlot()
		[
			BuildSection(TEXT("Errors"), StatusName(Turn.Status), true,
				SNew(STextBlock)
					.Text(FText::FromString(Turn.ErrorText))
					.ColorAndOpacity(FSlateColor(Turn.Status == ETryllTurnTerminalStatus::Error
						? FTryllEditorStyle::ColErr : FTryllEditorStyle::Faint))
					.AutoWrapText(true))
		];
	}

	if (Turn.DebugInfo.IsValid() || !Turn.RawDebugInfoText.IsEmpty())
	{
		AddRawJsonSection();
	}
}

void STryllTurnInspectorView::AddRawJsonSection()
{
	const bool bParsed = Turn.DebugInfo.IsValid();
	const FString Rendered = bParsed
		? FTryllLabJsonRender::PrettyPrintJson(Turn.DebugInfo, 0)
		: FTryllLabJsonRender::PrettyJsonOrRaw(Turn.RawDebugInfoText);
	const FString Trailing = bParsed
		? FString::Printf(TEXT("%d top-level keys"), Turn.DebugInfo->Values.Num())
		: FString(TEXT("unreadable payload"));
	Body->AddSlot()
	[
		BuildSection(TEXT("Raw JSON"), Trailing,
			false,
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				BuildMuted(TEXT("Escape hatch. Everything above is a purpose-built view over this."))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				BuildCodeBlock(Rendered, 320.0f)
			],
			[Rendered] { return Rendered; })
	];
}

TSharedRef<SWidget> STryllTurnInspectorView::BuildPinSlot(
	const FString& Key, const FString& Value, const FLinearColor& Accent)
{
	const bool bFilled = !Value.IsEmpty();
	TSharedRef<SHorizontalBox> Head = SNew(SHorizontalBox);
	Head->AddSlot().AutoWidth()
	[
		SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("PIN %s"), *Key)))
			.ColorAndOpacity(FSlateColor(bFilled ? FTryllEditorStyle::Txt : FTryllEditorStyle::Faint))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
	];
	Head->AddSlot().FillWidth(1.0f)[SNullWidget::NullWidget];
	if (bFilled)
	{
		const FString Captured = Key;
		Head->AddSlot().AutoWidth()
		[
			SNew(SButton)
				.ContentPadding(FMargin(3.0f, 0.0f))
				.OnClicked_Lambda([this, Captured]
				{
					if (Turn.Lab.IsSet() && Turn.Lab->OnClearPin)
					{
						Turn.Lab->OnClearPin(Captured);
					}
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
						.Text(FText::FromString(TEXT("✕")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				]
		];
	}

	// The pin's accent is a 1 px border, not a fill — a filled slot reads as a selected button.
	return SNew(SBorder)
		.BorderImage(FTryllEditorStyle::Rounded(
			FTryllEditorStyle::RadiusHeader, bFilled ? Accent : FTryllEditorStyle::Line2, 1.0f))
		.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::Panel))
		.Padding(FMargin(6.0f, 2.0f, 4.0f, 3.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[Head]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
					.Text(FText::FromString(bFilled
						? FString::Printf(TEXT("column %s"), *Value)
						: FString(TEXT("—"))))
					.ColorAndOpacity(FSlateColor(bFilled ? FTryllEditorStyle::Txt : FTryllEditorStyle::Faint))
			]
		];
}

TSharedRef<SWidget> STryllTurnInspectorView::BuildSection(
	const FString& Title, const FString& Trailing, bool bInitiallyOpen,
	TSharedRef<SWidget> InBody, TFunction<FString()> CopyText)
{
	TSharedRef<SHorizontalBox> Header = SNew(SHorizontalBox);
	Header->AddSlot().AutoWidth().VAlign(VAlign_Center)
	[
		SNew(STextBlock)
			.Text(FText::FromString(Title.ToUpper()))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
	];
	Header->AddSlot().FillWidth(1.0f)[SNullWidget::NullWidget];
	if (!Trailing.IsEmpty())
	{
		Header->AddSlot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock)
				.Text(FText::FromString(Trailing))
				.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		];
	}
	if (CopyText)
	{
		// A word rather than a glyph: the clipboard glyphs are missing from the editor font, and a
		// missing-glyph box in a header is worse than four letters. Fixed width so the "copied"
		// swap does not reflow the header.
		TSharedRef<STextBlock> Label = SNew(STextBlock)
			.Text(LOCTEXT("Copy", "copy"))
			.Justification(ETextJustify::Center)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8));
		TWeakPtr<STextBlock> WeakLabel = Label;

		Header->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SBox).WidthOverride(48.0f)
			[
				SNew(SButton)
					.ToolTipText(LOCTEXT("CopyTip", "Copy to clipboard"))
					.ContentPadding(FMargin(0.0f))
					.OnClicked_Lambda([CopyText, WeakLabel]
					{
						FPlatformApplicationMisc::ClipboardCopy(*CopyText());
						// A silent clipboard write is indistinguishable from a dead button.
						if (TSharedPtr<STextBlock> Pinned = WeakLabel.Pin())
						{
							Pinned->SetText(LOCTEXT("Copied", "copied"));
							FTimerHandle Handle;
							GEditor->GetTimerManager()->SetTimer(Handle, [WeakLabel]
							{
								if (TSharedPtr<STextBlock> Later = WeakLabel.Pin())
								{
									Later->SetText(LOCTEXT("Copy", "copy"));
								}
							}, 0.9f, false);
						}
						return FReply::Handled();
					})
					[
						Label
					]
			]
		];
	}

	// A square-cornered header bar over a body on the panel colour, the pair underlined by a single
	// Line pixel. SExpandableArea's own chrome is a dark rounded header, which reads as a different
	// widget entirely, so the shell is built by hand. The bar's value is SectionHeader rather than
	// Unity's Panel2 — see the note on that constant.
	TSharedRef<bool> bOpen = MakeShared<bool>(bInitiallyOpen);

	TSharedRef<SWidget> HeaderBar = SNew(SBorder)
		.BorderImage(FTryllEditorStyle::Rounded(0.0f))
		.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::SectionHeader))
		.Padding(FMargin(10.0f, 4.0f))
		.OnMouseButtonDown_Lambda([bOpen, this](const FGeometry&, const FPointerEvent&)
		{
			*bOpen = !*bOpen;
			return FReply::Handled();
		})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(12.0f)
				[
					SNew(STextBlock)
						.Text_Lambda([bOpen]
						{
							return FText::FromString(*bOpen ? TEXT("▼") : TEXT("▶"));
						})
						.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)[Header]
		];

	return SNew(SBorder)
		.BorderImage(FTryllEditorStyle::Rounded(0.0f))
		.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::Line))
		.Padding(FMargin(0.0f, 0.0f, 0.0f, 1.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[HeaderBar]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBorder)
					.BorderImage(FTryllEditorStyle::Rounded(0.0f))
					.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::SectionBody))
					.Padding(FMargin(10.0f, 6.0f, 10.0f, 8.0f))
					.Visibility_Lambda([bOpen]
					{
						return *bOpen ? EVisibility::Visible : EVisibility::Collapsed;
					})
					[
						InBody
					]
			]
		];
}

TSharedRef<SWidget> STryllTurnInspectorView::BuildMuted(const FString& Text) const
{
	return SNew(STextBlock)
		.Text(FText::FromString(Text))
		.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
		.AutoWrapText(true)
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8));
}

TSharedRef<SWidget> STryllTurnInspectorView::BuildCodeBlock(const FString& Text, float MaxHeight) const
{
	return SNew(SBorder)
		.BorderImage(FTryllEditorStyle::Rounded(
			FTryllEditorStyle::RadiusHeader, FTryllEditorStyle::Line, 1.0f))
		.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::CodeBg))
		.Padding(FMargin(7.0f, 5.0f))
		[
			SNew(SBox).MaxDesiredHeight(MaxHeight)
			[
				SNew(SScrollBox).Orientation(Orient_Vertical)
				+ SScrollBox::Slot()
				[
					SNew(STextBlock)
						.Text(FText::FromString(Text))
						.ColorAndOpacity(FSlateColor(FTryllEditorStyle::DiffSame))
						.AutoWrapText(true)
						.Font(FCoreStyle::GetDefaultFontStyle("Mono", 8))
				]
			]
		];
}

TSharedRef<SWidget> STryllTurnInspectorView::BuildRouteBody() const
{
	if (Turn.Route.Num() == 0)
	{
		return BuildMuted(TEXT("Turn was not executed."));
	}

	TSharedRef<SWrapBox> Row = SNew(SWrapBox).UseAllottedSize(true);
	for (int32 Index = 0; Index < Turn.Route.Num(); ++Index)
	{
		const FTryllTurnRouteNode& Node = Turn.Route[Index];
		if (Index > 0)
		{
			Row->AddSlot().Padding(3.0f, 0.0f)
			[
				SNew(STextBlock)
					.Text(FText::FromString(TEXT("→")))
					.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
			];
		}

		const bool bScoped = NodeScope == Index;
		STryllTurnInspectorView* Self = const_cast<STryllTurnInspectorView*>(this);

		Row->AddSlot().Padding(0.0f, 2.0f)
		[
			SNew(SBorder)
				.BorderImage(FTryllEditorStyle::Rounded(
					FTryllEditorStyle::RadiusHeader,
					bScoped ? FTryllEditorStyle::Accent : FTryllEditorStyle::Line2, 1.0f))
				.BorderBackgroundColor(FSlateColor(
					bScoped ? FTryllEditorStyle::Panel : FTryllEditorStyle::Panel2))
				.ToolTipText(FText::FromString(bScoped
					? Node.Type + TEXT(" — click to show the whole turn again")
					: Node.Type + TEXT(" — click to narrow the panel to this node")))
				.Padding(FMargin(6.0f, 1.0f))
				// The section header toggle sits above this, so the click is consumed here.
				.OnMouseButtonDown_Lambda(
					[Self, Index](const FGeometry&, const FPointerEvent&)
					{
						Self->NodeScope = Self->NodeScope == Index ? -1 : Index;
						Self->RebuildInspector();
						return FReply::Handled();
					})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(STextBlock)
							.Text(FText::FromString(Node.Name))
							.ColorAndOpacity(FSlateColor(
								bScoped ? FTryllEditorStyle::Accent : FTryllEditorStyle::Txt))
							.Font(FCoreStyle::GetDefaultFontStyle(
								bScoped ? "Bold" : "Regular", 9))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("[%s]"), *Node.Exit)))
							.ColorAndOpacity(FSlateColor(Node.Exit == TEXT("not_found")
								? FTryllEditorStyle::ColWarn : FTryllEditorStyle::ColOk))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					]
				]
		];
	}

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[Row]
		+ SVerticalBox::Slot().AutoHeight()
		[
			BuildMuted(NodeScope >= 0
				? TEXT("Showing this node only. Click it again for the whole turn.")
				: TEXT("Click a node to narrow the blocks below to it."))
		];
}

TArray<TPair<int32, TSharedPtr<FJsonObject>>> STryllTurnInspectorView::ScopedNodes(
	const TSharedPtr<FJsonObject>& DebugInfo) const
{
	TArray<TPair<int32, TSharedPtr<FJsonObject>>> Result;
	if (!DebugInfo.IsValid())
	{
		return Result;
	}
	const TArray<TSharedPtr<FJsonValue>>* Nodes = nullptr;
	if (!DebugInfo->TryGetArrayField(TEXT("nodes"), Nodes) || Nodes == nullptr)
	{
		return Result;
	}
	for (int32 Index = 0; Index < Nodes->Num(); ++Index)
	{
		if (NodeScope >= 0 && NodeScope != Index)
		{
			continue;
		}
		const TSharedPtr<FJsonObject>* Node = nullptr;
		if ((*Nodes)[Index].IsValid() && (*Nodes)[Index]->TryGetObject(Node) && Node != nullptr)
		{
			Result.Add({Index, *Node});
		}
	}
	return Result;
}

TArray<FTryllLabPromptSegment> STryllTurnInspectorView::ScopedSegments(
	const TArray<FTryllLabPromptSegment>& All) const
{
	if (NodeScope < 0)
	{
		return All;
	}
	return All.FilterByPredicate([this](const FTryllLabPromptSegment& Segment)
	{
		return Segment.NodeIndex == NodeScope;
	});
}

TArray<FTryllLabPromptSegment> STryllTurnInspectorView::MatchingSegments(
	const TArray<FTryllLabPromptSegment>& Theirs,
	const TArray<FTryllLabPromptSegment>& Mine) const
{
	if (NodeScope < 0)
	{
		return Theirs;
	}
	TSet<FString> Keys;
	for (const FTryllLabPromptSegment& Segment : Mine)
	{
		Keys.Add(Segment.PairKey());
	}
	return Theirs.FilterByPredicate([&Keys](const FTryllLabPromptSegment& Segment)
	{
		return Keys.Contains(Segment.PairKey());
	});
}

const FTryllTurnRouteNode* STryllTurnInspectorView::ScopedRouteNode() const
{
	return (NodeScope >= 0 && Turn.Route.IsValidIndex(NodeScope)) ? &Turn.Route[NodeScope] : nullptr;
}

TSharedRef<SWidget> STryllTurnInspectorView::BuildTimingBody() const
{
	if (Turn.Route.Num() == 0)
	{
		return BuildMuted(TEXT("—"));
	}

	int32 Total = 0;
	for (const FTryllTurnRouteNode& Node : Turn.Route)
	{
		Total += FMath::Max(0, Node.Ms);
	}

	TSharedRef<SHorizontalBox> Bar = SNew(SHorizontalBox);
	if (Total <= 0)
	{
		Bar->AddSlot().FillWidth(1.0f)
		[
			SNew(SColorBlock).Color(FTryllEditorStyle::Line2).Size(FVector2D(1.0f, 6.0f))
		];
	}
	else
	{
		for (const FTryllTurnRouteNode& Node : Turn.Route)
		{
			if (Node.Ms <= 0) { continue; }
			Bar->AddSlot().FillWidth(static_cast<float>(Node.Ms))
			[
				SNew(SColorBlock)
					.Color(FTryllEditorStyle::NodeColor(Node.Type))
					.Size(FVector2D(1.0f, 6.0f))
			];
		}
	}

	TSharedRef<SWrapBox> Legend = SNew(SWrapBox).UseAllottedSize(true);
	for (const FTryllTurnRouteNode& Node : Turn.Route)
	{
		Legend->AddSlot().Padding(0.0f, 2.0f, 10.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SColorBlock)
					.Color(FTryllEditorStyle::NodeColor(Node.Type))
					.Size(FVector2D(8.0f, 8.0f))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(5.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("%s %dms"), *Node.Name, Node.Ms)))
					.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			]
		];
	}
	if (Turn.TtftMs > 0)
	{
		Legend->AddSlot().Padding(0.0f, 2.0f, 10.0f, 0.0f)
		[
			SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("TTFT %dms"), Turn.TtftMs)))
				.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		];
	}

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[Bar]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)[Legend];
}

TSharedRef<SWidget> STryllTurnInspectorView::BuildParametersBody() const
{
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	bool bAny = false;

	for (const TPair<int32, TSharedPtr<FJsonObject>>& Scoped : ScopedNodes(Turn.DebugInfo))
	{
		const TSharedPtr<FJsonObject>& Node = Scoped.Value;
		const TSharedPtr<FJsonObject>* Diag = GetObject(Node, TEXT("diagnostics"));
		const TSharedPtr<FJsonObject>* Params = Diag != nullptr ? GetObject(*Diag, TEXT("parameters")) : nullptr;
		if (Params == nullptr || (*Params)->Values.Num() == 0) { continue; }

		bAny = true;
		FString NodeName;
		Node->TryGetStringField(TEXT("name"), NodeName);
		// Index-prefixed, so two executions of the same node are told apart.
		Rows->AddSlot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("#%d %s"), Scoped.Key, *NodeName)))
				.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Txt))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
		];

		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Params)->Values)
		{
			Rows->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 1.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(140.0f)
					[
						SNew(STextBlock)
							.Text(FText::FromString(Pair.Key))
							.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(STextBlock)
						.Text(FText::FromString(ParameterValueText(Pair.Value)))
						.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
						.AutoWrapText(true)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				]
			];
		}
	}

	return bAny ? Rows : BuildMuted(NodeScope >= 0
		? TEXT("This node reports no parameters.")
		: FTryllEditorUiContract::InspectorNoParameters);
}

TSharedRef<SWidget> STryllTurnInspectorView::BuildChecksBody() const
{
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	if (!Turn.Lab.IsSet())
	{
		return Rows;
	}

	const FTryllTurnLabContext& Lab = Turn.Lab.GetValue();
	for (const FTryllLabCheck& Authored : Lab.AuthoredChecks)
	{
		const FTryllLabCheckOutcome* Failure = Lab.Failed.FindByPredicate(
			[&Authored](const FTryllLabCheckOutcome& F)
			{
				return FTryllDialogLabChecks::Matches(F, Authored);
			});
		const bool bBad = Failure != nullptr;
		const FString Text = (bBad && !Failure->Label.IsEmpty()) ? Failure->Label : Authored.Label;

		Rows->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
					.Text(FText::FromString(Text))
					.ColorAndOpacity(FSlateColor(bBad ? FTryllEditorStyle::ColWarn : FTryllEditorStyle::Dim))
					.AutoWrapText(true)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBorder)
					.BorderImage(FTryllEditorStyle::Rounded(
						FTryllEditorStyle::RadiusChip,
						bBad ? FTryllEditorStyle::WarnBorder : FTryllEditorStyle::OkBorder, 1.0f))
					.BorderBackgroundColor(FSlateColor(bBad ? FTryllEditorStyle::WarnBg : FTryllEditorStyle::OkBg))
					.Padding(FMargin(5.0f, 0.0f, 5.0f, 1.0f))
					[
						SNew(STextBlock)
							.Text(FText::FromString(bBad ? TEXT("✗") : TEXT("✓")))
							.ColorAndOpacity(FSlateColor(bBad ? FTryllEditorStyle::ColWarn : FTryllEditorStyle::ColOk))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					]
			]
		];
	}
	return Rows;
}

TSharedRef<SWidget> STryllTurnInspectorView::BuildSegmentHeading(
	const FTryllLabPromptSegment& Segment, const FString& Suffix) const
{
	FString Text = FString::Printf(TEXT("#%d %s"), Segment.NodeIndex, *Segment.NodeName);
	if (!Segment.NodeType.IsEmpty())
	{
		Text += FString::Printf(TEXT(" (%s)"), *Segment.NodeType);
	}
	if (!Segment.ExitRoute.IsEmpty())
	{
		Text += FString::Printf(TEXT(" [%s]"), *Segment.ExitRoute);
	}
	if (!Suffix.IsEmpty())
	{
		Text += TEXT("  ") + Suffix;
	}

	return SNew(SBox).Padding(FMargin(0.0f, 6.0f, 0.0f, 2.0f))
	[
		SNew(STextBlock)
			.Text(FText::FromString(Text))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
			.AutoWrapText(true)
	];
}

TSharedRef<SWidget> STryllTurnInspectorView::BuildDiffLines(
	const TArray<FTryllLabDiffLine>& DiffLines) const
{
	TSharedRef<SVerticalBox> Lines = SNew(SVerticalBox);
	for (const FTryllLabDiffLine& Line : DiffLines)
	{
		const TCHAR* Prefix =
			Line.Op == ETryllLabDiffOp::Added   ? TEXT("+ ") :
			Line.Op == ETryllLabDiffOp::Removed ? TEXT("− ") : TEXT("  ");
		const FLinearColor Colour =
			Line.Op == ETryllLabDiffOp::Added   ? FTryllEditorStyle::DiffAdded :
			Line.Op == ETryllLabDiffOp::Removed ? FTryllEditorStyle::DiffRemoved :
			FTryllEditorStyle::DiffSame;
		const FLinearColor Background =
			Line.Op == ETryllLabDiffOp::Added   ? FTryllEditorStyle::OkBg :
			Line.Op == ETryllLabDiffOp::Removed ? FTryllEditorStyle::ErrBg :
			FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);

		Lines->AddSlot().AutoHeight()
		[
			SNew(SBorder)
				.BorderBackgroundColor(FSlateColor(Background))
				.Padding(FMargin(0.0f))
				[
					SNew(STextBlock)
						// An empty line renders as a space so it stays visible.
						.Text(FText::FromString(Prefix + (Line.Line.IsEmpty() ? TEXT(" ") : Line.Line)))
						.ColorAndOpacity(FSlateColor(Colour))
						.AutoWrapText(true)
						.Font(FCoreStyle::GetDefaultFontStyle("Mono", 8))
				]
		];
	}

	return Lines;
}

TSharedRef<SWidget> STryllTurnInspectorView::BuildPromptBody(
	const FString& Mine, const FTryllTurnInspection* Peer) const
{
	const bool bHasDiag = FTryllTurnInspectionFactory::HasUsableDiagnostics(Turn);
	const bool bOtherHasDiag = Peer != nullptr
		&& FTryllTurnInspectionFactory::HasUsableDiagnostics(*Peer);

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	if (Peer != nullptr && !bOtherHasDiag)
	{
		Box->AddSlot().AutoHeight()
		[
			BuildMuted(FString::Printf(
				TEXT("%s %s"),
				FTryllEditorUiContract::InspectorNoDiffPeer,
				*FTryllLabJsonRender::NoDiagnosticsNote(Peer->AbsenceReason)))
		];
	}

	if (bOtherHasDiag)
	{
		const TArray<FTryllLabPromptSegment> MySegments =
			ScopedSegments(FTryllLabJsonRender::PromptSegments(Turn.DebugInfo));
		const TArray<FTryllLabPromptSegment> TheirSegments = MatchingSegments(
			FTryllLabJsonRender::PromptSegments(Peer->DebugInfo), MySegments);

		TSharedRef<SVerticalBox> Stack = SNew(SVerticalBox);
		for (const FTryllLabSegmentDiff& Segment :
			FTryllLabJsonRender::DiffSegments(MySegments, TheirSegments))
		{
			const FString Suffix =
				Segment.Pairing == ETryllLabSegmentPairing::OnlyMine
					? FString(TEXT("· only in selected")) :
				Segment.Pairing == ETryllLabSegmentPairing::OnlyTheirs
					? FString(TEXT("· only in peer"))
					: FString();

			Stack->AddSlot().AutoHeight()[BuildSegmentHeading(Segment.Either(), Suffix)];
			Stack->AddSlot().AutoHeight()[BuildDiffLines(Segment.Lines)];
		}

		Box->AddSlot().AutoHeight()
		[
			SNew(SBorder)
				.BorderImage(FTryllEditorStyle::Rounded(
					FTryllEditorStyle::RadiusHeader, FTryllEditorStyle::Line, 1.0f))
				.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::CodeBg))
				.Padding(FMargin(7.0f, 5.0f))
				[
					SNew(SBox).MaxDesiredHeight(240.0f)
					[
						SNew(SScrollBox).Orientation(Orient_Vertical)
						+ SScrollBox::Slot()[Stack]
					]
				]
		];
		Box->AddSlot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
		[
			BuildMuted(TEXT("+ green = only in selected   ·   − red = only in peer"))
		];
		return Box;
	}

	const TArray<FTryllLabPromptSegment> MySegments = bHasDiag
		? ScopedSegments(FTryllLabJsonRender::PromptSegments(Turn.DebugInfo))
		: TArray<FTryllLabPromptSegment>();

	if (bHasDiag && MySegments.Num() == 0)
	{
		const FTryllTurnRouteNode* Scoped = ScopedRouteNode();
		Box->AddSlot().AutoHeight()
		[
			BuildMuted(Scoped != nullptr
				? FString::Printf(
					TEXT("%s runs no inference, so it has no prompt or output."), *Scoped->Name)
				: FString(TEXT("(no prompt data on this turn)")))
		];
		return Box;
	}

	if (!bHasDiag)
	{
		Box->AddSlot().AutoHeight()[BuildCodeBlock(Mine, 240.0f)];
		return Box;
	}

	const bool bLabel = MySegments.Num() > 1;
	TSharedRef<SVerticalBox> Stack = SNew(SVerticalBox);
	for (const FTryllLabPromptSegment& Segment : MySegments)
	{
		if (bLabel)
		{
			Stack->AddSlot().AutoHeight()[BuildSegmentHeading(Segment, FString())];
		}
		Stack->AddSlot().AutoHeight()
		[
			SNew(STextBlock)
				.Text(FText::FromString(FTryllLabJsonRender::RenderSegmentBody(Segment)))
				.ColorAndOpacity(FSlateColor(FTryllEditorStyle::DiffSame))
				.AutoWrapText(true)
				.Font(FCoreStyle::GetDefaultFontStyle("Mono", 8))
		];
	}

	Box->AddSlot().AutoHeight()
	[
		SNew(SBorder)
			.BorderImage(FTryllEditorStyle::Rounded(
				FTryllEditorStyle::RadiusHeader, FTryllEditorStyle::Line, 1.0f))
			.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::CodeBg))
			.Padding(FMargin(7.0f, 5.0f))
			[
				SNew(SBox).MaxDesiredHeight(240.0f)
				[
					SNew(SScrollBox).Orientation(Orient_Vertical)
					+ SScrollBox::Slot()[Stack]
				]
			]
	];
	return Box;
}

FTryllLabRetrievalExecution STryllTurnInspectorView::MakeRetrievalExecution(
	int32 NodeIndex, const TSharedPtr<FJsonObject>& Node)
{
	FTryllLabRetrievalExecution Execution;
	Execution.NodeIndex = NodeIndex;
	Node->TryGetStringField(TEXT("name"), Execution.NodeName);

	const TSharedPtr<FJsonObject>* Diag = GetObject(Node, TEXT("diagnostics"));
	if (Diag == nullptr)
	{
		return Execution;
	}

	if (const TSharedPtr<FJsonObject>* Params = GetObject(*Diag, TEXT("parameters")))
	{
		(*Params)->TryGetStringField(TEXT("retrieval_mode"), Execution.Mode);
		(*Params)->TryGetNumberField(TEXT("threshold"), Execution.Threshold);
		double K = 60.0;
		if ((*Params)->TryGetNumberField(TEXT("rrf_k"), K))
		{
			Execution.RrfK = static_cast<int32>(K);
		}
	}
	if (const TSharedPtr<FJsonObject>* Input = GetObject(*Diag, TEXT("input")))
	{
		(*Input)->TryGetStringField(TEXT("query"), Execution.Query);
	}
	if (const TSharedPtr<FJsonObject>* Output = GetObject(*Diag, TEXT("output")))
	{
		Execution.Hits = FTryllDialogLabRetrieval::ParseHits((*Output)->TryGetField(TEXT("results")));
	}
	return Execution;
}

TSharedRef<SWidget> STryllTurnInspectorView::BuildRetrievalBody(
	const TArray<FTryllLabRetrievalExecution>& Executions) const
{
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	const bool bLabel = Executions.Num() > 1;

	for (const FTryllLabRetrievalExecution& Execution : Executions)
	{
	if (bLabel)
	{
		Rows->AddSlot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 2.0f)
		[
			SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("#%d %s  %s"),
					Execution.NodeIndex, *Execution.NodeName,
					*FTryllDialogLabRetrieval::FormatHeader(
						Execution.Hits.Num(), Execution.Mode,
						Execution.Threshold, Execution.RrfK))))
				.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.AutoWrapText(true)
		];
	}

	if (!Execution.Query.IsEmpty())
	{
		Rows->AddSlot().AutoHeight()
		[
			BuildMuted(FString::Printf(TEXT("query: %s"), *Execution.Query))
		];
	}

	if (Execution.Hits.Num() == 0)
	{
		Rows->AddSlot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
		[
			BuildMuted(FTryllDialogLabRetrieval::FormatEmptyMessage(Execution.Mode))
		];
		continue;
	}

	for (const FTryllLabRetrievalHit& Hit : Execution.Hits)
	{
		const FString Meta = FTryllDialogLabRetrieval::FormatHitMeta(Hit);
		const FString BodyText = Meta.IsEmpty() ? Hit.Text : Meta + TEXT("\n") + Hit.Text;

		Rows->AddSlot().AutoHeight().Padding(0.0f, 2.0f)
		[
			SNew(SBorder)
				.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::Line))
				.Padding(FMargin(0.0f, 0.0f, 0.0f, 2.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(60.0f)
						[
							SNew(STextBlock)
								.Text(FText::FromString(Hit.FormatDistance()))
								.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Txt))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						]
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(88.0f)
						[
							SNew(STextBlock)
								.Text(FText::FromString(Hit.Id))
								.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						SNew(STextBlock)
							.Text(FText::FromString(BodyText))
							.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
							.AutoWrapText(true)
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					]
				]
		];
	}
	}
	return Rows;
}

FTryllIntentExecution STryllTurnInspectorView::MakeIntentExecution(
	int32 NodeIndex, const TSharedPtr<FJsonObject>& Node)
{
	return FTryllDialogLabIntent::ParseOne(NodeIndex, Node);
}

TSharedRef<SWidget> STryllTurnInspectorView::BuildIntentBody(
	const TArray<FTryllIntentExecution>& Executions) const
{
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);

	for (const FTryllIntentExecution& Execution : Executions)
	{
		TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
		Box->AddSlot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(
						TEXT("#%d %s"), Execution.NodeIndex, *Execution.NodeName)))
					.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Txt))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)
			[
				IntentChip(
					Execution.bAccepted ? TEXT("found") : TEXT("not_found"),
					Execution.bAccepted)
			]
		];

		Box->AddSlot().AutoHeight()
		[
			BuildMuted(Execution.Kind == FTryllIntentKind::Embedding
				? FTryllDialogLabIntent::FormatEmbedSubHeader(Execution)
				: FTryllDialogLabIntent::FormatLlmSubHeader(Execution))
		];

		if (Execution.Kind == FTryllIntentKind::Embedding)
		{
			if (Execution.bAccepted)
			{
				Box->AddSlot().AutoHeight()
				[
					SNew(STextBlock)
						.Text(FText::FromString(
							Execution.Intent.IsEmpty() ? FString(TEXT("—")) : Execution.Intent))
						.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Txt))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.AutoWrapText(true)
				];

				FString Meta = FString(TEXT("distance "))
					+ FTryllDialogLabIntent::FormatDistance(Execution.Distance);
				if (!Execution.RecordId.IsEmpty())
				{
					Meta += TEXT(" · id ") + Execution.RecordId;
				}
				if (Execution.RecordIndex.IsSet())
				{
					Meta += TEXT(" · index ") + FString::FromInt(Execution.RecordIndex.GetValue());
				}
				Box->AddSlot().AutoHeight()
				[
					SNew(STextBlock)
						.Text(FText::FromString(Meta))
						.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.AutoWrapText(true)
				];

				if (!Execution.RecordText.IsEmpty())
				{
					Box->AddSlot().AutoHeight()
					[
						SNew(STextBlock)
							.Text(FText::FromString(Execution.RecordText))
							.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
							.AutoWrapText(true)
					];
				}

				if (!Execution.RunnerUpIntent.IsEmpty())
				{
					Box->AddSlot().AutoHeight()
					[
						BuildMuted(FString(TEXT("runner-up ")) + Execution.RunnerUpIntent
							+ TEXT(" @ ") + FTryllDialogLabIntent::FormatDistance(Execution.RunnerUpDistance)
							+ TEXT(" (margin ") + FTryllDialogLabIntent::FormatDistance(Execution.Margin)
							+ TEXT(")"))
					];
				}
				else
				{
					Box->AddSlot().AutoHeight()[BuildMuted(TEXT("every hit shared the winning label"))];
				}
			}
			else
			{
				const FString Reason = Execution.NotFoundReason.IsEmpty()
					? FString(TEXT("not_found")) : Execution.NotFoundReason;
				Box->AddSlot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[IntentChip(Reason, false)]
				];

				const FString Rejected = FTryllDialogLabIntent::ResolveLabel(Execution);
				if (!Rejected.IsEmpty())
				{
					FString Line = FString(TEXT("rejected ")) + Rejected
						+ TEXT(" @ ") + FTryllDialogLabIntent::FormatDistance(Execution.Distance);
					if (Execution.Threshold.IsSet())
					{
						Line += FString::Printf(TEXT(" (threshold %.2f)"), Execution.Threshold.GetValue());
					}
					Box->AddSlot().AutoHeight()[BuildMuted(Line)];
				}
			}

			if (Execution.Topk.Num() == 0)
			{
				Box->AddSlot().AutoHeight()
				[
					BuildMuted(FTryllDialogLabIntent::FormatEmptyMessage(Execution))
				];
			}
			else
			{
				for (const FTryllIntentCandidate& Hit : Execution.Topk)
				{
					const bool bBeyond = Execution.Threshold.IsSet() && Hit.Distance.IsSet()
						&& Hit.Distance.GetValue() > Execution.Threshold.GetValue();
					Box->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 2.0f)
					[
						SNew(SBorder)
							.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::Line))
							.Padding(FMargin(0.0f, 0.0f, 0.0f, 2.0f))
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(SBox).WidthOverride(28.0f)
									[
										SNew(STextBlock)
											.Text(FText::FromString(FString::Printf(TEXT("#%d"), Hit.Rank)))
											.ColorAndOpacity(FSlateColor(
												bBeyond ? FTryllEditorStyle::Faint : FTryllEditorStyle::Dim))
											.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
									]
								]
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(SBox).WidthOverride(60.0f)
									[
										SNew(STextBlock)
											.Text(FText::FromString(
												FTryllDialogLabIntent::FormatDistance(Hit.Distance)))
											.ColorAndOpacity(FSlateColor(
												bBeyond ? FTryllEditorStyle::Faint : FTryllEditorStyle::Txt))
											.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
									]
								]
								+ SHorizontalBox::Slot().FillWidth(1.0f)
								[
									SNew(STextBlock)
										.Text(FText::FromString(Hit.Intent))
										.ColorAndOpacity(FSlateColor(
											bBeyond ? FTryllEditorStyle::Faint : FTryllEditorStyle::Dim))
										.AutoWrapText(true)
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
								]
							]
					];
				}
			}
		}
		else
		{
			const FString Shown = Execution.bAccepted
				? Execution.Intent : FTryllDialogLabIntent::ResolveLabel(Execution);
			if (!Shown.IsEmpty())
			{
				Box->AddSlot().AutoHeight()
				[
					SNew(STextBlock)
						.Text(FText::FromString(Shown))
						.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Txt))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.AutoWrapText(true)
				];
			}

			const FString Strip = FString(TEXT("letter "))
				+ (Execution.PredictedLetter.IsEmpty() ? FString(TEXT("—")) : Execution.PredictedLetter)
				+ TEXT(" · top ") + FTryllDialogLabIntent::FormatProb(Execution.TopProb)
				+ TEXT(" · second ") + FTryllDialogLabIntent::FormatProb(Execution.SecondProb)
				+ TEXT(" · margin ") + FTryllDialogLabIntent::FormatProb(Execution.MarginResult);
			Box->AddSlot().AutoHeight()
			[
				SNew(STextBlock)
					.Text(FText::FromString(Strip))
					.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					.AutoWrapText(true)
			];

			if (Execution.bProbsLengthMismatch)
			{
				Box->AddSlot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						IntentChip(FTryllDialogLabIntent::FormatMismatchWarning(Execution), false)
					]
				];
			}

			if (Execution.Probs.Num() == 0)
			{
				if (!Execution.NotFoundReason.IsEmpty())
				{
					Box->AddSlot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						[
							IntentChip(Execution.NotFoundReason, false)
						]
					];
				}
			}
			else
			{
				double Max = 0.0;
				for (const FTryllIntentLabelProb& Entry : Execution.Probs)
				{
					if (Entry.Prob > Max)
					{
						Max = Entry.Prob;
					}
				}
				if (Max <= 0.0)
				{
					Max = 1.0;
				}
				for (int32 Index = 0; Index < Execution.Probs.Num(); ++Index)
				{
					const FTryllIntentLabelProb& Entry = Execution.Probs[Index];
					Box->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 2.0f)
					[
						IntentProbBar(Entry.Label, Entry.Prob, Max, Execution.Threshold)
					];
					if (Index == 0 && Execution.MarginResult.IsSet())
					{
						Box->AddSlot().AutoHeight()
						[
							BuildMuted(FString(TEXT("margin "))
								+ FTryllDialogLabIntent::FormatProb(Execution.MarginResult))
						];
					}
				}
			}
		}

		Rows->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[Box];
	}

	const TArray<FString> Attached = FTryllDialogLabIntent::AttachedIntents(Turn.DebugInfo);
	if (Attached.Num() > 0)
	{
		Rows->AddSlot().AutoHeight()
		[
			BuildMuted(FString(TEXT("attached to dialog: ")) + FString::Join(Attached, TEXT(", ")))
		];
	}

	return Rows;
}

TSharedRef<SWidget> STryllTurnInspectorView::BuildToolCallsBody() const
{
	TArray<TSharedPtr<FJsonObject>> ToolOutputs;
	for (const TPair<int32, TSharedPtr<FJsonObject>>& Scoped : ScopedNodes(Turn.DebugInfo))
	{
		FString NodeType;
		Scoped.Value->TryGetStringField(TEXT("_type"), NodeType);
		if (NodeType != TEXT("ToolCallNode"))
		{
			continue;
		}
		if (const TSharedPtr<FJsonObject>* Diag = GetObject(Scoped.Value, TEXT("diagnostics")))
		{
			if (const TSharedPtr<FJsonObject>* Output = GetObject(*Diag, TEXT("output")))
			{
				ToolOutputs.Add(*Output);
			}
		}
	}

	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);

	// Only when true — a false value on every ordinary turn is noise.
	bool bParseFallback = false;
	for (const TSharedPtr<FJsonObject>& Output : ToolOutputs)
	{
		bool bNodeFallback = false;
		if (Output->TryGetBoolField(TEXT("parse_fallback"), bNodeFallback) && bNodeFallback)
		{
			bParseFallback = true;
			break;
		}
	}
	if (bParseFallback)
	{
		Rows->AddSlot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBorder)
					.BorderImage(FTryllEditorStyle::Rounded(
						FTryllEditorStyle::RadiusChip, FTryllEditorStyle::WarnBorder, 1.0f))
					.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::WarnBg))
					.Padding(FMargin(5.0f, 0.0f, 5.0f, 1.0f))
					[
						SNew(STextBlock)
							.Text(LOCTEXT("ParseFallback", "parse fallback"))
							.ColorAndOpacity(FSlateColor(FTryllEditorStyle::ColWarn))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					]
			]
		];
		Rows->AddSlot().AutoHeight()
		[
			BuildMuted(TEXT("The output could not be parsed as a tool call, so no call was "
				"recorded. The raw output is kept as text, and is sent as the answer under "
				"call_or_answer."))
		];
		for (const TSharedPtr<FJsonObject>& Output : ToolOutputs)
		{
			bool bNodeFallback = false;
			FString Failure;
			if (!Output->TryGetBoolField(TEXT("parse_fallback"), bNodeFallback) || !bNodeFallback
				|| !Output->TryGetStringField(TEXT("parse_failure"), Failure) || Failure.IsEmpty())
			{
				continue;
			}
			FString Error;
			Output->TryGetStringField(TEXT("parse_error"), Error);
			Rows->AddSlot().AutoHeight()
			[
				BuildMuted(Error.IsEmpty() ? Failure : FString::Printf(TEXT("%s: %s"), *Failure, *Error))
			];
		}
	}

	// interaction.components[] carries no node linkage, so the records below cannot be attributed
	// to one execution when the turn ran several. Say so rather than implying they belong to one.
	const int32 TurnToolNodes = Turn.Route.FilterByPredicate(
		[](const FTryllTurnRouteNode& Node) { return Node.Type == TEXT("ToolCallNode"); }).Num();
	if (TurnToolNodes > 1)
	{
		Rows->AddSlot().AutoHeight()
		[
			BuildMuted(FString::Printf(
				TEXT("This turn ran %d ToolCall nodes. Results come from the turn's interaction ")
				TEXT("components, which carry no node linkage, so they are listed for the whole turn."),
				TurnToolNodes))
		];
	}

	TArray<TSharedPtr<FJsonValue>> NodeCalls;
	for (const TSharedPtr<FJsonObject>& Output : ToolOutputs)
	{
		NodeCalls.Append(GetArray(Output, TEXT("tool_calls")));
	}

	const TSharedPtr<FJsonObject>* Interaction = GetObject(Turn.DebugInfo, TEXT("interaction"));
	const TArray<TSharedPtr<FJsonValue>> Components = Interaction != nullptr
		? GetArray(*Interaction, TEXT("components")) : TArray<TSharedPtr<FJsonValue>>();

	TArray<TSharedPtr<FJsonObject>> Records;
	TArray<TSharedPtr<FJsonObject>> Results;
	for (const TSharedPtr<FJsonValue>& Component : Components)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (!Component.IsValid() || !Component->TryGetObject(Object) || Object == nullptr) { continue; }
		const FString Type = ComponentType(Component);
		if (Type == TEXT("ToolCallRecord"))          { Records.Add(*Object); }
		else if (Type == TEXT("ToolResultComponent")) { Results.Add(*Object); }
	}

	auto AddCard = [&Rows, this](const FString& ToolName, const FString& Args, const FString* Result)
	{
		TSharedRef<SVerticalBox> Card = SNew(SVerticalBox);
		Card->AddSlot().AutoHeight()
		[
			SNew(STextBlock)
				.Text(FText::FromString(ToolName))
				.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Txt))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
		];
		Card->AddSlot().AutoHeight()
		[
			SNew(STextBlock)
				.Text(FText::FromString(FTryllLabJsonRender::PrettyJsonOrRaw(Args)))
				.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
				.AutoWrapText(true)
				.Font(FCoreStyle::GetDefaultFontStyle("Mono", 8))
		];
		if (Result == nullptr || Result->IsEmpty())
		{
			// Normal for RouteOnly / Notify / bare Pause — those write no result component at
			// all, so this is "nothing to show", not "missing".
			Card->AddSlot().AutoHeight()[BuildMuted(TEXT("no result recorded for this call"))];
		}
		else
		{
			Card->AddSlot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
					.Text(FText::FromString(TEXT("→ ") + *Result))
					.ColorAndOpacity(FSlateColor(FTryllEditorStyle::ColOk))
					.AutoWrapText(true)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			];
		}

		Rows->AddSlot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 2.0f)
		[
			SNew(SBorder)
				.BorderImage(FTryllEditorStyle::Rounded(
					FTryllEditorStyle::RadiusHeader, FTryllEditorStyle::Line, 1.0f))
				.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::CodeBg))
				.Padding(FMargin(8.0f, 4.0f, 8.0f, 5.0f))
				[
					Card
				]
		];
	};

	// The un-flattened arguments_json the model emitted, matched by tool name.
	auto RawArgsForTool = [&NodeCalls](const FString& ToolName) -> FString
	{
		for (const TSharedPtr<FJsonValue>& Entry : NodeCalls)
		{
			const TSharedPtr<FJsonObject>* Call = nullptr;
			if (!Entry.IsValid() || !Entry->TryGetObject(Call) || Call == nullptr) { continue; }
			FString Name;
			(*Call)->TryGetStringField(TEXT("name"), Name);
			if (Name == ToolName)
			{
				FString Args;
				(*Call)->TryGetStringField(TEXT("arguments_json"), Args);
				if (!Args.IsEmpty()) { return Args; }
			}
		}
		return FString();
	};

	const int32 CallCount = Records.Num() > 0 ? Records.Num() : NodeCalls.Num();
	if (CallCount == 0)
	{
		Rows->AddSlot().AutoHeight()[BuildMuted(TEXT("No tool call parsed this turn."))];
		return Rows;
	}

	if (Records.Num() == 0)
	{
		// The call↔result join key exists only on interaction.components[], which are gated by
		// include_interaction_in_diagnostics. Say why rather than rendering nothing.
		Rows->AddSlot().AutoHeight()
		[
			BuildMuted(TEXT("Results unavailable — interaction diagnostics are off "
				"(include_interaction_in_diagnostics in server-config.json), so calls cannot be "
				"paired with their results."))
		];
		for (const TSharedPtr<FJsonValue>& Entry : NodeCalls)
		{
			const TSharedPtr<FJsonObject>* Call = nullptr;
			if (!Entry.IsValid() || !Entry->TryGetObject(Call) || Call == nullptr) { continue; }
			FString Name;
			FString Args;
			(*Call)->TryGetStringField(TEXT("name"), Name);
			(*Call)->TryGetStringField(TEXT("arguments_json"), Args);
			AddCard(Name, Args, nullptr);
		}
		return Rows;
	}

	for (const TSharedPtr<FJsonObject>& Record : Records)
	{
		FString CallId;
		FString Name;
		Record->TryGetStringField(TEXT("call_id"), CallId);
		Record->TryGetStringField(TEXT("tool_name"), Name);

		FString Args = RawArgsForTool(Name);
		if (Args.IsEmpty())
		{
			// Fallback: the record's own flattened `arg.*` keys, re-grouped into an object.
			TArray<FString> Parts;
			for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Record->Values)
			{
				if (Pair.Key.StartsWith(TEXT("arg.")))
				{
					FString Value;
					Pair.Value->TryGetString(Value);
					Parts.Add(FString::Printf(TEXT("\"%s\": \"%s\""), *Pair.Key.RightChop(4), *Value));
				}
			}
			Args = Parts.Num() == 0 ? TEXT("{}") : TEXT("{ ") + FString::Join(Parts, TEXT(", ")) + TEXT(" }");
		}

		FString ResultText;
		bool bFound = false;
		for (const TSharedPtr<FJsonObject>& Result : Results)
		{
			FString ResultCallId;
			Result->TryGetStringField(TEXT("call_id"), ResultCallId);
			if (ResultCallId == CallId)
			{
				Result->TryGetStringField(TEXT("text"), ResultText);
				bFound = true;
				break;
			}
		}
		AddCard(Name, Args, bFound ? &ResultText : nullptr);
	}

	return Rows;
}

#undef LOCTEXT_NAMESPACE
