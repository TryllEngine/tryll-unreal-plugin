// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "STryllAgentLogPanel.h"
#include "UI/TryllPanelSnapshot.h"
#include "TryllEditorPrompts.h"
#include "TryllStatusPresentation.h"
#include "UI/STryllBanner.h"
#include "UI/STryllServerModeBadge.h"
#include "UI/STryllStatusBar.h"

#include "AgentLog/AgentLogConfig.h"
#include "AgentLog/AgentLogStore.h"
#include "AgentLog/AgentLogTurnCollapse.h"
#include "TryllAgentLogInspection.h"
#include "STryllAgentLogInspectorHost.h"
#include "TryllClientEditorModule.h"
#include "TryllEditorSubsystem.h"
#include "TryllInspectorSplitMath.h"
#include "UI/STryllInspectorSplit.h"
#include "UI/TryllEditorStyle.h"
#include "UI/TryllTailFollow.h"
#include "TryllObservationTypes.h"
#include "AgentLog/TryllAgentLogWriteStore.h"
#include "TryllAgentLogDocument.h"
#include "Welcome/TryllCommunityWidgets.h"

#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "Misc/EngineVersion.h"

#include "Editor.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/SWidget.h"

#define LOCTEXT_NAMESPACE "STryllAgentLogPanel"

namespace AgentLogPanelPrivate
{

FLinearColor RowBackground(const EAgentLogRowKind Kind, const bool bSelected)
{
	if (bSelected)
	{
		return FTryllEditorStyle::SelBub;
	}

	switch (Kind)
	{
	case EAgentLogRowKind::Sent:
		return FTryllEditorStyle::UserBub;
	case EAgentLogRowKind::Received:
		return FTryllEditorStyle::AsstBub;
	case EAgentLogRowKind::CaptureGap:
		return FTryllEditorStyle::WarnBg;
	default:
		return FTryllEditorStyle::Panel2;
	}
}

FString DirectionMarker(const EAgentLogRowKind Kind)
{
	switch (Kind)
	{
	case EAgentLogRowKind::Sent:      return TEXT("→");
	case EAgentLogRowKind::Received:  return TEXT("←");
	case EAgentLogRowKind::CaptureGap: return TEXT("·");
	default:                          return TEXT("•");
	}
}

FLinearColor MarkerColor(const EAgentLogRowKind Kind)
{
	switch (Kind)
	{
	case EAgentLogRowKind::Sent:      return FTryllEditorStyle::SummaryUser;
	case EAgentLogRowKind::Received:  return FTryllEditorStyle::ColOk;
	case EAgentLogRowKind::CaptureGap: return FTryllEditorStyle::ColWarn;
	default:                          return FTryllEditorStyle::Dim;
	}
}

TSharedRef<SWidget> BuildMarkerWidget(const EAgentLogRowKind Kind, const bool bVoiceSentFinal)
{
	const FSlateColor Tint = FSlateColor(MarkerColor(Kind));
	if (bVoiceSentFinal)
	{
		return SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.WidthOverride(12.0f)
			.HeightOverride(12.0f)
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("ClassIcon.AudioComponent"))
				.ColorAndOpacity(Tint)
			];
	}

	return SNew(STextBlock)
		.Text(FText::FromString(DirectionMarker(Kind)))
		.ColorAndOpacity(Tint)
		.Justification(ETextJustify::Center);
}

} // namespace AgentLogPanelPrivate

STryllAgentLogPanel::STryllAgentLogPanel() = default;

void STryllAgentLogPanel::Construct(const FArguments& InArgs)
{
	Store = &FTryllClientEditorModule::GetAgentLogStore();
	bPersistLayout = InArgs._PersistLayout;
	LoadLayoutConfig();

	if (GEditor || InArgs._EditorSubsystemOverride)
	{
		EditorSubsystem = InArgs._EditorSubsystemOverride
			? InArgs._EditorSubsystemOverride
			: GEditor->GetEditorSubsystem<UTryllEditorSubsystem>();
		if (EditorSubsystem)
		{
			// Mirror Test Chat join-late: connection may already be up, or may appear
			// later when Chat / PIE acquires it. One-shot Attach at construct misses both
			// the late create and reconnect-to-new-hub cases.
			ConnectionChangedHandle = EditorSubsystem->OnConnectionChangedNative.AddSP(
				this, &STryllAgentLogPanel::HandleConnectionChanged);
			OwnerPhaseHandle = EditorSubsystem->OnOwnerPhaseChangedNative.AddSP(
				this, &STryllAgentLogPanel::HandleOwnerPhaseChanged);
			AttachIfNeeded();
		}
	}

	JournalChangedHandle = Store->OnJournalChanged.AddSP(this, &STryllAgentLogPanel::OnStoreChanged);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 8, 8, 4)
		[
			BuildToolbar()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 4)
		[
			BuildBanners()
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SSeparator)
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(8, 4, 8, 8)
		[
			BuildMainArea()
		]
	];

	TimelineTail = FTryllTailFollow(
		[this] { return MeasureTimelineDistance(); },
		[this] { ScrollTimelineToEnd(); });

	RebuildAgentListItems();
	RebuildTimelineItems();
	RefreshInspector();
	RefreshStatusChrome();
	HostColorsHandle = FTryllEditorStyle::OnHostColorsRefreshed.AddRaw(
		this, &STryllAgentLogPanel::HandleHostColorsRefreshed);
}

void STryllAgentLogPanel::HandleHostColorsRefreshed()
{
	RebuildAgentListItems();
	RebuildTimelineItems();
	RefreshInspector();
	RefreshStatusChrome();
	RefreshBanners();
}

STryllAgentLogPanel::~STryllAgentLogPanel()
{
	if (HostColorsHandle.IsValid())
	{
		FTryllEditorStyle::OnHostColorsRefreshed.Remove(HostColorsHandle);
		HostColorsHandle.Reset();
	}
	SaveLayoutConfig();

	if (EditorSubsystem != nullptr && ConnectionChangedHandle.IsValid())
	{
		EditorSubsystem->OnConnectionChangedNative.Remove(ConnectionChangedHandle);
		ConnectionChangedHandle.Reset();
	}
	if (EditorSubsystem != nullptr && OwnerPhaseHandle.IsValid())
	{
		EditorSubsystem->OnOwnerPhaseChangedNative.Remove(OwnerPhaseHandle);
		OwnerPhaseHandle.Reset();
	}

	if (Store != nullptr)
	{
		if (JournalChangedHandle.IsValid())
		{
			Store->OnJournalChanged.Remove(JournalChangedHandle);
			JournalChangedHandle.Reset();
		}
		Store->Detach();
	}
}

TSharedRef<SWidget> STryllAgentLogPanel::BuildSegmented(
	const TArray<FText>& Labels,
	TFunction<int32()> GetActive,
	TFunction<void(int32)> SetActive)
{
	return FTryllEditorStyle::Segmented(Labels, MoveTemp(GetActive), MoveTemp(SetActive));
}

TSharedRef<SWidget> STryllAgentLogPanel::BuildToolbar()
{
	// Mirror Dialog Lab: action buttons → segmented controls | spacer | inspector.
	return SNew(SBorder)
		.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::Panel2))
		.Padding(FMargin(10.0f, 5.0f))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SButton)
					.Text(LOCTEXT("Clear", "Clear"))
					.OnClicked(this, &STryllAgentLogPanel::OnClearClicked)
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)
			[
				SNew(SButton)
					.Text(LOCTEXT("Export", "Export"))
					.IsEnabled_Lambda([this] { return Store != nullptr && Store->GetEnvelopeCount() > 0; })
					.OnClicked(this, &STryllAgentLogPanel::OnExportClicked)
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)
			[
				SNew(SButton)
					.Text_Lambda([this]
					{
						return bUserPaused
							? LOCTEXT("Resume", "Resume")
							: LOCTEXT("Pause", "Pause");
					})
					.OnClicked(this, &STryllAgentLogPanel::OnPauseResumeClicked)
			]

			+ SHorizontalBox::Slot().AutoWidth()
			[
				BuildSegmented(
					{LOCTEXT("ModeCurated", "Curated"), LOCTEXT("ModeRaw", "Raw")},
					[this] { return bRawMode ? 1 : 0; },
					[this](int32 Index)
					{
						const bool bNewRaw = Index == 1;
						if (bRawMode == bNewRaw)
						{
							return;
						}
						bRawMode = bNewRaw;
						RebuildTimelineItems();
						RefreshInspector();
					})
			]

			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox)
					.ToolTipText(LOCTEXT(
						"CoalesceTooltip",
						"Merge consecutive AnswerText tokens on the same turn into one row. "
						"TTS and other events still interrupt the stream."))
				[
					BuildSegmented(
						{LOCTEXT("ModeStream", "Stream"), LOCTEXT("ModeCoalesce", "Coalesce")},
						[this] { return bCoalesceAnswerText ? 1 : 0; },
						[this](int32 Index)
						{
							const bool bNew = Index == 1;
							if (bCoalesceAnswerText == bNew)
							{
								return;
							}
							bCoalesceAnswerText = bNew;
							if (Store != nullptr)
							{
								FAgentLogConfig Config = Store->GetConfig();
								Config.bCoalesceAnswerText = bCoalesceAnswerText;
								Store->SetConfig(Config);
								Config.Save();
							}
							RebuildTimelineItems();
						})
				]
			]

			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox)
					.ToolTipText(LOCTEXT(
						"CollapseCompletedTooltip",
						"Collapse completed turns you have not expanded. Manually expanded turns stay open."))
				[
					BuildSegmented(
						{LOCTEXT("ModeExpand", "Expand"),
						 LOCTEXT("ModeCollapseCompleted", "Collapse Completed")},
						[this] { return bCollapseCompletedTurns ? 1 : 0; },
						[this](int32 Index)
						{
							const bool bNew = Index == 1;
							if (bCollapseCompletedTurns == bNew)
							{
								return;
							}
							bCollapseCompletedTurns = bNew;
							if (Store != nullptr)
							{
								FAgentLogConfig Config = Store->GetConfig();
								Config.bCollapseCompletedTurns = bCollapseCompletedTurns;
								Store->SetConfig(Config);
								Config.Save();
							}
							RebuildTimelineItems();
						})
				]
			]

			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0, 0, 0)
			[
				SNew(STryllServerModeBadge)
			]

			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8, 0)
			[
				SAssignNew(StatusBar, STryllStatusBar)
			]

			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
					.Text_Lambda([this]
					{
						return bInspectorVisible
							? LOCTEXT("InspectorHide", "Inspector ▶")
							: LOCTEXT("InspectorShow", "Inspector ◀");
					})
					.OnClicked(this, &STryllAgentLogPanel::OnInspectorToggleClicked)
			]
		];
}

TSharedRef<SWidget> STryllAgentLogPanel::BuildBanners()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
		[
			SAssignNew(WaitingBanner, STryllBanner)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
		[
			SAssignNew(PauseBanner, STryllBanner)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
		[
			SAssignNew(DiscardedBanner, STryllBanner)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
		[
			SAssignNew(PreExistingBanner, STryllBanner)
		];
}

float STryllAgentLogPanel::GetInnerColumnMinWidth() const
{
	float Min = TimelineMinWidth;
	if (bInspectorVisible)
	{
		Min += FTryllEditorStyle::InspectorMinWidth + FTryllInspectorSplitMath::DefaultHandleWidth;
	}
	return Min;
}

TSharedRef<SWidget> STryllAgentLogPanel::BuildMainArea()
{
	// Nested layout: [agent list | (timeline | inspector)]. Outer is fixed-first;
	// inner is fixed-second. Do not make timeline+inspector two SizeToContent siblings.
	return SAssignNew(OuterSplitter, STryllInspectorSplit)
		.FixedSide(STryllInspectorSplit::EFixedSide::First)
		.PrimaryMin(AgentListMinWidth)
		.SecondaryMin_Lambda([this] { return GetInnerColumnMinWidth(); })
		.StoredWidth_Lambda([this] { return AgentListWidth; })
		.OnStoredWidthChanged_Lambda([this](float NewWidth)
		{
			AgentListWidth = NewWidth;
			SaveLayoutConfig();
		})
		.IsFixedPaneVisible(true)
		.PrimaryContent()
		[
			BuildAgentList()
		]
		.SecondaryContent()
		[
			SAssignNew(InnerSplitter, STryllInspectorSplit)
			.FixedSide(STryllInspectorSplit::EFixedSide::Second)
			.PrimaryMin(TimelineMinWidth)
			.SecondaryMin(FTryllEditorStyle::InspectorMinWidth)
			.StoredWidth_Lambda([this] { return InspectorWidth; })
			.OnStoredWidthChanged_Lambda([this](float NewWidth)
			{
				InspectorWidth = NewWidth;
				SaveLayoutConfig();
			})
			.IsFixedPaneVisible_Lambda([this] { return bInspectorVisible; })
			.PrimaryContent()
			[
				BuildTimeline()
			]
			.SecondaryContent()
			[
				BuildInspectorPane()
			]
		];
}

TSharedRef<SWidget> STryllAgentLogPanel::BuildAgentList()
{
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(2)
		[
			SAssignNew(AgentListView, SListView<TSharedPtr<FAgentLogAgentListItem>>)
			.ListItemsSource(&AgentListItems)
			.SelectionMode(ESelectionMode::Single)
			.OnGenerateRow(this, &STryllAgentLogPanel::GenerateAgentRow)
			.OnSelectionChanged(this, &STryllAgentLogPanel::OnAgentSelectionChanged)
		];
}

TSharedRef<SWidget> STryllAgentLogPanel::BuildTimeline()
{
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(0)
		[
			SAssignNew(TimelineListView, SListView<TSharedPtr<FAgentLogTimelineItem>>)
			.ListItemsSource(&TimelineItems)
			.SelectionMode(ESelectionMode::Single)
			.ItemHeight(RowHeightDouble)
			.OnGenerateRow(this, &STryllAgentLogPanel::GenerateTimelineRow)
			.OnSelectionChanged(this, &STryllAgentLogPanel::OnTimelineSelectionChanged)
			.OnListViewScrolled_Lambda([this](double)
			{
				TimelineTail.NotifyUserScroll();
			})
		];
}

TSharedRef<SWidget> STryllAgentLogPanel::BuildInspectorPane()
{
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(0)
		[
			SAssignNew(InspectorHost, STryllAgentLogInspectorHost)
		];
}

TSharedRef<ITableRow> STryllAgentLogPanel::GenerateAgentRow(
	TSharedPtr<FAgentLogAgentListItem> Item,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	const FString Title  = Item.IsValid() ? Item->Title : FString();
	const FString Status = Item.IsValid() ? Item->Status : FString();

	// Selection styling must be live: rows are virtualized and not rebuilt on every
	// click. Baking bSelected at GenerateRow left the blue marker stuck on the old agent.
	auto IsSelected = [this, Item]()
	{
		return SelectedAgentItem.IsValid() && Item.IsValid()
			&& SelectedAgentItem->AgentId == Item->AgentId
			&& SelectedAgentItem->bAllAgents == Item->bAllAgents;
	};

	return SNew(STableRow<TSharedPtr<FAgentLogAgentListItem>>, OwnerTable)
		[
			SNew(SBorder)
			.BorderImage(FTryllEditorStyle::Rounded(0.0f))
			.BorderBackgroundColor_Lambda([IsSelected]
			{
				return FSlateColor(IsSelected() ? FTryllEditorStyle::SelBub : FTryllEditorStyle::Panel);
			})
			.Padding(FMargin(8.0f, 4.0f, 6.0f, 4.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Title))
					.ColorAndOpacity_Lambda([IsSelected]
					{
						return FSlateColor(
							IsSelected() ? FTryllEditorStyle::SegmentOnText : FTryllEditorStyle::Txt);
					})
					.Font_Lambda([IsSelected]
					{
						return FCoreStyle::GetDefaultFontStyle(
							IsSelected() ? "Bold" : "Regular", 9);
					})
					.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Status))
					.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
				]
			]
		];
}

TSharedRef<ITableRow> STryllAgentLogPanel::GenerateTimelineRow(
	TSharedPtr<FAgentLogTimelineItem> Item,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	using namespace AgentLogPanelPrivate;

	const EAgentLogRowKind Kind = Item.IsValid() ? Item->Row.Kind : EAgentLogRowKind::System;
	const bool bFirstOfTurn = Item.IsValid() && Item->Row.bIsFirstRowOfTurn;
	const bool bShowAgentGutter = Item.IsValid() && Item->Row.bShowAgentGutter;
	const bool bReceived = Kind == EAgentLogRowKind::Received;
	const float RowHeight = Item.IsValid() && Item->bDoubleHeight ? RowHeightDouble : RowHeightSingle;
	const uint64 RowEventId = Item.IsValid() ? Item->EventId : 0;

	FString Timestamp;
	FString AgentLabel;
	FString Summary;
	FString Chevron;
	const bool bVoiceSentFinal = Item.IsValid() && Item->Row.bVoiceSentFinal;
	const bool bCanCollapse = Item.IsValid() && Item->Row.bCanCollapseTurn;
	const bool bCollapsed   = Item.IsValid() && Item->Row.bIsTurnCollapsed;
	const FTryllTurnKey TurnKey = Item.IsValid() ? Item->Row.TurnKey : FTryllTurnKey{};
	if (Item.IsValid() && Store != nullptr)
	{
		if (const FTryllObservationEnvelope* Envelope = EnvelopeForRow(Item->Row))
		{
			Timestamp = FTryllAgentLogInspection::FormatTimestamp(Envelope->WallTimeUtc);
		}
		if (bShowAgentGutter)
		{
			AgentLabel = FTryllAgentLogInspection::FormatAgentGutterLabel(
				Item->Row.AgentId, Store->GetDescriptors());
		}
		Summary = Item->Summary;
		if (bCanCollapse)
		{
			Chevron = bCollapsed ? TEXT("▶") : TEXT("▼");
		}
	}

	return SNew(STableRow<TSharedPtr<FAgentLogTimelineItem>>, OwnerTable)
		[
			SNew(SBox)
			.HeightOverride(RowHeight)
			[
				SNew(SBorder)
				.BorderImage(FTryllEditorStyle::Rounded(0.0f))
				.BorderBackgroundColor_Lambda([this, Kind, RowEventId]
				{
					const bool bSelected = SelectedTimelineItem.IsValid()
						&& SelectedTimelineItem->EventId == RowEventId
						&& RowEventId != 0;
					return FSlateColor(RowBackground(Kind, bSelected));
				})
				.Padding(FMargin(
					0.0f,
					bFirstOfTurn ? 2.0f : 0.0f,
					4.0f,
					0.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SBorder)
						.BorderImage(FTryllEditorStyle::Rounded(0.0f))
						.BorderBackgroundColor(FSlateColor(
							bFirstOfTurn ? FTryllEditorStyle::Line2 : FLinearColor(0, 0, 0, 0)))
						.Padding(0)
						[
							SNew(SBox).HeightOverride(bFirstOfTurn ? 1.0f : 0.0f)
						]
					]
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						SNew(SHorizontalBox)
						// Fixed chevron rail on every row (empty when not collapsible).
						+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0, 0, 0)
						.VAlign(VAlign_Center)
						[
							SNew(SBox)
							.WidthOverride(ChevronColumnWidth)
							[
								SNew(SButton)
								.ButtonStyle(FCoreStyle::Get(), "NoBorder")
								.ContentPadding(FMargin(0))
								.IsEnabled(bCanCollapse)
								.OnClicked_Lambda([this, TurnKey, bCollapsed]()
								{
									return OnTurnChevronClicked(TurnKey, bCollapsed);
								})
								[
									SNew(STextBlock)
									.Text(FText::FromString(Chevron))
									.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
									.Justification(ETextJustify::Center)
								]
							]
						]
						// Fixed marker + timestamp rails — never AutoWidth on glyphs.
						+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0, 0, 0)
						.VAlign(VAlign_Center)
						[
							SNew(SBox)
							.WidthOverride(MarkerColumnWidth)
							[
								BuildMarkerWidget(Kind, bVoiceSentFinal)
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 4, 0)
						.VAlign(VAlign_Center)
						[
							SNew(SBox)
							.WidthOverride(TimestampColumnWidth)
							[
								SNew(STextBlock)
								.Text(FText::FromString(Timestamp))
								.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Faint))
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 4, 0)
						.VAlign(VAlign_Center)
						[
							SNew(SBox)
							.WidthOverride(AgentGutterWidth)
							.Visibility(bShowAgentGutter ? EVisibility::Visible : EVisibility::Collapsed)
							[
								SNew(STextBlock)
								.Text(FText::FromString(AgentLabel))
								.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
								.Clipping(EWidgetClipping::ClipToBounds)
							]
						]
						// R12.3a: indent summary only so timestamps stay on one rail.
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
						.Padding(bReceived ? ReceivedIndent : 0.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(Summary))
							.ColorAndOpacity(FSlateColor(
								Kind == EAgentLogRowKind::System || Kind == EAgentLogRowKind::CaptureGap
									? FTryllEditorStyle::Dim
									: FTryllEditorStyle::Txt))
							.Clipping(EWidgetClipping::ClipToBounds)
						]
					]
				]
			]
		];
}

void STryllAgentLogPanel::RebuildAgentListItems()
{
	AgentListItems.Reset();

	TArray<uint64> AgentIds;
	int32 TotalCompletedTurns = 0;
	if (Store != nullptr)
	{
		// Refresh labels from live agents (DisplayName arrives after CreateAgentOk).
		Store->RefreshAgentLabels();
		AgentIds = Store->GetDescriptors().ListAgentIds();
		TotalCompletedTurns = Store->GetTotalCompletedTurnCount();
	}

	TSharedPtr<FAgentLogAgentListItem> AllAgents = MakeShared<FAgentLogAgentListItem>();
	AllAgents->bAllAgents = true;
	AllAgents->Title      = TEXT("All Agents");
	AllAgents->Status     = FTryllAgentLogInspection::FormatAllAgentsStatus(
		AgentIds.Num(), TotalCompletedTurns);
	AgentListItems.Add(AllAgents);

	if (Store != nullptr)
	{
		const FAgentDescriptorRegistry& Descriptors = Store->GetDescriptors();
		for (const uint64 AgentId : AgentIds)
		{
			TSharedPtr<FAgentLogAgentListItem> Entry = MakeShared<FAgentLogAgentListItem>();
			Entry->AgentId = AgentId;
			Entry->Title   = FTryllAgentLogInspection::FormatAgentGutterLabel(AgentId, Descriptors);

			bool bDestroyed = false;
			bool bLimitedDiagnostics = false;
			bool bPending = Entry->Title.StartsWith(TEXT("Pending agent (req "));
			if (const FAgentDescriptor* Descriptor = Descriptors.Find(AgentId))
			{
				bDestroyed = Descriptor->bDestroyed;
				bLimitedDiagnostics = Descriptor->bCreatedBeforeDiagnosticsCapture;
			}

			Entry->Status = FTryllAgentLogInspection::FormatAgentListStatus(
				Store->GetCompletedTurnCount(AgentId),
				bDestroyed,
				bLimitedDiagnostics,
				bPending);
			AgentListItems.Add(Entry);
		}
	}

	if (AgentListView.IsValid())
	{
		AgentListView->RequestListRefresh();

		if (!SelectedAgentItem.IsValid())
		{
			SelectedAgentItem = AllAgents;
			bAllAgentsSelected = true;
			SelectedAgentId = 0;
			AgentListView->SetSelection(AllAgents);
		}
	}
}

void STryllAgentLogPanel::RebuildTimelineItems()
{
	if (Store == nullptr)
	{
		TimelineItems.Reset();
		if (TimelineListView.IsValid())
		{
			TimelineListView->RequestListRefresh();
		}
		return;
	}

	FullProjectedRows = FAgentLogProjection::Project(*Store, CurrentFilter());
	const TArray<FAgentLogRow> Projected =
		FAgentLogTurnCollapse::Apply(FullProjectedRows, TurnCollapse, bCollapseCompletedTurns);

	TimelineItems.Reset();
	TimelineItems.Reserve(Projected.Num());

	for (const FAgentLogRow& Row : Projected)
	{
		const FTryllObservationEnvelope* Envelope = EnvelopeForRow(Row);
		if (Envelope == nullptr)
		{
			continue;
		}

		TSharedPtr<FAgentLogTimelineItem> Item = MakeShared<FAgentLogTimelineItem>();
		Item->Row       = Row;
		Item->EventId   = Row.EventId != 0 ? Row.EventId : Envelope->EventId;
		Item->Summary = !Row.OverrideSummary.IsEmpty()
			? Row.OverrideSummary
			: FTryllAgentLogInspection::RowSummary(Row, *Envelope, Store->GetDescriptors());
		if (Row.bIsTurnCollapsed && Row.HiddenBodyCount > 0
			&& !Item->Summary.Contains(TEXT(" events")))
		{
			Item->Summary += FString::Printf(TEXT("  ·  %d events"), Row.HiddenBodyCount + 1);
		}
		Item->bDoubleHeight = FTryllAgentLogInspection::RowUsesDoubleHeight(Item->Summary);
		TimelineItems.Add(Item);
	}

	if (TimelineListView.IsValid())
	{
		TimelineListView->RequestListRefresh();
	}

	RestoreTimelineSelection();
	TimelineTail.NotifyAppend();
}

float STryllAgentLogPanel::MeasureTimelineDistance() const
{
	if (!TimelineListView.IsValid())
	{
		return 0.0f;
	}
	return FMath::Max(0.0f, TimelineListView->GetScrollDistanceRemaining().Y);
}

void STryllAgentLogPanel::ScrollTimelineToEnd()
{
	if (!TimelineListView.IsValid() || TimelineItems.Num() == 0)
	{
		return;
	}
	TimelineTail.BeginProgrammaticScroll();
	TimelineListView->RequestScrollIntoView(TimelineItems.Last());
	RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateSP(
		this, &STryllAgentLogPanel::EndTimelineProgrammaticScroll));
}

EActiveTimerReturnType STryllAgentLogPanel::EndTimelineProgrammaticScroll(
	double /*CurrentTime*/, float /*DeltaTime*/)
{
	TimelineTail.EndProgrammaticScroll();
	return EActiveTimerReturnType::Stop;
}

void STryllAgentLogPanel::RestoreTimelineSelection()
{
	if (!TimelineListView.IsValid())
	{
		return;
	}

	auto FindVisibleItem = [this](const uint64 EventId) -> TSharedPtr<FAgentLogTimelineItem>
	{
		for (const TSharedPtr<FAgentLogTimelineItem>& Item : TimelineItems)
		{
			if (Item.IsValid() && Item->EventId == EventId)
			{
				return Item;
			}
		}

		// Selection may sit on a collapsed body — promote to that turn's header.
		FTryllTurnKey Key;
		for (const FAgentLogRow& Row : FullProjectedRows)
		{
			if (Row.EventId == EventId)
			{
				Key = Row.TurnKey;
				break;
			}
		}
		if (!Key.bHasValue)
		{
			return nullptr;
		}
		for (const TSharedPtr<FAgentLogTimelineItem>& Item : TimelineItems)
		{
			if (Item.IsValid() && Item->Row.bCanCollapseTurn && Item->Row.TurnKey == Key)
			{
				return Item;
			}
		}
		return nullptr;
	};

	if (PendingSelectionEventId != 0)
	{
		if (TSharedPtr<FAgentLogTimelineItem> Item = FindVisibleItem(PendingSelectionEventId))
		{
			SelectedTimelineItem    = Item;
			PendingSelectionEventId = Item->EventId;
			TimelineListView->SetSelection(Item);
			return;
		}
	}

	if (SelectedTimelineItem.IsValid())
	{
		if (TSharedPtr<FAgentLogTimelineItem> Item = FindVisibleItem(SelectedTimelineItem->EventId))
		{
			SelectedTimelineItem = Item;
			TimelineListView->SetSelection(Item);
			return;
		}
	}

	SelectedTimelineItem.Reset();
	TimelineListView->ClearSelection();
}

void STryllAgentLogPanel::RefreshInspector()
{
	if (!InspectorHost.IsValid() || Store == nullptr)
	{
		return;
	}

	if (!SelectedTimelineItem.IsValid())
	{
		InspectorHost->RenderNone();
		return;
	}

	InspectorHost->RenderRow(
		SelectedTimelineItem->Row,
		*Store,
		Store->GetDescriptors());
}

void STryllAgentLogPanel::OnStoreChanged()
{
	RebuildAgentListItems();
	RebuildTimelineItems();
	RefreshInspector();
	RefreshStatusChrome();
}

void STryllAgentLogPanel::AttachIfNeeded()
{
	if (bUserPaused || Store == nullptr || EditorSubsystem == nullptr)
	{
		return;
	}

	const TSharedPtr<FTryllConnection> Conn = EditorSubsystem->GetSharedConnectionHandle();
	if (Store->IsSubscribedTo(Conn))
	{
		return;
	}

	Store->Attach(EditorSubsystem);
}

void STryllAgentLogPanel::HandleConnectionChanged(const bool /*bConnected*/)
{
	AttachIfNeeded();
	RefreshStatusChrome();
}

void STryllAgentLogPanel::HandleOwnerPhaseChanged(ETryllOwnerPhase /*Phase*/, const FString& /*Detail*/)
{
	RefreshStatusChrome();
}

void STryllAgentLogPanel::RefreshStatusChrome()
{
	if (!StatusBar.IsValid())
	{
		return;
	}

	ETryllOwnerPhase Owner = EditorSubsystem
		? EditorSubsystem->GetOwnerPhase()
		: ETryllOwnerPhase::Idle;
	ETryllPanelOperation LocalOp = ETryllPanelOperation::None;
	if (Owner == ETryllOwnerPhase::Ready)
	{
		if (Store != nullptr && Store->IsSubscribed() && !bUserPaused)
		{
			LocalOp = ETryllPanelOperation::Capturing;
		}
		else
		{
			LocalOp = ETryllPanelOperation::Waiting;
		}
	}

	StatusBar->Bind(FTryllStatusPresentation::Compose(
		Owner,
		EditorSubsystem ? EditorSubsystem->GetOwnerDetail() : FString(),
		LocalOp,
		ETryllEditorPanel::AgentLog));
	RefreshBanners();
}

void STryllAgentLogPanel::RefreshBanners()
{
	if (WaitingBanner.IsValid())
	{
		if (!bUserPaused
			&& Store != nullptr
			&& Store->GetEnvelopeCount() == 0
			&& !Store->IsHistoryDiscarded())
		{
			if (Store->IsSubscribed())
			{
				WaitingBanner->Bind(
					FTryllEditorUiContract::AgentLogEmpty, ETryllStatusSeverity::Neutral);
			}
			else
			{
				WaitingBanner->Bind(
					TEXT("Waiting for the editor connection — start Test Chat or Play to begin capture."),
					ETryllStatusSeverity::Neutral);
			}
		}
		else
		{
			WaitingBanner->Clear();
		}
	}

	if (PauseBanner.IsValid())
	{
		if (bUserPaused)
		{
			PauseBanner->Bind(
				TEXT("Capture paused — new events are not recorded until you resume."),
				ETryllStatusSeverity::Warning);
		}
		else
		{
			PauseBanner->Clear();
		}
	}

	if (DiscardedBanner.IsValid())
	{
		if (Store != nullptr && Store->IsHistoryDiscarded())
		{
			DiscardedBanner->Bind(
				TEXT("History was discarded by an editor reload. New observations will appear from this point."),
				ETryllStatusSeverity::Warning);
		}
		else
		{
			DiscardedBanner->Clear();
		}
	}

	if (PreExistingBanner.IsValid())
	{
		bool bShow = false;
		if (!bAllAgentsSelected && Store != nullptr && SelectedAgentId != 0)
		{
			if (const FAgentDescriptor* Desc = Store->GetDescriptors().Find(SelectedAgentId))
			{
				bShow = Desc->bCreatedBeforeDiagnosticsCapture;
			}
		}
		if (bShow)
		{
			PreExistingBanner->Bind(
				TEXT("This agent existed before diagnostics capture started. Received turns show "
				     "client observations with a pre-existing-agent absence reason."),
				ETryllStatusSeverity::Neutral);
		}
		else
		{
			PreExistingBanner->Clear();
		}
	}
}

FReply STryllAgentLogPanel::OnExportClicked()
{
	if (Store == nullptr || Store->GetEnvelopeCount() == 0)
	{
		return FReply::Handled();
	}
	const FString Name = FTryllAgentLogDocument::FileName(FApp::GetProjectName(), FDateTime::Now());
	TArray<FString> Chosen;
	if (!FTryllEditorPrompts::SaveFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			TEXT("Export Agent Log"), FPaths::ProjectSavedDir(),
			Name, TEXT("JSON|*.json"), EFileDialogFlags::None, Chosen)
		|| Chosen.Num() == 0)
	{
		return FReply::Handled();
	}

	FTryllAgentLogExportInfo Info;
	Info.HostVersion = FEngineVersion::Current().ToString();
	// Same number the Welcome and Help tabs show (a dev checkout reads VERSION).
	Info.PluginVersion = TryllCommunityWidgets::ClientVersion();
	Info.ProjectName = FApp::GetProjectName();
	Info.Os = FPlatformMisc::GetOSVersion();
	Info.CaptureState = Store->IsHistoryDiscarded()
		? TEXT("discarded")
		: (bUserPaused ? TEXT("paused") : (Store->IsSubscribed() ? TEXT("capturing") : TEXT("idle")));
	Info.SelectedAgentId = bAllAgentsSelected ? 0 : SelectedAgentId;
	if (EditorSubsystem != nullptr)
	{
		Info.ServerVersion = EditorSubsystem->ServerVersion;
		Info.ServerRunId = EditorSubsystem->ServerRunId;
		Info.ServerBuildConfig = EditorSubsystem->ServerBuildConfig;
		Info.ServerMode = EditorSubsystem->ServerMode;
	}
	const FString Json = FTryllAgentLogWriteStore::WriteStore(*Store, Info);
	FFileHelper::SaveStringToFile(Json, *Chosen[0], FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	return FReply::Handled();
}

FReply STryllAgentLogPanel::OnClearClicked()
{
	if (Store != nullptr)
	{
		PendingSelectionEventId = 0;
		SelectedTimelineItem.Reset();
		TurnCollapse.Clear();
		FullProjectedRows.Reset();
		Store->Clear();
		AttachIfNeeded();
	}
	return FReply::Handled();
}

FReply STryllAgentLogPanel::OnTurnChevronClicked(const FTryllTurnKey TurnKey, const bool bCurrentlyCollapsed)
{
	if (!TurnKey.bHasValue)
	{
		return FReply::Handled();
	}
	TurnCollapse.Toggle(TurnKey, bCurrentlyCollapsed);
	for (const TSharedPtr<FAgentLogTimelineItem>& Item : TimelineItems)
	{
		if (Item.IsValid() && Item->Row.TurnKey == TurnKey && Item->Row.bCanCollapseTurn)
		{
			PendingSelectionEventId = Item->EventId;
			break;
		}
	}
	RebuildTimelineItems();
	RefreshInspector();
	return FReply::Handled();
}

FReply STryllAgentLogPanel::OnPauseResumeClicked()
{
	if (Store == nullptr)
	{
		return FReply::Handled();
	}

	if (!bUserPaused)
	{
		bUserPaused = true;
		Store->Detach();
	}
	else
	{
		bUserPaused = false;
		AttachIfNeeded();
	}

	RefreshStatusChrome();
	return FReply::Handled();
}

FReply STryllAgentLogPanel::OnInspectorToggleClicked()
{
	bInspectorVisible = !bInspectorVisible;
	SaveLayoutConfig();
	// WidthOverride_Lambda alone can leave a stale splitter grip until the next
	// parent layout; force a pass so collapse-to-zero removes the handle.
	Invalidate(EInvalidateWidgetReason::Layout);
	return FReply::Handled();
}

void STryllAgentLogPanel::OnAgentSelectionChanged(
	TSharedPtr<FAgentLogAgentListItem> Item,
	ESelectInfo::Type /*SelectInfo*/)
{
	if (!Item.IsValid())
	{
		return;
	}

	SelectedAgentItem   = Item;
	bAllAgentsSelected  = Item->bAllAgents;
	SelectedAgentId     = Item->bAllAgents ? 0 : Item->AgentId;
	RebuildTimelineItems();
	TimelineTail.ResetFollowing();
	RefreshInspector();
	RefreshBanners();
}

void STryllAgentLogPanel::OnTimelineSelectionChanged(
	TSharedPtr<FAgentLogTimelineItem> Item,
	ESelectInfo::Type /*SelectInfo*/)
{
	SelectedTimelineItem = Item;
	PendingSelectionEventId = Item.IsValid() ? Item->EventId : 0;
	RefreshInspector();
}

void STryllAgentLogPanel::DebugRefreshForTests()
{
	AttachIfNeeded();
	RebuildAgentListItems();
	RefreshStatusChrome();
	RefreshBanners();
}

FTryllPanelSnapshot STryllAgentLogPanel::DebugSnapshotForTests() const
{
	FTryllPanelSnapshot Out;
	Out.OwnerPhase = EditorSubsystem ? EditorSubsystem->GetOwnerPhase() : ETryllOwnerPhase::Idle;
	Out.LocalState = Store && Store->IsSubscribed() ? TEXT("Capturing") : TEXT("Waiting");
	if (StatusBar.IsValid())
	{
		Out.StatusCaption = StatusBar->DebugCaption();
		Out.StatusAction = StatusBar->DebugActionText();
	}
	if (WaitingBanner.IsValid())
	{
		Out.BannerText = WaitingBanner->DebugText();
	}
	for (const TSharedPtr<FAgentLogAgentListItem>& Item : AgentListItems)
	{
		if (Item.IsValid() && !Item->bAllAgents)
		{
			Out.Rows.Add(Item->Title);
		}
	}
	Out.ListGeneration = Store ? Store->GetEnvelopeCount() : 0;
	return Out;
}

void STryllAgentLogPanel::LoadLayoutConfig()
{
	if (Store == nullptr || !bPersistLayout)
	{
		return;
	}

	const FAgentLogConfig Config = Store->GetConfig();
	AgentListWidth    = FMath::Max(Config.AgentListWidth, AgentListMinWidth);
	TimelineWidth     = FMath::Max(Config.TimelineWidth, TimelineMinWidth);
	InspectorWidth    = FMath::Max(Config.InspectorWidth, FTryllEditorStyle::InspectorMinWidth);
	bInspectorVisible = Config.bInspectorVisible;
	bCoalesceAnswerText     = Config.bCoalesceAnswerText;
	bCollapseCompletedTurns = Config.bCollapseCompletedTurns;
}

void STryllAgentLogPanel::SaveLayoutConfig() const
{
	if (Store == nullptr || !bPersistLayout)
	{
		return;
	}

	FAgentLogConfig Config = Store->GetConfig();
	Config.AgentListWidth    = AgentListWidth;
	Config.TimelineWidth     = TimelineWidth;
	Config.InspectorWidth    = InspectorWidth;
	Config.bInspectorVisible = bInspectorVisible;
	Config.bCoalesceAnswerText     = bCoalesceAnswerText;
	Config.bCollapseCompletedTurns = bCollapseCompletedTurns;
	Store->SetConfig(Config);
	Config.Save();
}

FAgentLogFilter STryllAgentLogPanel::CurrentFilter() const
{
	FAgentLogFilter Filter = bAllAgentsSelected
		? FTryllAgentLogInspection::MakeAllAgentsFilter(bRawMode)
		: FTryllAgentLogInspection::MakeSingleAgentFilter(SelectedAgentId, bRawMode);
	Filter.bCoalesceAnswerText = bCoalesceAnswerText;
	return Filter;
}

const FTryllObservationEnvelope* STryllAgentLogPanel::EnvelopeForRow(const FAgentLogRow& Row) const
{
	if (Store == nullptr || Row.EventId == 0)
	{
		return nullptr;
	}
	return Store->FindEnvelope(Row.EventId);
}

#undef LOCTEXT_NAMESPACE
