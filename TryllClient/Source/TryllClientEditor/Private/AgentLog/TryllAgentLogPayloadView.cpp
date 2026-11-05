// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "AgentLog/TryllAgentLogPayloadView.h"

#include "AgentLog/TryllAgentLogPayloadModel.h"
#include "TryllObservationTypes.h"
#include "UI/TryllEditorStyle.h"

#include "HAL/PlatformApplicationMisc.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void STryllAgentLogPayloadView::Construct(const FArguments& /*InArgs*/)
{
	ChildSlot[SNullWidget::NullWidget];
}

void STryllAgentLogPayloadView::Clear()
{
	ChildSlot[SNullWidget::NullWidget];
}

void STryllAgentLogPayloadView::SetSource(const FTryllObservationEnvelope& Envelope)
{
	if (Envelope.Frame.Num() == 0)
	{
		Clear();
		return;
	}

	const FString Json = FTryllAgentLogPayloadModel::WriteJson(Envelope, true);
	ChildSlot
	[
		SNew(SBox)
		.MaxDesiredHeight(280.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(8.0f, 8.0f, 8.0f, 4.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Raw JSON")))
						.ColorAndOpacity(FTryllEditorStyle::Dim)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("Copy")))
						.OnClicked_Lambda([Json]()
						{
							FPlatformApplicationMisc::ClipboardCopy(*Json);
							return FReply::Handled();
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(8.0f, 0.0f, 8.0f, 8.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Json.IsEmpty() ? TEXT("\u2014") : Json))
					.ColorAndOpacity(FTryllEditorStyle::Dim)
					.AutoWrapText(true)
				]
			]
		]
	];
}
