// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "TryllStatusPresentation.h"

DECLARE_DELEGATE(FOnTryllStatusAction);

/**
 * Host renderer for FTryllStatusPresentation. Severity drives a text/dot cue
 * as well as colour — never colour alone.
 */
class STryllStatusBar : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STryllStatusBar) {}
		SLATE_EVENT(FOnTryllStatusAction, OnAction)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void Bind(const FTryllStatusPresentation& Presentation);

	const FString& DebugCaption() const { return Current.Caption; }
	/** Action button text, empty when the button is hidden. */
	FString DebugActionText() const
	{
		return GetActionVisibility() == EVisibility::Visible ? GetActionLabel().ToString() : FString();
	}
	/** Press the action button as a click would. */
	void DebugClickActionForTests() { OnActionClicked(); }

private:
	FText GetCueText() const;
	FText GetCaptionText() const;
	FSlateColor GetSeverityColor() const;
	FText GetActionLabel() const;
	EVisibility GetActionVisibility() const;
	FReply OnActionClicked();

	FTryllStatusPresentation Current;
	FOnTryllStatusAction OnAction;
};
