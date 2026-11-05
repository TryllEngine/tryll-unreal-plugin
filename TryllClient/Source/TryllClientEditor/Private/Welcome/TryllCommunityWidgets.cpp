// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "Welcome/TryllCommunityWidgets.h"

#include "Welcome/TryllCommunityLinks.h"
#include "UI/TryllEditorStyle.h"
#include "TryllDevCheckout.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Types/ISlateMetaData.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

// Named, not anonymous: UBT unity builds merge this file with others in the module.
namespace TryllCommunityWidgetsDetail
{
void AddTag(const TSharedRef<SButton>& Button, const TCHAR* Tag)
{
	if (Tag != nullptr)
	{
		Button->AddMetadata(MakeShared<FTagMetaData>(FName(Tag)));
	}
}
} // namespace TryllCommunityWidgetsDetail

namespace TryllCommunityWidgets
{
using TryllCommunityWidgetsDetail::AddTag;

TSharedRef<SWidget> Paragraph(const FString& Value, const FLinearColor& Color)
{
	return SNew(STextBlock)
		.Text(FText::FromString(Value))
		.ColorAndOpacity(FSlateColor(Color))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
		.AutoWrapText(true);
}

TSharedRef<SWidget> SectionLabel(const FString& Value)
{
	return SNew(STextBlock)
		.Text(FText::FromString(Value))
		.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9));
}

TSharedRef<SWidget> PrimaryButton(const TCHAR* Label, TFunction<void()> OnClick, const TCHAR* Tag)
{
	TSharedRef<SButton> Button = SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
		.Text(FText::FromString(Label))
		.OnClicked_Lambda([OnClick]() { OnClick(); return FReply::Handled(); });
	AddTag(Button, Tag);
	return Button;
}

TSharedRef<SWidget> SecondaryButton(const TCHAR* Label, TFunction<void()> OnClick, const TCHAR* Tag,
	const FString& Tooltip)
{
	TSharedRef<SButton> Button = SNew(SButton)
		.Text(FText::FromString(Label))
		.ToolTipText(FText::FromString(Tooltip))
		.OnClicked_Lambda([OnClick]() { OnClick(); return FReply::Handled(); });
	AddTag(Button, Tag);
	return Button;
}

TSharedRef<SWidget> LinkButton(const TCHAR* Label, TFunction<void()> OnClick, const TCHAR* Tag,
	const FString& Tooltip)
{
	TSharedRef<SButton> Button = SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ToolTipText(FText::FromString(Tooltip))
		.OnClicked_Lambda([OnClick]() { OnClick(); return FReply::Handled(); })
		[
			SNew(STextBlock)
			.Text(FText::FromString(Label))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Accent))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
		];
	AddTag(Button, Tag);
	return Button;
}

TSharedRef<SWidget> LinkChip(const FString& Label, const FString& Url)
{
	return SNew(SButton)
		.Text(FText::FromString(Label))
		.ToolTipText(FText::FromString(Url))
		.OnClicked_Lambda([Url]() { FTryllCommunityLinks::OpenUrl(Url); return FReply::Handled(); });
}

TSharedRef<SWidget> Pill(const FString& Value, ETone Tone)
{
	switch (Tone)
	{
	case ETone::Ok:
		return FTryllEditorStyle::Chip(FText::FromString(Value),
			FTryllEditorStyle::ColOk, FTryllEditorStyle::OkBg, FTryllEditorStyle::OkBorder);
	case ETone::Warn:
		return FTryllEditorStyle::Chip(FText::FromString(Value),
			FTryllEditorStyle::ColWarn, FTryllEditorStyle::WarnBg, FTryllEditorStyle::WarnBorder);
	default:
		return FTryllEditorStyle::Chip(FText::FromString(Value),
			FTryllEditorStyle::Dim, FTryllEditorStyle::ChipBg, FTryllEditorStyle::Line);
	}
}

TSharedRef<SWidget> Card(const FString& Header, const TSharedRef<SWidget>& Content)
{
	return SNew(SBorder)
		.BorderImage(FTryllEditorStyle::Rounded(FTryllEditorStyle::RadiusCell, FTryllEditorStyle::Line, 1.0f))
		.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::Panel))
		.Padding(FMargin(13.0f, 9.0f, 13.0f, 8.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SectionLabel(Header)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				Content
			]
		];
}

TSharedRef<SWrapBox> Row()
{
	return SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6.0f, 6.0f));
}

FString ClientVersion()
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("TryllClient"));
	if (!Plugin.IsValid())
	{
		return FString();
	}
	// A development checkout reports the number the release would stamp.
	const FString RepoVersion = FTryllDevCheckout::RepoVersion();
	return RepoVersion.IsEmpty() ? Plugin->GetDescriptor().VersionName : RepoVersion;
}

TSharedRef<SWidget> BrandHeader(bool& bOutLogoShown)
{
	const FString Version = ClientVersion();
	const FSlateBrush* Logo = FTryllEditorStyle::LogoOnDark();
	bOutLogoShown = Logo != nullptr;

	TSharedRef<SHorizontalBox> Band = SNew(SHorizontalBox);
	if (Logo != nullptr)
	{
		Band->AddSlot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SImage).Image(Logo)
		];
	}
	Band->AddSlot().FillWidth(1.0f)[ SNew(SSpacer) ];
	if (!Version.IsEmpty())
	{
		Band->AddSlot().AutoWidth().VAlign(VAlign_Center)
		[
			Pill(TEXT("v") + Version, ETone::Neutral)
		];
	}
	return SNew(SBorder)
		.BorderImage(FTryllEditorStyle::Rounded(FTryllEditorStyle::RadiusCell))
		.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::HeaderBg))
		.Padding(FMargin(14.0f, 12.0f))
		[
			Band
		];
}
} // namespace TryllCommunityWidgets
