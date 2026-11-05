// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "TryllStatusPresentation.h"

/**
 * Inline banner. Severity is explicit — never inferred from the calling panel.
 */
class STryllBanner : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STryllBanner) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void Bind(const FString& Text, ETryllStatusSeverity Severity);
	void Clear();

	/** Shown text, empty when cleared. */
	FString DebugText() const
	{
		return GetBannerVisibility() == EVisibility::Visible ? Caption : FString();
	}

private:
	FText GetLabelText() const;
	FSlateColor GetForeground() const;
	FSlateColor GetBackground() const;
	EVisibility GetBannerVisibility() const;

	FString Caption;
	ETryllStatusSeverity Severity = ETryllStatusSeverity::Neutral;
};
