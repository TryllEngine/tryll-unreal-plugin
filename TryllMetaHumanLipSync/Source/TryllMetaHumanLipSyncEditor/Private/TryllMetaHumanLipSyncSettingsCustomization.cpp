// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllMetaHumanLipSyncSettingsCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "TryllMetaHumanLipSyncSettings.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "TryllMetaHumanLipSync"

TSharedRef<IDetailCustomization> FTryllMetaHumanLipSyncSettingsCustomization::MakeInstance()
{
	return MakeShared<FTryllMetaHumanLipSyncSettingsCustomization>();
}

void FTryllMetaHumanLipSyncSettingsCustomization::CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder)
{
	WeakDetailBuilder = DetailBuilder;
	CustomizeDetails(*DetailBuilder);
}

void FTryllMetaHumanLipSyncSettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	const FText ButtonText = LOCTEXT("SetUpButton", "Set Up");

	IDetailCategoryBuilder& Setup = DetailBuilder.EditCategory(TEXT("Setup"));
	Setup.AddCustomRow(ButtonText)
	.WholeRowContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.Text(ButtonText)
			.ToolTipText(LOCTEXT("SetUpButtonTooltip", "Copy Epic's speech-to-animation model into this project and configure it for CPU inference."))
			.OnClicked_Lambda([WeakBuilder = WeakDetailBuilder]()
			{
				GetMutableDefault<UTryllMetaHumanLipSyncSettings>()->SetUpMetaHumanLipSync();
				if (const TSharedPtr<IDetailLayoutBuilder> Builder = WeakBuilder.Pin())
				{
					Builder->ForceRefreshDetails();
				}
				return FReply::Handled();
			})
		]
	];
}

#undef LOCTEXT_NAMESPACE
