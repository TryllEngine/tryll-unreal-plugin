// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "UI/STryllBanner.h"
#include "UI/TryllEditorStyle.h"
#include "UI/TryllPanelSnapshot.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"
#include "Types/ISlateMetaData.h"

void STryllBanner::Construct(const FArguments& /*InArgs*/)
{
	ChildSlot
	[
		SNew(SBorder)
		.AddMetaData<FTagMetaData>(FTagMetaData(TryllEditorWidgetIds::Banner))
		.Visibility(this, &STryllBanner::GetBannerVisibility)
		.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(this, &STryllBanner::GetBackground)
		.Padding(FMargin(8, 5))
		[
			SNew(STextBlock)
			.Text(this, &STryllBanner::GetLabelText)
			.ColorAndOpacity(this, &STryllBanner::GetForeground)
			.AutoWrapText(true)
		]
	];
}

void STryllBanner::Bind(const FString& Text, const ETryllStatusSeverity InSeverity)
{
	Caption = Text;
	Severity = InSeverity;
	Invalidate(EInvalidateWidgetReason::Layout | EInvalidateWidgetReason::Paint);
}

void STryllBanner::Clear()
{
	Caption.Reset();
	Severity = ETryllStatusSeverity::Neutral;
	Invalidate(EInvalidateWidgetReason::Layout | EInvalidateWidgetReason::Paint);
}

FText STryllBanner::GetLabelText() const
{
	return FText::FromString(Caption);
}

FSlateColor STryllBanner::GetForeground() const
{
	switch (Severity)
	{
	case ETryllStatusSeverity::Error:
		return FSlateColor(FTryllEditorStyle::ColErr);
	case ETryllStatusSeverity::Warning:
		return FSlateColor(FTryllEditorStyle::ColWarn);
	case ETryllStatusSeverity::Success:
		return FSlateColor(FTryllEditorStyle::ColOk);
	default:
		return FSlateColor(FTryllEditorStyle::Dim);
	}
}

FSlateColor STryllBanner::GetBackground() const
{
	switch (Severity)
	{
	case ETryllStatusSeverity::Error:
		return FSlateColor(FTryllEditorStyle::ErrBg);
	case ETryllStatusSeverity::Warning:
		return FSlateColor(FTryllEditorStyle::WarnBg);
	case ETryllStatusSeverity::Success:
		return FSlateColor(FTryllEditorStyle::OkBg);
	default:
		return FSlateColor(FTryllEditorStyle::Panel2);
	}
}

EVisibility STryllBanner::GetBannerVisibility() const
{
	return Caption.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
}
