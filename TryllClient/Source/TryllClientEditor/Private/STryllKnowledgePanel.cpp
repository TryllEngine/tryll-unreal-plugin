// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "STryllKnowledgePanel.h"
#include "TryllEditorPrompts.h"

#include "Knowledge/TryllKnowledgeConfig.h"
#include "Knowledge/TryllKnowledgePresentation.h"
#include "TryllEditorConnectionPolicy.h"
#include "TryllEditorSubsystem.h"
#include "TryllEditorUiContract.h"
#include "TryllGraphDescription.h"
#include "TryllInspectorSplitMath.h"
#include "TryllMkbSnapshot.h"
#include "TryllRuntimeSettings.h"
#include "TryllSchemaCatalog.h"
#include "TryllStatusPresentation.h"
#include "UI/STryllBanner.h"
#include "UI/STryllInspectorSplit.h"
#include "UI/STryllStatusBar.h"
#include "UI/TryllEditorStyle.h"
#include "Types/ISlateMetaData.h"

#include "DesktopPlatformModule.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "IDesktopPlatform.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "STryllKnowledgePanel"

namespace
{
	TSharedRef<SWidget> HairlineRule()
	{
		return SNew(SBox)
			.HeightOverride(1.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::Hairline))
				.Padding(0)
			];
	}

	TSharedRef<SWidget> ColumnHeader(const FText& Title)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(10, 7)
			[
				FTryllEditorStyle::SectionBox(Title)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				HairlineRule()
			];
	}

	FSlateColor SelectionFill(bool bOn)
	{
		return bOn ? FSlateColor(FTryllEditorStyle::SelBub) : FSlateColor(FTryllEditorStyle::Panel2);
	}

	TSharedRef<SWidget> SelectableChrome(
		TFunction<FSlateColor()> GetFill,
		EHorizontalAlignment HAlign,
		const FMargin& ContentPadding,
		const FText& Tooltip,
		FOnClicked OnClicked,
		const TSharedRef<SWidget>& Content)
	{
		return SNew(SBorder)
			.BorderImage(FTryllEditorStyle::Rounded(FTryllEditorStyle::RadiusCard))
			.BorderBackgroundColor_Lambda([GetFill]
			{
				return GetFill();
			})
			.Padding(0)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.HAlign(HAlign)
				.VAlign(VAlign_Center)
				.ContentPadding(ContentPadding)
				.ToolTipText(Tooltip)
				.OnClicked(OnClicked)
				[ Content ]
			];
	}

	TSharedRef<SWidget> ModeButton(
		const FText& Label,
		const FText& Tooltip,
		TFunction<bool()> IsOn,
		FOnClicked OnClicked)
	{
		return SelectableChrome(
			[IsOn] { return SelectionFill(IsOn()); },
			HAlign_Center,
			FMargin(8, 4),
			Tooltip,
			OnClicked,
			SNew(STextBlock)
			.Text(Label)
			.Font_Lambda([IsOn]
			{
				return FCoreStyle::GetDefaultFontStyle(IsOn() ? "Bold" : "Regular", 9);
			})
			.Justification(ETextJustify::Center));
	}

	TSharedRef<SWidget> SelectableRow(const FString& Caption, bool bSelected, FOnClicked OnClicked)
	{
		return SelectableChrome(
			[bSelected] { return SelectionFill(bSelected); },
			HAlign_Fill,
			FMargin(8, 6),
			FText::GetEmpty(),
			OnClicked,
			SNew(STextBlock)
			.Text(FText::FromString(Caption))
			.AutoWrapText(true)
			.Justification(ETextJustify::Left));
	}

	TSharedPtr<FString> FindOption(const TArray<TSharedPtr<FString>>& Options, const FString& Value)
	{
		for (const TSharedPtr<FString>& Option : Options)
		{
			if (Option.IsValid() && *Option == Value)
			{
				return Option;
			}
		}
		return Options.Num() > 0 ? Options[0] : nullptr;
	}
}

void STryllKnowledgePanel::Construct(const FArguments& InArgs)
{
	EditorSubsystemOverride = InArgs._EditorSubsystemOverride;
	bPersistLayout = InArgs._PersistLayout;
	AutofindSettings();
	LoadLayoutConfig();
	InitCatalogOptions();

	ChildSlot
	[
		SNew(SBox)
		.MinDesiredWidth(FTryllEditorStyle::PanelMinWidth)
		.MinDesiredHeight(FTryllEditorStyle::PanelMinHeight)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(8, 6)[ BuildToolbar() ]
			+ SVerticalBox::Slot().AutoHeight().Padding(8, 0)
			[
				SAssignNew(Banner, STryllBanner)
			]
			+ SVerticalBox::Slot().AutoHeight()[ HairlineRule() ]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SNew(STryllInspectorSplit)
				.FixedSide(STryllInspectorSplit::EFixedSide::First)
				.PrimaryMin(FTryllEditorUiContract::KnowledgeStoreMinWidth)
				.SecondaryMin(
					FTryllEditorUiContract::KnowledgeRecordsMinWidth
					+ FTryllInspectorSplitMath::DefaultHandleWidth
					+ FTryllEditorUiContract::KnowledgeInspectorMinWidth)
				.StoredWidth_Lambda([this] { return StoreWidth; })
				.OnStoredWidthChanged_Lambda([this](float NewWidth)
				{
					StoreWidth = NewWidth;
					SaveLayoutConfig();
				})
				.IsFixedPaneVisible(true)
				.PrimaryContent()
				[
					BuildLeft()
				]
				.SecondaryContent()
				[
					SNew(STryllInspectorSplit)
					.FixedSide(STryllInspectorSplit::EFixedSide::Second)
					.PrimaryMin(FTryllEditorUiContract::KnowledgeRecordsMinWidth)
					.SecondaryMin(FTryllEditorUiContract::KnowledgeInspectorMinWidth)
					.StoredWidth_Lambda([this] { return InspectorWidth; })
					.OnStoredWidthChanged_Lambda([this](float NewWidth)
					{
						InspectorWidth = NewWidth;
						SaveLayoutConfig();
					})
					.IsFixedPaneVisible(true)
					.PrimaryContent()
					[
						BuildCenter()
					]
					.SecondaryContent()
					[
						BuildRight()
					]
				]
			]
		]
	];

	Start();
	RefreshLists();
	HostColorsHandle = FTryllEditorStyle::OnHostColorsRefreshed.AddRaw(
		this, &STryllKnowledgePanel::HandleHostColorsRefreshed);
}

STryllKnowledgePanel::~STryllKnowledgePanel()
{
	if (HostColorsHandle.IsValid())
	{
		FTryllEditorStyle::OnHostColorsRefreshed.Remove(HostColorsHandle);
		HostColorsHandle.Reset();
	}
	SaveLayoutConfig();
	if (State != EState::Idle)
	{
		Stop();
	}
}

void STryllKnowledgePanel::InitCatalogOptions()
{
	SchemaOptions.Reset();
	const FTryllSchemaCatalog Catalog = FTryllSchemaCatalog::LoadBundled(Settings.Get());
	for (const FString& Name : Catalog.Names)
	{
		if (!Name.IsEmpty())
		{
			SchemaOptions.Add(MakeShared<FString>(Name));
		}
	}
	if (SchemaOptions.Num() == 0)
	{
		SchemaOptions.Add(MakeShared<FString>(FString(TEXT("npc_memory"))));
	}
	SelectedSchemaOption = FindOption(SchemaOptions, EnsureSchema);
	if (SelectedSchemaOption.IsValid())
	{
		EnsureSchema = *SelectedSchemaOption;
	}

	EmbedOptions = {
		MakeShared<FString>(FString(TEXT("Lazy"))),
		MakeShared<FString>(FString(TEXT("Now"))),
	};
	SelectedEmbedOption = EmbedTiming == ETryllMutableKbEmbedTiming::Now
		? EmbedOptions[1]
		: EmbedOptions[0];

	CompatOptions = {
		MakeShared<FString>(FString(TEXT("Strict"))),
		MakeShared<FString>(FString(TEXT("Allow schema drift"))),
	};
	SelectedCompatOption = Compatibility == ETryllMutableKbCompatibility::AllowSchemaDrift
		? CompatOptions[1]
		: CompatOptions[0];
}

void STryllKnowledgePanel::LoadLayoutConfig()
{
	if (!bPersistLayout)
	{
		return;
	}
	const FTryllKnowledgeConfig Config = FTryllKnowledgeConfig::Load();
	StoreWidth = Config.StoreWidth;
	InspectorWidth = Config.InspectorWidth;
}

void STryllKnowledgePanel::SaveLayoutConfig() const
{
	if (!bPersistLayout)
	{
		return;
	}
	FTryllKnowledgeConfig Config = FTryllKnowledgeConfig::Load();
	if (FTryllInspectorSplitMath::ShouldPersistWidth(
			StoreWidth, FTryllEditorUiContract::KnowledgeStoreMinWidth))
	{
		Config.StoreWidth = StoreWidth;
	}
	if (FTryllInspectorSplitMath::ShouldPersistWidth(
			InspectorWidth, FTryllEditorUiContract::KnowledgeInspectorMinWidth))
	{
		Config.InspectorWidth = InspectorWidth;
	}
	Config.Save();
}

TSharedRef<SWidget> STryllKnowledgePanel::Hint(const FString& Text)
{
	return SNew(STextBlock)
		.Text(FText::FromString(Text))
		.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		.AutoWrapText(true);
}

TSharedRef<SWidget> STryllKnowledgePanel::LabeledField(const FString& Label, const TSharedRef<SWidget>& Field)
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 2)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Label))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			Field
		];
}

TSharedRef<SWidget> STryllKnowledgePanel::SectionBox(const FString& Title, const TSharedRef<SWidget>& Body)
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
		[
			FTryllEditorStyle::SectionBox(FText::FromString(Title.ToUpper()))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
		[
			Body
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			HairlineRule()
		];
}

TSharedRef<SWidget> STryllKnowledgePanel::BuildToolbar()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Title", "Tryll Knowledge"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10, 0, 0, 0)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("LiveNote", "live · session stores"))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).HAlign(HAlign_Right).Padding(10, 0)
		[
			SAssignNew(StatusBar, STryllStatusBar)
			.OnAction_Lambda([this]
			{
				if (State == EState::Error || State == EState::Idle)
				{
					RetryOwner();
				}
			})
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Text(LOCTEXT("Refresh", "Refresh"))
			.ToolTipText(LOCTEXT("RefreshTip", "Reload the session store list and the selected store's records. Knowledge never polls."))
			.IsEnabled(this, &STryllKnowledgePanel::CanRefresh)
			.AddMetaData<FTagMetaData>(FTagMetaData(TryllEditorWidgetIds::Refresh))
			.OnClicked(this, &STryllKnowledgePanel::OnRefreshClicked)
		];
}

TSharedRef<SWidget> STryllKnowledgePanel::BuildLeft()
{
	TSharedRef<SWidget> EnsureBody = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
		[ Hint(TEXT("Create the named store if it is missing. Schema must exist in the bundled catalog.")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
		[
			LabeledField(TEXT("Name"),
				SNew(SEditableTextBox)
				.Text(FText::FromString(EnsureName))
				.ToolTipText(LOCTEXT("EnsureNameTip", "Store name, e.g. npc.memory.nessa"))
				.OnTextChanged_Lambda([this](const FText& Value) { EnsureName = Value.ToString(); }))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
		[
			LabeledField(TEXT("Schema"),
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&SchemaOptions)
				.InitiallySelectedItem(SelectedSchemaOption)
				.ToolTipText(LOCTEXT("EnsureSchemaTip", "Catalog schema. Server validation stays authoritative."))
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
				{
					SelectedSchemaOption = Item;
					if (Item.IsValid())
					{
						EnsureSchema = *Item;
					}
				})
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
				{
					return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : FString()));
				})
				[
					SNew(STextBlock).Text_Lambda([this]
					{
						return FText::FromString(EnsureSchema);
					})
				])
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Text(LOCTEXT("Ensure", "Ensure"))
			.ToolTipText(LOCTEXT("EnsureTip", "Get-or-create this store in the current session."))
			.IsEnabled(this, &STryllKnowledgePanel::IsSessionReady)
			.OnClicked(this, &STryllKnowledgePanel::OnEnsureClicked)
		];

	TSharedRef<SWidget> SessionBody = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
		[ Hint(TEXT("Save or load every store in this session. Paths are storage-root relative.")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
		[
			LabeledField(TEXT("Path"),
				SNew(SEditableTextBox)
				.Text(FText::FromString(GamePath))
				.ToolTipText(LOCTEXT("GamePathTip", "Storage-root-relative TMKB path, e.g. memory/game.tmkb"))
				.OnTextChanged_Lambda([this](const FText& Value) { GamePath = Value.ToString(); }))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
		[
			LabeledField(TEXT("Save id"),
				SNew(SEditableTextBox)
				.Text(FText::FromString(SaveId))
				.ToolTipText(LOCTEXT("SaveIdTip", "Caller save_id stamped beside the snapshot."))
				.OnTextChanged_Lambda([this](const FText& Value) { SaveId = Value.ToString(); }))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
		[
			LabeledField(TEXT("Embed timing"),
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&EmbedOptions)
				.InitiallySelectedItem(SelectedEmbedOption)
				.ToolTipText(LOCTEXT("EmbedTip", "Required on load. Lazy rebuilds vectors on first use; Now embeds immediately."))
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
				{
					SelectedEmbedOption = Item;
					EmbedTiming = (Item.IsValid() && *Item == TEXT("Now"))
						? ETryllMutableKbEmbedTiming::Now
						: ETryllMutableKbEmbedTiming::Lazy;
				})
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
				{
					return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : FString()));
				})
				[
					SNew(STextBlock).Text_Lambda([this]
					{
						return FText::FromString(
							EmbedTiming == ETryllMutableKbEmbedTiming::Now ? TEXT("Now") : TEXT("Lazy"));
					})
				])
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
		[
			LabeledField(TEXT("Compatibility"),
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&CompatOptions)
				.InitiallySelectedItem(SelectedCompatOption)
				.ToolTipText(LOCTEXT("CompatTip", "Strict rejects schema mismatches. Allow schema drift imports with a warning."))
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
				{
					SelectedCompatOption = Item;
					Compatibility = (Item.IsValid() && *Item == TEXT("Allow schema drift"))
						? ETryllMutableKbCompatibility::AllowSchemaDrift
						: ETryllMutableKbCompatibility::Strict;
				})
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
				{
					return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : FString()));
				})
				[
					SNew(STextBlock).Text_Lambda([this]
					{
						return FText::FromString(
							Compatibility == ETryllMutableKbCompatibility::AllowSchemaDrift
								? TEXT("Allow schema drift")
								: TEXT("Strict"));
					})
				])
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 4, 0)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Text(LOCTEXT("SaveGame", "Save game memory"))
				.ToolTipText(LOCTEXT("SaveGameTip", "Write a session TMKB to the storage-root-relative path."))
				.IsEnabled(this, &STryllKnowledgePanel::IsSessionReady)
				.OnClicked(this, &STryllKnowledgePanel::OnSaveGameClicked)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Text(LOCTEXT("LoadGame", "Load game memory"))
				.ToolTipText(LOCTEXT("LoadGameTip", "Replace session stores from that path. Embed timing is required."))
				.IsEnabled(this, &STryllKnowledgePanel::IsSessionReady)
				.OnClicked(this, &STryllKnowledgePanel::OnLoadGameClicked)
			]
		];

	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::PanelTint))
		.Padding(0)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[ ColumnHeader(LOCTEXT("Stores", "STORES")) ]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot().Padding(6, 0)
				[
					SAssignNew(StoreList, SVerticalBox)
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[ HairlineRule() ]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FSlateColor(FLinearColor::Transparent))
				.Padding(FMargin(10, 12))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[ SectionBox(TEXT("Ensure store"), EnsureBody) ]
					+ SVerticalBox::Slot().AutoHeight()
					[ SectionBox(TEXT("Session file"), SessionBody) ]
				]
			]
		];
}

TSharedRef<SWidget> STryllKnowledgePanel::BuildCenter()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(10, 6)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SAssignNew(StatsLabel, STextBlock)
				.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 4, 0)
				[
					ModeButton(
						LOCTEXT("Filter", "Filter"),
						FText::FromString(FTryllEditorUiContract::KnowledgeFilterHint),
						[this] { return Presentation.Mode == ETryllKnowledgeBrowseMode::Filter; },
						FOnClicked::CreateSP(this, &STryllKnowledgePanel::OnFilterModeClicked))
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					ModeButton(
						LOCTEXT("Query", "Query"),
						FText::FromString(FTryllEditorUiContract::KnowledgeQueryHint),
						[this] { return Presentation.Mode == ETryllKnowledgeBrowseMode::Query; },
						FOnClicked::CreateSP(this, &STryllKnowledgePanel::OnQueryModeClicked))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
			[
				SAssignNew(FilterRow, SVerticalBox)
				.Visibility_Lambda([this]
				{
					return Presentation.Mode == ETryllKnowledgeBrowseMode::Filter
						? EVisibility::Visible
						: EVisibility::Collapsed;
				})
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
				[
					LabeledField(TEXT("Filter"),
						SNew(SEditableTextBox)
						.Text(FText::FromString(Presentation.Filter))
						.ToolTipText(FText::FromString(FTryllEditorUiContract::KnowledgeFilterHint))
						.OnTextChanged_Lambda([this](const FText& Value)
						{
							Presentation.Filter = Value.ToString();
							RebuildRecordList();
						}))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[ Hint(FTryllEditorUiContract::KnowledgeFilterHint) ]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
			[
				SAssignNew(QueryRow, SVerticalBox)
				.Visibility_Lambda([this]
				{
					return Presentation.Mode == ETryllKnowledgeBrowseMode::Query
						? EVisibility::Visible
						: EVisibility::Collapsed;
				})
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
				[
					LabeledField(TEXT("Query"),
						SNew(SEditableTextBox)
						.Text(FText::FromString(QueryText))
						.ToolTipText(FText::FromString(FTryllEditorUiContract::KnowledgeQueryHint))
						.OnTextChanged_Lambda([this](const FText& Value) { QueryText = Value.ToString(); }))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
				[
					LabeledField(TEXT("Top K"),
						SNew(SSpinBox<int32>)
						.MinValue(1)
						.MaxValue(64)
						.Value_Lambda([this] { return QueryTopK; })
						.OnValueChanged_Lambda([this](int32 Value) { QueryTopK = FMath::Max(1, Value); })
						.ToolTipText(LOCTEXT("TopKTip", "How many nearest records to return.")))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Text(LOCTEXT("RunQuery", "Run query"))
					.ToolTipText(FText::FromString(FTryllEditorUiContract::KnowledgeQueryHint))
					.IsEnabled(this, &STryllKnowledgePanel::IsSessionReady)
					.OnClicked(this, &STryllKnowledgePanel::OnRunQueryClicked)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[ Hint(FTryllEditorUiContract::KnowledgeQueryHint) ]
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[ HairlineRule() ]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot().Padding(10, 0)
			[
				SAssignNew(RecordList, SVerticalBox)
			]
		];
}

TSharedRef<SWidget> STryllKnowledgePanel::BuildRight()
{
	TSharedRef<SWidget> AddBody = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
		[ Hint(TEXT("Empty Id lets the server generate one. Write upserts that text into the selected store.")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
		[
			LabeledField(TEXT("Id"),
				SAssignNew(UpsertIdBox, SEditableTextBox)
				.Text(FText::FromString(UpsertId))
				.ToolTipText(LOCTEXT("UpsertIdTip", "Record id. Leave empty to generate."))
				.OnTextChanged_Lambda([this](const FText& Value) { UpsertId = Value.ToString(); }))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
		[
			LabeledField(TEXT("Text"),
				SNew(SBox)
				.MinDesiredHeight(72.0f)
				[
					SAssignNew(UpsertTextBox, SMultiLineEditableTextBox)
					.Text(FText::FromString(UpsertText))
					.ToolTipText(LOCTEXT("UpsertTextTip", "Standalone memory sentence stored as the record text."))
					.AutoWrapText(true)
					.OnTextChanged_Lambda([this](const FText& Value) { UpsertText = Value.ToString(); })
				])
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Text(LOCTEXT("Write", "Write"))
			.ToolTipText(LOCTEXT("WriteTip", "Upsert this id/text into the selected store."))
			.IsEnabled(this, &STryllKnowledgePanel::IsSessionReady)
			.OnClicked(this, &STryllKnowledgePanel::OnWriteClicked)
		];

	TSharedRef<SWidget> RemoveBody = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
		[ Hint(TEXT("Delete removes the selected record. Clear wipes every live record in the store.")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Text(LOCTEXT("Delete", "Delete selected"))
			.ToolTipText(LOCTEXT("DeleteTip", "Delete the highlighted record. The store itself remains."))
			.IsEnabled(this, &STryllKnowledgePanel::IsSessionReady)
			.OnClicked(this, &STryllKnowledgePanel::OnDeleteClicked)
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Text(LOCTEXT("Clear", "Clear store"))
			.ToolTipText(LOCTEXT("ClearTip", "Delete every live record. Asks for confirmation."))
			.IsEnabled(this, &STryllKnowledgePanel::IsSessionReady)
			.OnClicked(this, &STryllKnowledgePanel::OnClearClicked)
		];

	TSharedRef<SWidget> SnapBody = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
		[ Hint(TEXT("Export or import one store as a TMKB file. This is not the whole-session save on the left.")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
		[
			LabeledField(TEXT("Hint name"),
				SNew(SEditableTextBox)
				.Text(FText::FromString(StoreExportHint))
				.ToolTipText(LOCTEXT("HintNameTip", "Suggested filename when the save dialog opens."))
				.OnTextChanged_Lambda([this](const FText& Value) { StoreExportHint = Value.ToString(); }))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 4, 0)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Text(LOCTEXT("Export", "Export TMKB"))
				.ToolTipText(LOCTEXT("ExportTip", "Download this store's snapshot to a .tmkb file."))
				.IsEnabled(this, &STryllKnowledgePanel::IsSessionReady)
				.OnClicked(this, &STryllKnowledgePanel::OnExportClicked)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Text(LOCTEXT("Import", "Import TMKB"))
				.ToolTipText(LOCTEXT("ImportTip", "Replace this store from a .tmkb file. Snapshot name must match."))
				.IsEnabled(this, &STryllKnowledgePanel::IsSessionReady)
				.OnClicked(this, &STryllKnowledgePanel::OnImportClicked)
			]
		];

	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::PanelTint))
		.Padding(0)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[ ColumnHeader(LOCTEXT("Modify", "MODIFY")) ]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot().Padding(10, 12)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
					[ Hint(FTryllEditorUiContract::KnowledgeSelectRecord) ]
					+ SVerticalBox::Slot().AutoHeight()
					[ SectionBox(TEXT("Add / update"), AddBody) ]
					+ SVerticalBox::Slot().AutoHeight()
					[ SectionBox(TEXT("Remove"), RemoveBody) ]
					+ SVerticalBox::Slot().AutoHeight()
					[ SectionBox(TEXT("Store snapshot"), SnapBody) ]
				]
			]
		];
}

void STryllKnowledgePanel::HandleHostColorsRefreshed()
{
	RefreshLists();
	RefreshStatusChrome();
}

void STryllKnowledgePanel::RefreshStatusChrome()
{
	if (!StatusBar.IsValid())
	{
		return;
	}

	ETryllOwnerPhase Owner = EditorSubsystem
		? EditorSubsystem->GetOwnerPhase()
		: ETryllOwnerPhase::Idle;
	FString Detail = EditorSubsystem ? EditorSubsystem->GetOwnerDetail() : FString();
	if (State == EState::Error)
	{
		Owner = ETryllOwnerPhase::Error;
		Detail = Status;
	}

	ETryllPanelOperation LocalOp = ETryllPanelOperation::None;
	if (bRefreshInFlight)
	{
		LocalOp = bListing ? ETryllPanelOperation::Listing : ETryllPanelOperation::Refreshing;
	}
	StatusBar->Bind(FTryllStatusPresentation::Compose(
		Owner, Detail, LocalOp, ETryllEditorPanel::Knowledge));
	RefreshBanner();
}

void STryllKnowledgePanel::RefreshBanner()
{
	if (!Banner.IsValid())
	{
		return;
	}

	TArray<FString> Parts;
	if (!Presentation.StaleBanner.IsEmpty())
	{
		Parts.Add(Presentation.bHasLastGoodView
			? FString(TEXT("Showing the last good view.\n")) + Presentation.StaleBanner
			: Presentation.StaleBanner);
	}
	if (Presentation.bListTruncated)
	{
		Parts.Add(FTryllEditorUiContract::FormatKnowledgeTruncated(
			static_cast<int32>(FTryllKnowledgePresentation::ListLimit)));
	}
	for (const FString& Warning : Presentation.Warnings)
	{
		if (!Warning.IsEmpty())
		{
			Parts.Add(Warning);
		}
	}
	if (Parts.Num() == 0)
	{
		Banner->Clear();
		return;
	}
	Banner->Bind(FString::Join(Parts, TEXT("\n\n")), ETryllStatusSeverity::Warning);
}

void STryllKnowledgePanel::RefreshLists()
{
	RebuildStoreList();
	RebuildRecordList();
	RefreshBanner();
	if (UpsertIdBox.IsValid())
	{
		UpsertIdBox->SetText(FText::FromString(UpsertId));
	}
	if (UpsertTextBox.IsValid())
	{
		UpsertTextBox->SetText(FText::FromString(UpsertText));
	}
}

void STryllKnowledgePanel::RefreshStatsLabel()
{
	if (!StatsLabel.IsValid())
	{
		return;
	}
	if (Presentation.Stats.IsSet())
	{
		const FTryllMutableKbStats& Stats = Presentation.Stats.GetValue();
		const FString Title = Presentation.SelectedName.IsEmpty()
			? FString(TEXT("Records"))
			: FString::Printf(TEXT("Records — %s"), *Presentation.SelectedName);
		StatsLabel->SetText(FText::FromString(FString::Printf(
			TEXT("%s\nlive %u  ·  tombstones %u  ·  gen %llu  ·  %s  ·  ~%llu bytes"),
			*Title,
			Stats.LiveCount,
			Stats.TombstonedCount,
			static_cast<unsigned long long>(Stats.WriteGeneration),
			Stats.bMaterialized ? TEXT("materialized") : TEXT("lazy"),
			static_cast<unsigned long long>(Stats.BytesEstimate))));
	}
	else if (!Presentation.SelectedName.IsEmpty())
	{
		StatsLabel->SetText(FText::FromString(FString::Printf(
			TEXT("Records — %s\nLoading…"), *Presentation.SelectedName)));
	}
	else
	{
		StatsLabel->SetText(FText::FromString(FTryllEditorUiContract::KnowledgeSelectStore));
	}
}

void STryllKnowledgePanel::RebuildStoreList()
{
	if (!StoreList.IsValid())
	{
		return;
	}
	StoreList->ClearChildren();
	if (Presentation.Stores.Num() == 0)
	{
		StoreList->AddSlot().AutoHeight().Padding(0, 24, 0, 0)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoStores", "No stores. Click Refresh or Ensure below."))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
			.Justification(ETextJustify::Center)
			.AutoWrapText(true)
		];
		return;
	}

	for (const FTryllMutableKbInfo& Store : Presentation.Stores)
	{
		const FString Name = Store.Name;
		const bool bSelected = Name == Presentation.SelectedName;
		const FString Caption = FString::Printf(
			TEXT("%s\n%s  ·  %u  ·  gen %llu  ·  %s"),
			*Store.Name,
			*Store.Schema,
			Store.LiveCount,
			static_cast<unsigned long long>(Store.WriteGeneration),
			Store.bMaterialized ? TEXT("materialized") : TEXT("lazy"));
		StoreList->AddSlot().AutoHeight().Padding(0, 0, 0, 4)
		[
			SelectableRow(Caption, bSelected, FOnClicked::CreateLambda([this, Name]
			{
				Presentation.SelectStore(Name);
				RefreshLists();
				LoadSelectedStore(Presentation.BeginRefresh());
				return FReply::Handled();
			}))
		];
	}
}

void STryllKnowledgePanel::RebuildRecordList()
{
	RefreshStatsLabel();
	if (!RecordList.IsValid())
	{
		return;
	}
	RecordList->ClearChildren();

	const TArray<FTryllKnowledgeVisibleRow> Rows = Presentation.VisibleRows();
	if (Rows.Num() == 0)
	{
		FString Empty = FTryllEditorUiContract::KnowledgeSelectStore;
		if (!Presentation.SelectedName.IsEmpty())
		{
			Empty = Presentation.Mode == ETryllKnowledgeBrowseMode::Query
				? FString(TEXT("No query results. Enter text and Run query."))
				: FString(TEXT("No records in this store."));
		}
		RecordList->AddSlot().AutoHeight().Padding(0, 24, 0, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Empty))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
			.Justification(ETextJustify::Center)
			.AutoWrapText(true)
		];
		return;
	}

	for (const FTryllKnowledgeVisibleRow& Row : Rows)
	{
		const FString Id = Row.Id;
		const FString Text = Row.Text;
		const bool bSelected = Id == Presentation.SelectedRecordId;
		const FString Caption = Row.Distance.IsSet()
			? FString::Printf(TEXT("%s  d=%.3f\n%s"), *Id, Row.Distance.GetValue(), *Text)
			: FString::Printf(TEXT("%s\n%s"), *Id, *Text);
		RecordList->AddSlot().AutoHeight().Padding(0, 0, 0, 4)
		[
			SelectableRow(Caption, bSelected, FOnClicked::CreateLambda([this, Id, Text]
			{
				Presentation.SelectedRecordId = Id;
				UpsertId = Id;
				UpsertText = Text;
				RefreshLists();
				return FReply::Handled();
			}))
		];
	}
}

void STryllKnowledgePanel::HandleOwnerPhaseChanged(ETryllOwnerPhase /*Phase*/, const FString& /*Detail*/)
{
	RefreshStatusChrome();
}

bool STryllKnowledgePanel::CanRefresh() const
{
	return IsSessionReady() && !bRefreshInFlight;
}

bool STryllKnowledgePanel::IsSessionReady() const
{
	return State == EState::Ready && EditorSubsystem && EditorSubsystem->IsSessionReady();
}

FTryllPanelSnapshot STryllKnowledgePanel::DebugSnapshotForTests() const
{
	static const TCHAR* StateNames[] = { TEXT("Idle"), TEXT("Connecting"), TEXT("Configuring"), TEXT("Ready"), TEXT("Error") };
	FTryllPanelSnapshot Out;
	Out.OwnerPhase = EditorSubsystem ? EditorSubsystem->GetOwnerPhase() : ETryllOwnerPhase::Idle;
	Out.LocalState = StateNames[static_cast<uint8>(State)];
	if (StatusBar.IsValid())
	{
		Out.StatusCaption = StatusBar->DebugCaption();
		Out.StatusAction = StatusBar->DebugActionText();
	}
	if (Banner.IsValid())
	{
		Out.BannerText = Banner->DebugText();
	}
	for (const FTryllMutableKbInfo& Store : Presentation.Stores)
	{
		Out.Rows.Add(Store.Name);
	}
	Out.bLeaseHeld = bAcquiredConnection;
	Out.ListGeneration = Presentation.RefreshGeneration;
	return Out;
}

FReply STryllKnowledgePanel::OnRefreshClicked()
{
	if (!CanRefresh())
	{
		return FReply::Handled();
	}
	RefreshStores(Presentation.BeginRefresh());
	return FReply::Handled();
}

FReply STryllKnowledgePanel::OnFilterModeClicked()
{
	Presentation.SetMode(ETryllKnowledgeBrowseMode::Filter);
	RefreshLists();
	return FReply::Handled();
}

FReply STryllKnowledgePanel::OnQueryModeClicked()
{
	Presentation.SetMode(ETryllKnowledgeBrowseMode::Query);
	RefreshLists();
	return FReply::Handled();
}

FString STryllKnowledgePanel::SchemaOf(const FString& Name) const
{
	for (const FTryllMutableKbInfo& Store : Presentation.Stores)
	{
		if (Store.Name == Name && !Store.Schema.IsEmpty())
		{
			return Store.Schema;
		}
	}
	return EnsureSchema;
}

void STryllKnowledgePanel::SurfaceError(const FTryllError& Error, const TArray<FString>& Extra)
{
	const FString Text = FTryllKnowledgePresentation::FormatWarning(Error, Extra);
	if (FTryllKnowledgePresentation::ShouldSurfaceBusy(Error) || Presentation.bHasLastGoodView)
	{
		Presentation.MarkStale(Text);
	}
	else
	{
		Presentation.AddWarning(Text);
	}
}

void STryllKnowledgePanel::RequireSelected(TFunction<void(TSharedPtr<FTryllMutableKb>)> Then)
{
	if (!IsSessionReady() || Presentation.SelectedName.IsEmpty() || !Then)
	{
		return;
	}
	if (TSharedPtr<FTryllMutableKb>* Existing = Handles.Find(Presentation.SelectedName))
	{
		if (Existing->IsValid())
		{
			Then(*Existing);
			return;
		}
	}

	TWeakPtr<STryllKnowledgePanel> Weak = SharedThis(this);
	const FString Name = Presentation.SelectedName;
	EditorSubsystem->RequestEnsureMutableKb(
		Name,
		SchemaOf(Name),
		[Weak, Then](TSharedPtr<FTryllMutableKb> Handle, FTryllError Error)
		{
			TSharedPtr<STryllKnowledgePanel> Panel = Weak.Pin();
			if (!Panel.IsValid())
			{
				return;
			}
			if (!Error.IsOk() || !Handle.IsValid())
			{
				Panel->bListing = false;
				Panel->bRefreshInFlight = false;
				Panel->SurfaceError(Error);
				Panel->RefreshStatusChrome();
				Panel->RefreshLists();
				return;
			}
			Panel->Handles.Add(Handle->GetName(), Handle);
			Then(Handle);
		});
}

void STryllKnowledgePanel::LoadSelectedStore(int32 Generation)
{
	if (!IsSessionReady() || Presentation.SelectedName.IsEmpty())
	{
		bListing = false;
		bRefreshInFlight = false;
		RefreshStatusChrome();
		return;
	}

	bRefreshInFlight = true;
	bListing = true;
	RefreshStatusChrome();

	TWeakPtr<STryllKnowledgePanel> Weak = SharedThis(this);
	RequireSelected([Weak, Generation](TSharedPtr<FTryllMutableKb> Handle)
	{
		TSharedPtr<STryllKnowledgePanel> Panel = Weak.Pin();
		if (!Panel.IsValid() || !Panel->Presentation.IsCurrent(Generation) || !Handle.IsValid())
		{
			return;
		}
		Handle->List(
			0,
			FTryllKnowledgePresentation::ListLimit,
			[Weak, Generation, Handle](const FTryllMutableKbListPage& Page, const FTryllError& ListError)
			{
				TSharedPtr<STryllKnowledgePanel> Inner = Weak.Pin();
				if (!Inner.IsValid() || !Inner->Presentation.IsCurrent(Generation))
				{
					return;
				}
				if (!ListError.IsOk())
				{
					Inner->SurfaceError(ListError);
					Inner->bListing = false;
					Inner->bRefreshInFlight = false;
					Inner->RefreshStatusChrome();
					Inner->RefreshLists();
					return;
				}
				Handle->Stats([Weak, Generation, Page](const FTryllMutableKbStats& Stats, const FTryllError& StatsError)
				{
					TSharedPtr<STryllKnowledgePanel> Done = Weak.Pin();
					if (!Done.IsValid() || !Done->Presentation.IsCurrent(Generation))
					{
						return;
					}
					if (!StatsError.IsOk())
					{
						Done->SurfaceError(StatsError);
					}
					else
					{
						Done->Presentation.ApplyList(Page, Stats);
					}
					Done->bListing = false;
					Done->bRefreshInFlight = false;
					Done->RefreshStatusChrome();
					Done->RefreshLists();
				});
			});
	});
}

void STryllKnowledgePanel::RefreshStores(int32 Generation)
{
	if (!IsSessionReady())
	{
		return;
	}

	bRefreshInFlight = true;
	bListing = false;
	Presentation.ClearWarnings();
	RefreshStatusChrome();

	TWeakPtr<STryllKnowledgePanel> Weak = SharedThis(this);
	EditorSubsystem->RequestEnumerateMutableKbs(
		[Weak, Generation](TArray<FTryllMutableKbInfo> Stores, FTryllError Error)
		{
			TSharedPtr<STryllKnowledgePanel> Panel = Weak.Pin();
			if (!Panel.IsValid() || !Panel->Presentation.IsCurrent(Generation))
			{
				return;
			}
			if (!Error.IsOk())
			{
				Panel->bRefreshInFlight = false;
				Panel->SurfaceError(Error);
				Panel->RefreshStatusChrome();
				Panel->RefreshLists();
				return;
			}
			Panel->Presentation.ApplyEnumerate(Stores);
			if (!Panel->Presentation.SelectedName.IsEmpty())
			{
				Panel->LoadSelectedStore(Generation);
				return;
			}
			Panel->bRefreshInFlight = false;
			Panel->RefreshStatusChrome();
			Panel->RefreshLists();
		});
}

FReply STryllKnowledgePanel::OnEnsureClicked()
{
	if (!IsSessionReady())
	{
		return FReply::Handled();
	}
	TWeakPtr<STryllKnowledgePanel> Weak = SharedThis(this);
	EditorSubsystem->RequestEnsureMutableKb(
		EnsureName,
		EnsureSchema,
		[Weak](TSharedPtr<FTryllMutableKb> Handle, FTryllError Error)
		{
			TSharedPtr<STryllKnowledgePanel> Panel = Weak.Pin();
			if (!Panel.IsValid())
			{
				return;
			}
			if (!Error.IsOk() || !Handle.IsValid())
			{
				Panel->SurfaceError(Error);
				Panel->RefreshStatusChrome();
				Panel->RefreshLists();
				return;
			}
			Panel->Handles.Add(Handle->GetName(), Handle);
			Panel->Presentation.SelectStore(Handle->GetName());
			Panel->RefreshStores(Panel->Presentation.BeginRefresh());
		});
	return FReply::Handled();
}

FReply STryllKnowledgePanel::OnSaveGameClicked()
{
	if (!IsSessionReady())
	{
		return FReply::Handled();
	}
	TWeakPtr<STryllKnowledgePanel> Weak = SharedThis(this);
	EditorSubsystem->RequestSaveGameMemory(
		GamePath,
		SaveId,
		[Weak](FTryllGameMemorySaveResult Result, FTryllError Error)
		{
			TSharedPtr<STryllKnowledgePanel> Panel = Weak.Pin();
			if (!Panel.IsValid())
			{
				return;
			}
			if (!Error.IsOk())
			{
				Panel->SurfaceError(Error);
			}
			else
			{
				Panel->Presentation.AddWarning(FString::Printf(
					TEXT("Saved %u stores at %llu (path %s)."),
					Result.StoreCount,
					static_cast<unsigned long long>(Result.SavedAtMs),
					*Panel->GamePath));
			}
			Panel->RefreshStatusChrome();
			Panel->RefreshLists();
		});
	return FReply::Handled();
}

FReply STryllKnowledgePanel::OnLoadGameClicked()
{
	if (!IsSessionReady())
	{
		return FReply::Handled();
	}
	TWeakPtr<STryllKnowledgePanel> Weak = SharedThis(this);
	EditorSubsystem->RequestLoadGameMemory(
		GamePath,
		EmbedTiming,
		Compatibility,
		[Weak](FTryllGameMemoryLoadResult Result, FTryllError Error)
		{
			TSharedPtr<STryllKnowledgePanel> Panel = Weak.Pin();
			if (!Panel.IsValid())
			{
				return;
			}
			if (!Error.IsOk())
			{
				Panel->SurfaceError(Error, Result.Warnings);
				Panel->RefreshStatusChrome();
				Panel->RefreshLists();
				return;
			}
			const FString Formatted = FTryllKnowledgePresentation::FormatWarning(FTryllError(), Result.Warnings);
			if (!Formatted.IsEmpty())
			{
				Panel->Presentation.AddWarning(Formatted);
			}
			Panel->Handles.Reset();
			Panel->RefreshStores(Panel->Presentation.BeginRefresh());
		});
	return FReply::Handled();
}

FReply STryllKnowledgePanel::OnRunQueryClicked()
{
	TWeakPtr<STryllKnowledgePanel> Weak = SharedThis(this);
	RequireSelected([Weak](TSharedPtr<FTryllMutableKb> Handle)
	{
		TSharedPtr<STryllKnowledgePanel> Panel = Weak.Pin();
		if (!Panel.IsValid() || !Handle.IsValid())
		{
			return;
		}
		const FString Store = Handle->GetName();
		const int32 Generation = Panel->Presentation.RefreshGeneration;
		const int32 Epoch = ++Panel->QueryEpoch;
		Panel->Presentation.ClearWarnings();
		Handle->Query(
			Panel->QueryText,
			static_cast<uint32>(FMath::Max(1, Panel->QueryTopK)),
			[Weak, Store, Generation, Epoch](const TArray<FTryllMutableKbHit>& Hits, const FTryllError& Error)
			{
				TSharedPtr<STryllKnowledgePanel> Inner = Weak.Pin();
				if (!Inner.IsValid()
					|| Epoch != Inner->QueryEpoch
					|| !Inner->Presentation.IsCurrent(Generation)
					|| Inner->Presentation.SelectedName != Store)
				{
					return;
				}
				if (!Error.IsOk())
				{
					Inner->SurfaceError(Error);
					Inner->RefreshStatusChrome();
					Inner->RefreshLists();
					return;
				}
				Inner->Presentation.ApplyQueryHits(Hits);
				if (Hits.Num() == 0)
				{
					Inner->Presentation.AddWarning(TEXT("No matching records."));
				}
				Inner->RefreshLists();
			});
	});
	return FReply::Handled();
}

FReply STryllKnowledgePanel::OnWriteClicked()
{
	TWeakPtr<STryllKnowledgePanel> Weak = SharedThis(this);
	RequireSelected([Weak](TSharedPtr<FTryllMutableKb> Handle)
	{
		TSharedPtr<STryllKnowledgePanel> Panel = Weak.Pin();
		if (!Panel.IsValid() || !Handle.IsValid())
		{
			return;
		}
		TArray<FTryllMutableKbUpsert> Records;
		FTryllMutableKbUpsert Row;
		Row.Id = Panel->UpsertId;
		Row.Text = Panel->UpsertText;
		Records.Add(MoveTemp(Row));
		const FString Store = Handle->GetName();
		const int32 Generation = Panel->Presentation.RefreshGeneration;
		Handle->Write(Records, [Weak, Store, Generation](const TArray<FTryllMutableKbWriteResult>& Results, const FTryllError& Error)
		{
			TSharedPtr<STryllKnowledgePanel> Inner = Weak.Pin();
			if (!Inner.IsValid()
				|| !Inner->Presentation.IsCurrent(Generation)
				|| Inner->Presentation.SelectedName != Store)
			{
				return;
			}
			if (!Error.IsOk())
			{
				Inner->SurfaceError(Error);
				Inner->RefreshStatusChrome();
				Inner->RefreshLists();
				return;
			}
			for (const FTryllMutableKbWriteResult& Result : Results)
			{
				if (!Result.bAccepted)
				{
					Inner->Presentation.AddWarning(
						Result.Error.IsEmpty() ? FString(TEXT("Write rejected")) : Result.Error);
				}
				else if (!Result.Id.IsEmpty())
				{
					Inner->UpsertId = Result.Id;
				}
			}
			Inner->RefreshStores(Inner->Presentation.BeginRefresh());
		});
	});
	return FReply::Handled();
}

FReply STryllKnowledgePanel::OnDeleteClicked()
{
	if (Presentation.SelectedRecordId.IsEmpty())
	{
		return FReply::Handled();
	}
	TWeakPtr<STryllKnowledgePanel> Weak = SharedThis(this);
	const FString Id = Presentation.SelectedRecordId;
	RequireSelected([Weak, Id](TSharedPtr<FTryllMutableKb> Handle)
	{
		TSharedPtr<STryllKnowledgePanel> Panel = Weak.Pin();
		if (!Panel.IsValid() || !Handle.IsValid())
		{
			return;
		}
		Handle->Delete({ Id }, [Weak](uint32 /*Deleted*/, const FTryllError& Error)
		{
			TSharedPtr<STryllKnowledgePanel> Inner = Weak.Pin();
			if (!Inner.IsValid())
			{
				return;
			}
			if (!Error.IsOk())
			{
				Inner->SurfaceError(Error);
				Inner->RefreshStatusChrome();
				Inner->RefreshLists();
				return;
			}
			Inner->RefreshStores(Inner->Presentation.BeginRefresh());
		});
	});
	return FReply::Handled();
}

FReply STryllKnowledgePanel::OnClearClicked()
{
	const FText Body = FText::Format(
		LOCTEXT("ClearBody", "Delete every live record in '{0}'?"),
		FText::FromString(Presentation.SelectedName));
	const bool bConfirmed = FTryllEditorPrompts::Open(EAppMsgType::YesNo, Body) == EAppReturnType::Yes;
	if (!FTryllKnowledgePresentation::IsDestructiveClearConfirmed(bConfirmed))
	{
		return FReply::Handled();
	}
	TWeakPtr<STryllKnowledgePanel> Weak = SharedThis(this);
	RequireSelected([Weak](TSharedPtr<FTryllMutableKb> Handle)
	{
		TSharedPtr<STryllKnowledgePanel> Panel = Weak.Pin();
		if (!Panel.IsValid() || !Handle.IsValid())
		{
			return;
		}
		Handle->Clear([Weak](uint32 /*Cleared*/, const FTryllError& Error)
		{
			TSharedPtr<STryllKnowledgePanel> Inner = Weak.Pin();
			if (!Inner.IsValid())
			{
				return;
			}
			if (!Error.IsOk())
			{
				Inner->SurfaceError(Error);
				Inner->RefreshStatusChrome();
				Inner->RefreshLists();
				return;
			}
			Inner->RefreshStores(Inner->Presentation.BeginRefresh());
		});
	});
	return FReply::Handled();
}

FReply STryllKnowledgePanel::OnExportClicked()
{
	TWeakPtr<STryllKnowledgePanel> Weak = SharedThis(this);
	RequireSelected([Weak](TSharedPtr<FTryllMutableKb> Handle)
	{
		TSharedPtr<STryllKnowledgePanel> Panel = Weak.Pin();
		if (!Panel.IsValid() || !Handle.IsValid())
		{
			return;
		}
		Handle->Export([Weak](const FTryllMutableKbSnapshot& Snapshot, const FTryllError& Error)
		{
			TSharedPtr<STryllKnowledgePanel> Inner = Weak.Pin();
			if (!Inner.IsValid())
			{
				return;
			}
			if (!Error.IsOk())
			{
				Inner->SurfaceError(Error);
				Inner->RefreshStatusChrome();
				Inner->RefreshLists();
				return;
			}
			TArray<uint8> Bytes;
			FTryllError EncodeError;
			if (!Snapshot.ToFlatBufferBytes(Bytes, EncodeError))
			{
				Inner->Presentation.AddWarning(EncodeError.Message);
				Inner->RefreshLists();
				return;
			}
			TArray<FString> OutFiles;
			const bool bSaved = FTryllEditorPrompts::SaveFileDialog(
				FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
				TEXT("Export TMKB"),
				FString(),
				FPaths::GetCleanFilename(Inner->StoreExportHint),
				TEXT("TMKB files (*.tmkb)|*.tmkb"),
				EFileDialogFlags::None,
				OutFiles);
			if (!bSaved || OutFiles.Num() == 0)
			{
				return;
			}
			if (!FFileHelper::SaveArrayToFile(Bytes, *OutFiles[0]))
			{
				Inner->Presentation.AddWarning(TEXT("Failed to write TMKB file."));
			}
			else
			{
				Inner->StoreExportHint = FPaths::GetCleanFilename(OutFiles[0]);
			}
			Inner->RefreshLists();
		});
	});
	return FReply::Handled();
}

FReply STryllKnowledgePanel::OnImportClicked()
{
	TArray<FString> OutFiles;
	const bool bOpened = FTryllEditorPrompts::OpenFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Import TMKB"),
		FString(),
		FString(),
		TEXT("TMKB files (*.tmkb)|*.tmkb"),
		EFileDialogFlags::None,
		OutFiles);
	if (!bOpened || OutFiles.Num() == 0)
	{
		return FReply::Handled();
	}

	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *OutFiles[0]))
	{
		Presentation.AddWarning(TEXT("Failed to read TMKB file."));
		RefreshLists();
		return FReply::Handled();
	}
	FTryllMutableKbSnapshot Snapshot;
	FTryllError DecodeError;
	if (!FTryllMutableKbSnapshot::FromFlatBufferBytes(Bytes, Snapshot, DecodeError))
	{
		Presentation.AddWarning(DecodeError.Message);
		RefreshLists();
		return FReply::Handled();
	}

	const ETryllMutableKbCompatibility Compat = Compatibility;
	TWeakPtr<STryllKnowledgePanel> Weak = SharedThis(this);
	RequireSelected([Weak, Snapshot, Compat](TSharedPtr<FTryllMutableKb> Handle)
	{
		TSharedPtr<STryllKnowledgePanel> Panel = Weak.Pin();
		if (!Panel.IsValid() || !Handle.IsValid())
		{
			return;
		}
		Handle->Import(Snapshot, Compat, [Weak](const FTryllMutableKbImportResult& Result, const FTryllError& Error)
		{
			TSharedPtr<STryllKnowledgePanel> Inner = Weak.Pin();
			if (!Inner.IsValid())
			{
				return;
			}
			if (!Error.IsOk())
			{
				Inner->SurfaceError(Error, Result.Warnings);
				Inner->RefreshStatusChrome();
				Inner->RefreshLists();
				return;
			}
			const FString Formatted = FTryllKnowledgePresentation::FormatWarning(FTryllError(), Result.Warnings);
			if (!Formatted.IsEmpty())
			{
				Inner->Presentation.AddWarning(Formatted);
			}
			Inner->RefreshStores(Inner->Presentation.BeginRefresh());
		});
	});
	return FReply::Handled();
}

void STryllKnowledgePanel::SetState(EState S, const FString& InStatus)
{
	State = S;
	Status = InStatus;
	RefreshStatusChrome();
}

void STryllKnowledgePanel::Fail(const FString& Reason)
{
	SetState(EState::Error, Reason);
	if (Presentation.bHasLastGoodView)
	{
		Presentation.MarkStale(Reason);
	}
	else
	{
		Presentation.AddWarning(Reason);
	}
	RefreshLists();
}

void STryllKnowledgePanel::AutofindSettings()
{
	if (Settings.IsValid())
	{
		return;
	}
	Settings = GetMutableDefault<UTryllRuntimeSettings>();
}

void STryllKnowledgePanel::RetryOwner()
{
	const bool bSessionReady = EditorSubsystem && EditorSubsystem->IsSessionReady();
	if (TryllEditorConnectionPolicy::ShouldRetryLocalWork(bAcquiredConnection, bSessionReady))
	{
		OnRefreshClicked();
		return;
	}
	if (bAcquiredConnection)
	{
		Stop();
	}
	Start();
}

void STryllKnowledgePanel::Start()
{
	if (!Settings.IsValid())
	{
		AutofindSettings();
	}
	if (!Settings.IsValid())
	{
		Fail(TEXT("No TryllRuntimeSettings available"));
		return;
	}

	if (!TryllEditorConnectionPolicy::ShouldBindAndAcquire(bAcquiredConnection))
	{
		if (EditorSubsystem && EditorSubsystem->IsConnected())
		{
			HandleConnectionChanged(true);
		}
		return;
	}

	EditorSubsystem = EditorSubsystemOverride
		? EditorSubsystemOverride
		: (GEditor ? GEditor->GetEditorSubsystem<UTryllEditorSubsystem>() : nullptr);
	if (!EditorSubsystem)
	{
		Fail(TEXT("Shared editor subsystem unavailable"));
		return;
	}

	EditorSubsystem->OnConnectionChangedNative.AddSP(this, &STryllKnowledgePanel::HandleConnectionChanged);
	EditorSubsystem->OnCreateSessionCompleteNative.AddSP(this, &STryllKnowledgePanel::HandleCreateSessionComplete);
	EditorSubsystem->OnErrorNative.AddSP(this, &STryllKnowledgePanel::HandleError);
	OwnerPhaseHandle = EditorSubsystem->OnOwnerPhaseChangedNative.AddSP(
		this, &STryllKnowledgePanel::HandleOwnerPhaseChanged);

	SetState(EState::Connecting, FTryllEditorUiContract::StatusConnecting);
	bAcquiredConnection = true;
	EditorSubsystem->AcquireConnection();

	if (EditorSubsystem->IsConnected())
	{
		HandleConnectionChanged(true);
	}
}

void STryllKnowledgePanel::Stop()
{
	Presentation.BeginRefresh();
	bRefreshInFlight = false;
	bListing = false;
	Handles.Reset();

	if (EditorSubsystem)
	{
		EditorSubsystem->OnConnectionChangedNative.RemoveAll(this);
		EditorSubsystem->OnCreateSessionCompleteNative.RemoveAll(this);
		EditorSubsystem->OnErrorNative.RemoveAll(this);
		EditorSubsystem->OnOwnerPhaseChangedNative.Remove(OwnerPhaseHandle);
		OwnerPhaseHandle.Reset();
		if (bAcquiredConnection)
		{
			EditorSubsystem->ReleaseConnection();
		}
	}
	bAcquiredConnection = false;
	EditorSubsystem = nullptr;
	SetState(EState::Idle, TEXT("Idle"));
}

void STryllKnowledgePanel::HandleConnectionChanged(bool bConnected)
{
	if (!bConnected)
	{
		const bool bRetriesRemaining =
			EditorSubsystem && EditorSubsystem->HasConnectRetriesRemaining();
		if (TryllEditorConnectionPolicy::ShouldWaitForOwnerRetry(
				State == EState::Connecting,
				bRetriesRemaining))
		{
			SetState(EState::Connecting, TEXT("Waiting for server…"));
			return;
		}
		Fail(LastErrorMessage.IsEmpty() ? FString(TEXT("Disconnected")) : LastErrorMessage);
		return;
	}
	if (!EditorSubsystem)
	{
		return;
	}

	SetState(EState::Configuring, TEXT("Configuring session…"));
	EditorSubsystem->CreateSession(FString());
}

void STryllKnowledgePanel::HandleCreateSessionComplete(bool bSuccess)
{
	if (!bSuccess)
	{
		Fail(FString::Printf(TEXT("CreateSession failed: %s"), *LastErrorMessage));
		return;
	}
	if (State == EState::Ready)
	{
		return;
	}
	SetState(EState::Ready, TEXT("Ready"));
}

void STryllKnowledgePanel::HandleError(const FString& ErrorMessage)
{
	LastErrorMessage = ErrorMessage;
}

#undef LOCTEXT_NAMESPACE
