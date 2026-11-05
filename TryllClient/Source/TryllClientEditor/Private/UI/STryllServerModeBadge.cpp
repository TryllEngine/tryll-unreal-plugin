// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "UI/STryllServerModeBadge.h"
#include "UI/TryllEditorStyle.h"
#include "TryllEditorSubsystem.h"
#include "TryllSubsystem.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Editor.h"
#include "Engine/GameInstance.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "STryllServerModeBadge"

namespace
{
bool IsTryllDev(const FString& Caption)
{
	return Caption.Equals(TEXT("tryll-dev"), ESearchCase::IgnoreCase);
}
}

void STryllServerModeBadge::Construct(const FArguments& /*InArgs*/)
{
	ChildSlot
	[
		SNew(SBorder)
		.Visibility(this, &STryllServerModeBadge::GetVisibility)
		.BorderImage(this, &STryllServerModeBadge::GetBrush)
		.Padding(FMargin(6.f, 0.f, 6.f, 1.f))
		.ToolTipText(LOCTEXT("ModeTip", "Deployment mode reported by the connected Tryll server."))
		[
			SNew(STextBlock)
			.Text_Lambda([this] { return FText::FromString(CurrentCaption()); })
			.ColorAndOpacity(this, &STryllServerModeBadge::GetTextColor)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		]
	];
}

FString STryllServerModeBadge::Caption(bool bConnected, const FString& Mode)
{
	return bConnected && !Mode.Equals(TEXT("client-dev"), ESearchCase::IgnoreCase) ? Mode : FString();
}

FString STryllServerModeBadge::CurrentCaption() const
{
	if (GEditor)
	{
		if (const UTryllEditorSubsystem* Sub = GEditor->GetEditorSubsystem<UTryllEditorSubsystem>())
		{
			const FString EditorMode = Caption(Sub->IsConnected(), Sub->ServerMode);
			if (!EditorMode.IsEmpty())
			{
				return EditorMode;
			}
		}
		for (const FWorldContext& Context : GEditor->GetWorldContexts())
		{
			const UGameInstance* Game = Context.OwningGameInstance;
			if (const UTryllSubsystem* Sub = Game ? Game->GetSubsystem<UTryllSubsystem>() : nullptr)
			{
				const FString PieMode = Caption(Sub->IsConnected(), Sub->ServerMode);
				if (!PieMode.IsEmpty())
				{
					return PieMode;
				}
			}
		}
	}
	return FString();
}

EVisibility STryllServerModeBadge::GetVisibility() const
{
	return CurrentCaption().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
}

const FSlateBrush* STryllServerModeBadge::GetBrush() const
{
	static const FSlateRoundedBoxBrush Dev(
		FTryllEditorStyle::Wash(FTryllEditorStyle::ColWarn, 0.14f), 7.f,
		FTryllEditorStyle::Wash(FTryllEditorStyle::ColWarn, 0.45f), 1.f);
	static const FSlateRoundedBoxBrush Other(
		FTryllEditorStyle::Wash(FTryllEditorStyle::Dim, 0.14f), 7.f,
		FTryllEditorStyle::Wash(FTryllEditorStyle::Dim, 0.45f), 1.f);
	return IsTryllDev(CurrentCaption()) ? &Dev : &Other;
}

FSlateColor STryllServerModeBadge::GetTextColor() const
{
	return FSlateColor(IsTryllDev(CurrentCaption()) ? FTryllEditorStyle::ColWarn : FTryllEditorStyle::Dim);
}

#undef LOCTEXT_NAMESPACE
