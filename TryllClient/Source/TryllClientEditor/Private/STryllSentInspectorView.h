// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "TryllSentInspection.h"

class SScrollBox;

/** Simple sent-row inspector (R12.5 / R12.6). */
class STryllSentInspectorView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STryllSentInspectorView) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void Render(const FTryllSentInspection& Sent);

private:
	TSharedRef<SWidget> BuildMuted(const FString& Text) const;
	TSharedRef<SWidget> BuildCodeBlock(const FString& Text) const;

	TSharedPtr<SScrollBox> Body;
	FTryllSentInspection Sent;
};
