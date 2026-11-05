// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SButton;
class SMultiLineEditableTextBox;

/**
 * Read-only view of the plugin's LICENSE.md (FTryllLicence), opened by "Read the
 * licence" in the Welcome tab. The text shows inside the editor, so no .md file
 * association is needed; it scrolls and can be selected and copied. Offers
 * "Accept licence" until the licence is accepted. Follows
 * docs/dev/editor-window-checklist.md: theme roles only, copy from
 * FTryllEditorUiContract, and a debug surface for headless tests.
 */
class STryllLicenceViewer : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STryllLicenceViewer) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~STryllLicenceViewer() override;

	/** Open the viewer window, or bring the open one to the front. */
	static void Open();

	/** Test seam: called instead of opening the window. */
	static TFunction<void()> DebugOpenForTests;

	FString DebugTextForTests() const;
	bool DebugAcceptVisibleForTests() const;
	/** Press "Accept licence" as a click would. */
	void DebugAcceptForTests() { OnAcceptClicked(); }

private:
	void Refresh();
	FReply OnAcceptClicked();

	TSharedPtr<SMultiLineEditableTextBox> TextBox;
	TSharedPtr<SButton> AcceptButton;
	FDelegateHandle LicenceChangedHandle;
};
