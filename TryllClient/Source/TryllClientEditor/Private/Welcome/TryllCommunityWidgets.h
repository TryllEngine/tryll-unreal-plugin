// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

class SWidget;
class SWrapBox;

/**
 * Small Slate builders shared by the Welcome and Help & Community tabs, the Unreal
 * side of Unity's TryllCommunityView: text, buttons, link chips, status pills, cards
 * and the brand header. Colours come from FTryllEditorStyle roles only.
 */
namespace TryllCommunityWidgets
{
enum class ETone : uint8 { Ok, Warn, Neutral };

TSharedRef<SWidget> Paragraph(const FString& Value, const FLinearColor& Color);
TSharedRef<SWidget> SectionLabel(const FString& Value);

/** Filled accent button for the one main action in a group. */
TSharedRef<SWidget> PrimaryButton(const TCHAR* Label, TFunction<void()> OnClick, const TCHAR* Tag = nullptr);

/** Host button for every other action. */
TSharedRef<SWidget> SecondaryButton(const TCHAR* Label, TFunction<void()> OnClick, const TCHAR* Tag = nullptr,
	const FString& Tooltip = FString());

/** Accent-coloured text: small, low-weight actions. */
TSharedRef<SWidget> LinkButton(const TCHAR* Label, TFunction<void()> OnClick, const TCHAR* Tag = nullptr,
	const FString& Tooltip = FString());

/** Rounded chip that opens Url; the tooltip shows the address. */
TSharedRef<SWidget> LinkChip(const FString& Label, const FString& Url);

TSharedRef<SWidget> Pill(const FString& Value, ETone Tone);

/** Bordered panel with a section label on top. */
TSharedRef<SWidget> Card(const FString& Header, const TSharedRef<SWidget>& Content);

/** Wrapping row with the shared gap. */
TSharedRef<SWrapBox> Row();

/**
 * The Tryll client plugin's VersionName, or empty. An unstamped in-tree plugin (0.0.0)
 * reports the repo-root VERSION instead, like Unity's TryllVersionLookup.Client().
 */
FString ClientVersion();

/** Logo band with the client version pill. bOutLogoShown says whether the logo brush exists. */
TSharedRef<SWidget> BrandHeader(bool& bOutLogoShown);
} // namespace TryllCommunityWidgets
