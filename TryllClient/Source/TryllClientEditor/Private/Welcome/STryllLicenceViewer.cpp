// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "Welcome/STryllLicenceViewer.h"

#include "TryllEditorUiContract.h"
#include "TryllLicence.h"
#include "UI/TryllEditorStyle.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

// Named, not anonymous: UBT unity builds merge this file with others in the module.
namespace TryllLicenceViewerPrivate
{
/** The one viewer window; a second "Read the licence" focuses it. */
TWeakPtr<SWindow> OpenWindow;
} // namespace TryllLicenceViewerPrivate

TFunction<void()> STryllLicenceViewer::DebugOpenForTests;

void STryllLicenceViewer::Open()
{
	using namespace TryllLicenceViewerPrivate;
	if (DebugOpenForTests)
	{
		DebugOpenForTests();
		return;
	}
	if (!FSlateApplication::IsInitialized())
	{
		return;
	}
	if (const TSharedPtr<SWindow> Existing = OpenWindow.Pin())
	{
		Existing->BringToFront();
		return;
	}

	const TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(FText::FromString(FTryllEditorUiContract::WelcomeLicenceViewerTitle))
		.ClientSize(FVector2D(640.0f, 720.0f))
		.MinWidth(420.0f)
		.MinHeight(360.0f)
		.SupportsMaximize(true)
		.SupportsMinimize(false)
		[
			SNew(STryllLicenceViewer)
		];
	OpenWindow = Window;

	const TSharedPtr<SWindow> Parent = FGlobalTabmanager::Get()->GetRootWindow();
	if (Parent.IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(Window, Parent.ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(Window);
	}
}

void STryllLicenceViewer::Construct(const FArguments& /*InArgs*/)
{
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(16.0f, 14.0f, 16.0f, 12.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FTryllEditorUiContract::WelcomeLicenceViewerTitle))
				.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Txt))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
			]
			// A read-only text box: it scrolls, and the text can be selected and copied.
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 0.0f, 0.0f, 10.0f)
			[
				SAssignNew(TextBox, SMultiLineEditableTextBox)
				.IsReadOnly(true)
				.AutoWrapText(true)
				.AlwaysShowScrollbars(false)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)
			[
				SAssignNew(AcceptButton, SButton)
				.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
				.Text(FText::FromString(FTryllEditorUiContract::WelcomeLicenceAccept))
				.OnClicked(this, &STryllLicenceViewer::OnAcceptClicked)
			]
		]
	];

	LicenceChangedHandle = FTryllLicence::OnChanged().AddSP(this, &STryllLicenceViewer::Refresh);
	Refresh();
}

STryllLicenceViewer::~STryllLicenceViewer()
{
	if (LicenceChangedHandle.IsValid())
	{
		FTryllLicence::OnChanged().Remove(LicenceChangedHandle);
		LicenceChangedHandle.Reset();
	}
}

void STryllLicenceViewer::Refresh()
{
	if (!TextBox.IsValid())
	{
		return;
	}
	// Without a LICENSE.md the Welcome tab hides "Read the licence"; a viewer
	// that is already open says why instead of showing nothing.
	TextBox->SetText(FText::FromString(FTryllLicence::HasLicenceFile()
		? FTryllLicence::LoadLicenceText()
		: FString(FTryllEditorUiContract::WelcomeLicencePending)));
	AcceptButton->SetVisibility(FTryllLicence::IsAccepted() ? EVisibility::Collapsed : EVisibility::Visible);
}

FReply STryllLicenceViewer::OnAcceptClicked()
{
	// OnChanged refreshes this viewer and the Welcome tab.
	FTryllLicence::SetAccepted(true);
	return FReply::Handled();
}

FString STryllLicenceViewer::DebugTextForTests() const
{
	return TextBox.IsValid() ? TextBox->GetText().ToString() : FString();
}

bool STryllLicenceViewer::DebugAcceptVisibleForTests() const
{
	return AcceptButton.IsValid() && AcceptButton->GetVisibility() == EVisibility::Visible;
}
