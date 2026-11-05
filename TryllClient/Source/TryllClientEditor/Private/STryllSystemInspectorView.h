// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "TryllSystemInspection.h"

class SScrollBox;

/** Simple system / gap-row inspector (R12.5 / R12.8). */
class STryllSystemInspectorView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STryllSystemInspectorView) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void Render(const FTryllSystemInspection& System, const FString& RawJson = FString());

private:
	TSharedRef<SWidget> BuildSection(
		const FString& Title,
		bool bInitiallyOpen,
		TSharedRef<SWidget> InBody,
		TFunction<FString()> CopyText = nullptr) const;
	TSharedRef<SWidget> BuildField(const FString& Label, const FString& Value) const;
	TSharedRef<SWidget> BuildMuted(const FString& Text) const;

	TSharedPtr<SScrollBox> Body;
	FTryllSystemInspection System;
};
