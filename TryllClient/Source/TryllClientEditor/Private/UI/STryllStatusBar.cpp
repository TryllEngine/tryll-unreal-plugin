// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "UI/STryllStatusBar.h"
#include "UI/TryllEditorStyle.h"
#include "UI/TryllPanelSnapshot.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Types/ISlateMetaData.h"

#define LOCTEXT_NAMESPACE "STryllStatusBar"

void STryllStatusBar::Construct(const FArguments& InArgs)
{
	OnAction = InArgs._OnAction;

	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
		[
			SNew(STextBlock)
			.Text(this, &STryllStatusBar::GetCueText)
			.ColorAndOpacity(this, &STryllStatusBar::GetSeverityColor)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.AddMetaData<FTagMetaData>(FTagMetaData(TryllEditorWidgetIds::StatusCaption))
			.Text(this, &STryllStatusBar::GetCaptionText)
			.ColorAndOpacity(this, &STryllStatusBar::GetSeverityColor)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0, 0, 0)
		[
			SNew(SButton)
			.AddMetaData<FTagMetaData>(FTagMetaData(TryllEditorWidgetIds::StatusAction))
			.Visibility(this, &STryllStatusBar::GetActionVisibility)
			.OnClicked(this, &STryllStatusBar::OnActionClicked)
			[
				SNew(STextBlock)
				.Text(this, &STryllStatusBar::GetActionLabel)
			]
		]
	];
}

void STryllStatusBar::Bind(const FTryllStatusPresentation& Presentation)
{
	Current = Presentation;
}

FText STryllStatusBar::GetCueText() const
{
	const bool bFilled = Current.Severity == ETryllStatusSeverity::Success
		|| Current.Severity == ETryllStatusSeverity::Error
		|| Current.Severity == ETryllStatusSeverity::Warning;
	return FText::FromString(bFilled ? TEXT("*") : TEXT("o"));
}

FText STryllStatusBar::GetCaptionText() const
{
	return FText::FromString(Current.Caption);
}

FSlateColor STryllStatusBar::GetSeverityColor() const
{
	switch (Current.Severity)
	{
	case ETryllStatusSeverity::Success:
		return FSlateColor(FTryllEditorStyle::ColOk);
	case ETryllStatusSeverity::Error:
		return FSlateColor(FTryllEditorStyle::ColErr);
	case ETryllStatusSeverity::Warning:
		return FSlateColor(FTryllEditorStyle::ColWarn);
	default:
		return FSlateColor(FTryllEditorStyle::Dim);
	}
}

FText STryllStatusBar::GetActionLabel() const
{
	switch (Current.Action)
	{
	case ETryllStatusActionIntent::Retry:
		return LOCTEXT("Retry", "Retry");
	case ETryllStatusActionIntent::Start:
		return LOCTEXT("Start", "Start");
	case ETryllStatusActionIntent::Stop:
		return LOCTEXT("Stop", "Stop");
	default:
		return FText::GetEmpty();
	}
}

EVisibility STryllStatusBar::GetActionVisibility() const
{
	return Current.Action != ETryllStatusActionIntent::None && OnAction.IsBound()
		? EVisibility::Visible
		: EVisibility::Collapsed;
}

FReply STryllStatusBar::OnActionClicked()
{
	OnAction.ExecuteIfBound();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
