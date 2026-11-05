// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/SlateBrush.h"
#include "TryllEditorUiContract.h"

class SWidget;

/**
 * Host / semantic / component roles for Tryll editor panels.
 * Surfaces resolve through FStyleColors so they track the Unreal theme.
 * Semantic RGB is authored here only (via Srgb).
 */
struct TRYLLCLIENTEDITOR_API FTryllEditorStyle
{
	static float SrgbChannelToLinear(float Channel)
	{
		return Channel <= 0.04045f
			? Channel / 12.92f
			: FMath::Pow((Channel + 0.055f) / 1.055f, 2.4f);
	}

	static FLinearColor Srgb(float R, float G, float B, float A = 1.0f)
	{
		return FLinearColor(
			SrgbChannelToLinear(R), SrgbChannelToLinear(G), SrgbChannelToLinear(B), A);
	}

	static FLinearColor Hex(const TCHAR* Value)
	{
		const FColor Bytes = FColor::FromHex(Value);
		return Srgb(Bytes.R / 255.0f, Bytes.G / 255.0f, Bytes.B / 255.0f, Bytes.A / 255.0f);
	}

	static FLinearColor Wash(const FLinearColor& Fg, float Alpha)
	{
		return FLinearColor(Fg.R, Fg.G, Fg.B, Alpha);
	}

	static void RefreshHostColors();
	static void BindEditorTheme();
	static void UnbindEditorTheme();

	/** Fired after host surfaces are re-sampled so open panels can rebuild baked colors. */
	static FSimpleMulticastDelegate OnHostColorsRefreshed;

	// ── Host surfaces (FStyleColors; refreshed on theme change) ───────────────
	static FLinearColor Bg;
	static FLinearColor Panel;
	static FLinearColor Panel2;
	static FLinearColor Line;
	static FLinearColor Line2;
	static FLinearColor Txt;
	static FLinearColor Dim;
	static FLinearColor Faint;
	static FLinearColor CodeBg;
	static FLinearColor AsstBub;
	static FLinearColor SelBub;
	static FLinearColor SectionHeader;
	static FLinearColor SectionBody;
	static FLinearColor SegmentOnBg;
	static FLinearColor SegmentOnText;
	static FLinearColor CellBody;
	static FLinearColor Hairline;
	static FLinearColor HeaderBg;
	static FLinearColor PanelTint;
	static FLinearColor TrackBg;

	// ── Semantic (authored sRGB) ──────────────────────────────────────────────
	static inline const FLinearColor Accent  = Srgb(0.306f, 0.631f, 1.000f);
	static inline const FLinearColor ColOk   = Srgb(0.306f, 0.769f, 0.478f);
	static inline const FLinearColor ColWarn = Srgb(0.878f, 0.663f, 0.235f);
	static inline const FLinearColor ColErr  = Srgb(0.878f, 0.396f, 0.369f);
	static inline const FLinearColor Loaded  = Srgb(0.690f, 0.420f, 1.000f);
	static inline const FLinearColor DiffAdded   = Srgb(0.620f, 0.906f, 0.722f);
	static inline const FLinearColor DiffRemoved = Srgb(0.950f, 0.650f, 0.620f);
	static inline const FLinearColor DiffSame    = Srgb(0.784f, 0.800f, 0.831f);

	static inline const FLinearColor UserBub = Srgb(0.220f, 0.400f, 0.620f);

	static inline const FLinearColor OkBg     = Wash(ColOk, 0.18f);
	static inline const FLinearColor WarnBg   = Wash(ColWarn, 0.18f);
	static inline const FLinearColor ErrBg    = Wash(ColErr, 0.18f);
	static inline const FLinearColor ErrCell  = Wash(ColErr, 0.22f);
	static inline const FLinearColor SkipCell = Wash(Srgb(0.420f, 0.440f, 0.475f), 0.20f);
	static inline const FLinearColor OkBorder   = Wash(ColOk, 0.45f);
	static inline const FLinearColor WarnBorder = Wash(ColWarn, 0.45f);
	static inline const FLinearColor ErrBorder  = Wash(ColErr, 0.45f);
	static inline const FLinearColor ChipBg     = FLinearColor(0.0f, 0.0f, 0.0f, 0.22f);

	static inline const FLinearColor BandText    = Srgb(0.902f, 0.933f, 0.969f);
	static inline const FLinearColor SummaryUser = Srgb(0.812f, 0.878f, 0.949f);

	/**
	 * Positional variant colours, the same five as Unity's TryllEditorTheme.VariantCycle. None is
	 * green or amber, so a variant cannot be mistaken for a pass/fail verdict.
	 */
	static FLinearColor VariantSwatch(int32 Index)
	{
		static const FLinearColor Cycle[] =
		{
			Srgb(0.224f, 0.529f, 0.898f), // blue    #3987e5
			Srgb(0.565f, 0.522f, 0.914f), // violet  #9085e9
			Srgb(0.835f, 0.318f, 0.506f), // magenta #d55181
			Srgb(0.851f, 0.349f, 0.149f), // orange  #d95926
			Srgb(0.098f, 0.620f, 0.439f), // aqua    #199e70
		};
		const int32 Count = UE_ARRAY_COUNT(Cycle);
		return Cycle[((Index % Count) + Count) % Count];
	}

	static constexpr float LineHeight = 17.0f;
	static constexpr float Gap        = 8.0f;

	static constexpr float CellWidthSmall  = FTryllEditorUiContract::CellWidthSmall;
	static constexpr float CellWidthMedium = FTryllEditorUiContract::CellWidthMedium;
	static constexpr float CellWidthLarge  = FTryllEditorUiContract::CellWidthLarge;
	static constexpr float InspectorMinWidth     = FTryllEditorUiContract::InspectorMinWidth;
	static constexpr float InspectorDefaultWidth = FTryllEditorUiContract::InspectorDefaultWidth;
	static constexpr float ChatMinWidth          = FTryllEditorUiContract::ChatMinWidth;
	static constexpr float ChatBubbleWidthFraction = 0.9f;
	static constexpr float StageMinWidth         = FTryllEditorUiContract::StageMinWidth;
	static constexpr float PanelMinWidth         = FTryllEditorUiContract::PanelMinWidth;
	static constexpr float PanelMinHeight        = FTryllEditorUiContract::PanelMinHeight;
	static constexpr int32 DensityLines[3] = {
		FTryllEditorUiContract::DensityLines[0],
		FTryllEditorUiContract::DensityLines[1],
		FTryllEditorUiContract::DensityLines[2],
	};

	static constexpr float RadiusChip   = 8.0f;
	static constexpr float RadiusBand   = 8.0f;
	static constexpr float RadiusCell   = 6.0f;
	static constexpr float RadiusCard   = 4.0f;
	static constexpr float RadiusHeader = 3.0f;
	static constexpr float RadiusDot    = 3.0f;
	static constexpr float RadiusBar    = 2.0f;

	static const FSlateBrush* Rounded(
		float Radius,
		const FLinearColor& Outline = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f),
		float OutlineWidth = 0.0f)
	{
		return RoundedCorners(
			FVector4(Radius, Radius, Radius, Radius), Outline, OutlineWidth);
	}

	static const FSlateBrush* RoundedCorners(
		const FVector4& Radii,
		const FLinearColor& Outline = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f),
		float OutlineWidth = 0.0f)
	{
		struct FEntry
		{
			FVector4 Radii;
			FLinearColor Outline;
			float Width;
			TSharedPtr<FSlateRoundedBoxBrush> Brush;
		};
		static TArray<FEntry> Cache;

		for (const FEntry& Entry : Cache)
		{
			if (Entry.Radii.Equals(Radii)
				&& FMath::IsNearlyEqual(Entry.Width, OutlineWidth)
				&& Entry.Outline.Equals(Outline))
			{
				return Entry.Brush.Get();
			}
		}

		FEntry Added;
		Added.Radii   = Radii;
		Added.Outline = Outline;
		Added.Width   = OutlineWidth;
		Added.Brush   = MakeShared<FSlateRoundedBoxBrush>(
			FSlateColor(FLinearColor::White), Radii, FSlateColor(Outline), OutlineWidth);
		Cache.Add(Added);
		return Cache.Last().Brush.Get();
	}

	static FLinearColor NodeColor(const FString& Type)
	{
		if (Type == TEXT("RetrieveNode"))          { return Srgb(0.306f, 0.631f, 1.000f); }
		if (Type == TEXT("TransformNode"))         { return Srgb(0.420f, 0.440f, 0.475f); }
		if (Type == TEXT("GenerateNode"))          { return Srgb(0.690f, 0.420f, 1.000f); }
		if (Type == TEXT("CannedResponseNode"))    { return Srgb(0.306f, 0.769f, 0.478f); }
		if (Type == TEXT("BranchNode"))            { return Srgb(0.700f, 0.620f, 0.180f); }
		if (Type == TEXT("GenerateAndSpeakNode"))  { return Srgb(0.831f, 0.451f, 0.925f); }
		if (Type == TEXT("ToolCallNode"))          { return Srgb(0.235f, 0.706f, 0.729f); }
		if (Type == TEXT("RegexGuardrailNode"))    { return Srgb(0.906f, 0.541f, 0.318f); }
		if (Type == TEXT("ClassifyIntentNode"))    { return Srgb(0.478f, 0.549f, 0.902f); }
		if (Type == TEXT("ClassifyIntentLLMNode")) { return Srgb(0.612f, 0.510f, 0.929f); }
		if (Type == TEXT("ClassifyAttributesLLMNode")) { return Srgb(0.545f, 0.455f, 0.835f); }
		if (Type == TEXT("IntentToInstructionNode")) { return Srgb(0.400f, 0.635f, 0.851f); }
		if (Type == TEXT("InstructionNode"))       { return Srgb(0.541f, 0.667f, 0.549f); }
		if (Type == TEXT("SpeakNode"))             { return Srgb(0.851f, 0.400f, 0.612f); }
		if (Type == TEXT("PauseNode"))             { return Srgb(0.514f, 0.514f, 0.565f); }
		return Line2;
	}

	static int32 EstimateLines(const FString& Text, float CellWidth)
	{
		const float CharsPerLine = FMath::Max(8.0f, (CellWidth - 24.0f) / 6.2f);
		return FMath::CeilToInt(Text.Len() / CharsPerLine);
	}

	/** Logo art is 2x: 480x129 px, drawn at 240x64.5. */
	static constexpr float LogoWidth = 240.0f;

	/**
	 * The Tryll Engine logo with light text, for the dark editor theme
	 * (Resources/TryllLogoOnDark.png in the TryllClient plugin). Null when the
	 * file is missing, so a window still builds without it.
	 */
	static const FSlateBrush* LogoOnDark();

	static TSharedRef<SWidget> Heading(const FText& Title);
	static TSharedRef<SWidget> SectionBox(const FText& Title);
	static TSharedRef<SWidget> Chip(
		const FText& Text, const FLinearColor& Fg, const FLinearColor& Background, const FLinearColor& Border);
	static TSharedRef<SWidget> Placeholder(const FText& Text);

	/** Linked S | M | L style control. Off segments paint `Bg`, not the default dark button. */
	static TSharedRef<SWidget> Segmented(
		const TArray<FText>& Labels,
		TFunction<int32()> GetActive,
		TFunction<void(int32)> SetActive,
		TFunction<bool()> IsEnabled = {});
};
