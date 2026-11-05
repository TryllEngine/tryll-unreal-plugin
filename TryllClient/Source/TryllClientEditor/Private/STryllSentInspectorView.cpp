// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "STryllSentInspectorView.h"

#include "TryllAgentLogInspection.h"
#include "UI/TryllEditorStyle.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "STryllSentInspectorView"

void STryllSentInspectorView::Construct(const FArguments& /*InArgs*/)
{
	ChildSlot
	[
		SAssignNew(Body, SScrollBox)
	];
	Render(FTryllSentInspectionFactory::Empty());
}

void STryllSentInspectorView::Render(const FTryllSentInspection& InSent)
{
	Sent = InSent;
	Body->ClearChildren();

	Body->AddSlot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 8, 8, 4)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Sent.Title))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Txt))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Sent.Hint))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 4)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("PayloadLabel", "Payload"))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
		[
			BuildCodeBlock(Sent.PayloadSummary)
		]
	];

	if (!Sent.PayloadDetail.IsEmpty())
	{
		Body->AddSlot().Padding(8, 0, 8, 8)
		[
			BuildCodeBlock(Sent.PayloadDetail)
		];
	}

	if (Sent.DeferredVariableBatches.Num() > 0)
	{
		Body->AddSlot().Padding(8, 0, 8, 4)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("DeferredVars", "Deferred variable flush"))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
		];

		for (const FTryllDeferredVariableBatch& Batch : Sent.DeferredVariableBatches)
		{
			Body->AddSlot().Padding(8, 0, 8, 4)
			[
				BuildMuted(Batch.Summary)
			];
		}
	}

	if (Sent.JoinedAck.IsSet())
	{
		const FTryllObservationEnvelope& Ack = Sent.JoinedAck.GetValue();
		FString AckText = FTryllAgentLogInspection::FormatWireLabel(Ack);
		if (!Ack.Message.IsEmpty())
		{
			AckText += TEXT(": ") + Ack.Message;
		}
		if (Ack.Code != 0)
		{
			AckText += FString::Printf(TEXT(" (code %u)"), Ack.Code);
		}

		Body->AddSlot().Padding(8, 4, 8, 8)
		[
			SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("AckFmt", "Correlated response: {0}"), FText::FromString(AckText)))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
			.AutoWrapText(true)
		];
	}

	if (!Sent.AbsenceNote.IsEmpty() && !Sent.bHasDetailBeyondRow)
	{
		Body->AddSlot().Padding(8, 4, 8, 8)
		[
			BuildMuted(Sent.AbsenceNote)
		];
	}
}

TSharedRef<SWidget> STryllSentInspectorView::BuildMuted(const FString& Text) const
{
	return SNew(STextBlock)
		.Text(FText::FromString(Text))
		.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
		.AutoWrapText(true);
}

TSharedRef<SWidget> STryllSentInspectorView::BuildCodeBlock(const FString& Text) const
{
	return SNew(SBorder)
		.BorderImage(FTryllEditorStyle::Rounded(
			FTryllEditorStyle::RadiusHeader, FTryllEditorStyle::Line, 1.0f))
		.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::CodeBg))
		.Padding(FMargin(8, 6))
		[
			SNew(STextBlock)
			.Text(FText::FromString(Text))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::DiffSame))
			.AutoWrapText(true)
		];
}

#undef LOCTEXT_NAMESPACE
