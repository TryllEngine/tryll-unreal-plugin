// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "STryllSystemInspectorView.h"

#include "TryllAgentLogInspection.h"
#include "UI/TryllEditorStyle.h"

#include "Editor.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "STryllSystemInspectorView"

void STryllSystemInspectorView::Construct(const FArguments& /*InArgs*/)
{
	ChildSlot
	[
		SAssignNew(Body, SScrollBox)
	];
	Render(FTryllSystemInspectionFactory::Empty());
}

void STryllSystemInspectorView::Render(const FTryllSystemInspection& InSystem, const FString& RawJson)
{
	System = InSystem;
	Body->ClearChildren();

	Body->AddSlot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 8, 8, 4)
		[
			SNew(STextBlock)
			.Text(FText::FromString(System.Title))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Txt))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
		[
			SNew(STextBlock)
			.Text(FText::FromString(System.Hint))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
			.AutoWrapText(true)
		]
	];

	const TSharedRef<SVerticalBox> Details = SNew(SVerticalBox);
	if (!System.EventLabel.IsEmpty() && System.EventLabel != System.Title)
	{
		Details->AddSlot().AutoHeight()[BuildField(TEXT("Event"), System.EventLabel)];
	}
	if (!System.Detail.IsEmpty())
	{
		Details->AddSlot().AutoHeight()[BuildField(TEXT("Detail"), System.Detail)];
	}
	if (!System.BeforeAfter.IsEmpty())
	{
		Details->AddSlot().AutoHeight()[BuildField(TEXT("Before / after"), System.BeforeAfter)];
	}
	if (!System.AckSummary.IsEmpty())
	{
		Details->AddSlot().AutoHeight()[BuildField(TEXT("Ack / error"), System.AckSummary)];
	}
	if (!System.AbsenceNote.IsEmpty() && System.AbsenceNote != System.Detail)
	{
		Details->AddSlot().AutoHeight()[BuildField(TEXT("Notes"), System.AbsenceNote)];
	}
	if (Details->NumSlots() == 0)
	{
		Details->AddSlot().AutoHeight()[BuildMuted(TEXT("—"))];
	}

	Body->AddSlot()
	[
		BuildSection(TEXT("Details"), true, Details)
	];

	const FString Json = RawJson;
	Body->AddSlot()
	[
		BuildSection(
			TEXT("Raw JSON"),
			false,
			SNew(STextBlock)
				.Text(FText::FromString(Json.IsEmpty() ? TEXT("—") : Json))
				.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
				.AutoWrapText(true),
			[Json]() { return Json; })
	];
}

TSharedRef<SWidget> STryllSystemInspectorView::BuildField(const FString& Label, const FString& Value) const
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 2)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Label))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Value.IsEmpty() ? TEXT("—") : Value))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Txt))
			.AutoWrapText(true)
		];
}

TSharedRef<SWidget> STryllSystemInspectorView::BuildSection(
	const FString& Title,
	bool bInitiallyOpen,
	TSharedRef<SWidget> InBody,
	TFunction<FString()> CopyText) const
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
	if (CopyText)
	{
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
				.OnClicked_Lambda([CopyText, WeakLabel]()
				{
					FPlatformApplicationMisc::ClipboardCopy(*CopyText());
					if (TSharedPtr<STextBlock> Pinned = WeakLabel.Pin())
					{
						Pinned->SetText(LOCTEXT("Copied", "copied"));
						FTimerHandle Handle;
						if (GEditor != nullptr)
						{
							GEditor->GetTimerManager()->SetTimer(Handle, [WeakLabel]()
							{
								if (TSharedPtr<STextBlock> Later = WeakLabel.Pin())
								{
									Later->SetText(LOCTEXT("Copy", "copy"));
								}
							}, 0.9f, false);
						}
					}
					return FReply::Handled();
				})
				[
					Label
				]
			]
		];
	}

	TSharedRef<bool> bOpen = MakeShared<bool>(bInitiallyOpen);
	TSharedRef<SWidget> HeaderBar = SNew(SBorder)
		.BorderImage(FTryllEditorStyle::Rounded(0.0f))
		.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::SectionHeader))
		.Padding(FMargin(10.0f, 4.0f))
		.OnMouseButtonDown_Lambda([bOpen](const FGeometry&, const FPointerEvent&)
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
					.Text_Lambda([bOpen]()
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
				.Visibility_Lambda([bOpen]()
				{
					return *bOpen ? EVisibility::Visible : EVisibility::Collapsed;
				})
				[
					InBody
				]
			]
		];
}

TSharedRef<SWidget> STryllSystemInspectorView::BuildMuted(const FString& Text) const
{
	return SNew(STextBlock)
		.Text(FText::FromString(Text))
		.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
		.AutoWrapText(true);
}

#undef LOCTEXT_NAMESPACE
