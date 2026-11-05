// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "UI/TryllEditorStyle.h"

#include "Brushes/SlateDynamicImageBrush.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/StyleColors.h"
#include "Styling/WidgetStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

FLinearColor FTryllEditorStyle::Bg;
FLinearColor FTryllEditorStyle::Panel;
FLinearColor FTryllEditorStyle::Panel2;
FLinearColor FTryllEditorStyle::Line;
FLinearColor FTryllEditorStyle::Line2;
FLinearColor FTryllEditorStyle::Txt;
FLinearColor FTryllEditorStyle::Dim;
FLinearColor FTryllEditorStyle::Faint;
FLinearColor FTryllEditorStyle::CodeBg;
FLinearColor FTryllEditorStyle::AsstBub;
FLinearColor FTryllEditorStyle::SelBub;
FLinearColor FTryllEditorStyle::SectionHeader;
FLinearColor FTryllEditorStyle::SectionBody;
FLinearColor FTryllEditorStyle::SegmentOnBg;
FLinearColor FTryllEditorStyle::SegmentOnText;
FLinearColor FTryllEditorStyle::CellBody;
FLinearColor FTryllEditorStyle::Hairline;
FLinearColor FTryllEditorStyle::HeaderBg;
FLinearColor FTryllEditorStyle::PanelTint;
FLinearColor FTryllEditorStyle::TrackBg;
FSimpleMulticastDelegate FTryllEditorStyle::OnHostColorsRefreshed;

namespace
{
	FDelegateHandle GEditorThemeHandle;

	FLinearColor Host(const FSlateColor& Color)
	{
		return Color.GetColor(FWidgetStyle());
	}

	FLinearColor Lerp(const FLinearColor& A, const FLinearColor& B, float Alpha)
	{
		return A + (B - A) * Alpha;
	}

	void HandleEditorThemeChanged(FGuid)
	{
		FTryllEditorStyle::RefreshHostColors();
		FTryllEditorStyle::OnHostColorsRefreshed.Broadcast();
	}
}

void FTryllEditorStyle::BindEditorTheme()
{
	if (GEditorThemeHandle.IsValid())
	{
		return;
	}

	GEditorThemeHandle = USlateThemeManager::Get().OnThemeChanged().AddStatic(&HandleEditorThemeChanged);
}

void FTryllEditorStyle::UnbindEditorTheme()
{
	if (!GEditorThemeHandle.IsValid())
	{
		return;
	}

	if (UObjectInitialized())
	{
		USlateThemeManager::Get().OnThemeChanged().Remove(GEditorThemeHandle);
	}
	GEditorThemeHandle.Reset();
}

void FTryllEditorStyle::RefreshHostColors()
{
	const FLinearColor Foreground = Host(FStyleColors::Foreground);
	Bg     = Host(FStyleColors::Background);
	Panel  = Host(FStyleColors::Recessed);
	Panel2 = Host(FStyleColors::Header);
	Line   = Host(FStyleColors::InputOutline);
	Line2  = Lerp(Host(FStyleColors::InputOutline), Foreground, 0.25f);
	Txt    = Host(FStyleColors::ForegroundHeader);
	Dim    = Lerp(Foreground, Bg, 0.35f);
	Faint  = Lerp(Foreground, Bg, 0.55f);
	CodeBg = Host(FStyleColors::Input);
	AsstBub = Lerp(Bg, Txt, 0.14f);
	SelBub  = Lerp(AsstBub, Accent, 0.22f);
	SectionHeader = Host(FStyleColors::Panel);
	SectionBody   = Bg;
	SegmentOnBg   = Host(FStyleColors::Select);
	SegmentOnText = Lerp(FLinearColor::White, Accent, 0.12f);
	CellBody  = Txt;
	Hairline  = FLinearColor(0.0f, 0.0f, 0.0f, 0.30f);
	HeaderBg  = FLinearColor(1.0f, 1.0f, 1.0f, 0.05f);
	PanelTint = FLinearColor(1.0f, 1.0f, 1.0f, 0.03f);
	TrackBg   = FLinearColor(1.0f, 1.0f, 1.0f, 0.10f);
}

const FSlateBrush* FTryllEditorStyle::LogoOnDark()
{
	// Made on first use and kept for the editor session; the path never changes.
	// Dynamic: the renderer only preloads brushes of registered style sets, so a
	// plain FSlateImageBrush built here has no texture and draws as a white box.
	static TSharedPtr<FSlateDynamicImageBrush> Brush;
	static bool bResolved = false;
	if (!bResolved)
	{
		bResolved = true;
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("TryllClient"));
		const FString Path = Plugin.IsValid()
			? FPaths::ConvertRelativePathToFull(
				FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources"), TEXT("TryllLogoOnDark.png")))
			: FString();
		if (!Path.IsEmpty() && FPaths::FileExists(Path))
		{
			Brush = MakeShared<FSlateDynamicImageBrush>(FName(*Path), FVector2D(LogoWidth, LogoWidth * 129.0f / 480.0f));
		}
	}
	return Brush.Get();
}

TSharedRef<SWidget> FTryllEditorStyle::Heading(const FText& Title)
{
	return SNew(STextBlock)
		.Text(Title)
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11));
}

TSharedRef<SWidget> FTryllEditorStyle::SectionBox(const FText& Title)
{
	return SNew(STextBlock)
		.Text(Title)
		.ColorAndOpacity(FSlateColor(Dim))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8));
}

TSharedRef<SWidget> FTryllEditorStyle::Chip(
	const FText& Text, const FLinearColor& Fg, const FLinearColor& Background, const FLinearColor& Border)
{
	return SNew(SBorder)
		.BorderImage(Rounded(RadiusChip, Border, 1.0f))
		.BorderBackgroundColor(FSlateColor(Background))
		.Padding(FMargin(6.0f, 1.0f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(Text)
			.ColorAndOpacity(FSlateColor(Fg))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
		];
}

TSharedRef<SWidget> FTryllEditorStyle::Placeholder(const FText& Text)
{
	return SNew(STextBlock)
		.Text(Text)
		.ColorAndOpacity(FSlateColor(Faint))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		.Justification(ETextJustify::Center)
		.AutoWrapText(true);
}

TSharedRef<SWidget> FTryllEditorStyle::Segmented(
	const TArray<FText>& Labels,
	TFunction<int32()> GetActive,
	TFunction<void(int32)> SetActive,
	TFunction<bool()> IsEnabled)
{
	// SButton's default brush is near-black; ButtonColorAndOpacity tints it and
	// the off state stays black. Paint a white rounded fill, then color it.
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < Labels.Num(); ++Index)
	{
		Row->AddSlot().AutoWidth()
		[
			SNew(SBorder)
				.BorderImage(Rounded(RadiusHeader))
				.BorderBackgroundColor_Lambda([GetActive, Index]
				{
					return FSlateColor(GetActive() == Index ? SegmentOnBg : Bg);
				})
				.Padding(FMargin(8.0f, 2.0f))
				[
					SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "NoBorder")
						.Text(Labels[Index])
						.IsEnabled_Lambda([IsEnabled]
						{
							return !IsEnabled || IsEnabled();
						})
						.ForegroundColor_Lambda([GetActive, Index]
						{
							return FSlateColor(GetActive() == Index ? SegmentOnText : Txt);
						})
						.OnClicked_Lambda([SetActive, Index]
						{
							SetActive(Index);
							return FReply::Handled();
						})
				]
		];
	}
	return SNew(SBox).Padding(FMargin(0.0f, 0.0f, 10.0f, 0.0f))[Row];
}
