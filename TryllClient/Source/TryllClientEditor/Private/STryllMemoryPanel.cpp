// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "STryllMemoryPanel.h"
#include "TryllMemoryPresentation.h"
#include "TryllMemoryTableColumns.h"
#include "TryllStatusPresentation.h"
#include "TryllEditorUiContract.h"
#include "UI/STryllStatusBar.h"
#include "UI/STryllBanner.h"

#include "TryllRuntimeSettings.h"
#include "TryllEditorSubsystem.h"
#include "TryllEditorConnectionPolicy.h"
#include "UI/TryllEditorStyle.h"
#include "Editor.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Types/ISlateMetaData.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "HAL/PlatformMemory.h"

#define LOCTEXT_NAMESPACE "STryllMemoryPanel"

namespace
{
	const FSlateBrush* TableBrush()
	{
		return FTryllEditorStyle::Rounded(4.f, FTryllEditorStyle::Hairline, 1.f);
	}

	TSharedRef<SWidget> MakePill(const FString& Text, const FLinearColor& Fg)
	{
		return FTryllEditorStyle::Chip(
			FText::FromString(Text),
			Fg,
			FTryllEditorStyle::Wash(Fg, 0.14f),
			FTryllEditorStyle::Wash(Fg, 0.45f));
	}

	TSharedRef<SWidget> MakeCenteredColumnPill(const FString& Text, const FLinearColor& Fg)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.05f)
			+ SHorizontalBox::Slot().FillWidth(0.90f).HAlign(HAlign_Fill).VAlign(VAlign_Center)
			[ MakePill(Text, Fg) ]
			+ SHorizontalBox::Slot().FillWidth(0.05f);
	}

	TSharedRef<SWidget> Cell(const FString& Text, bool bMuted = false, bool bRight = false, bool bWarn = false)
	{
		FLinearColor Color = bWarn ? FTryllEditorStyle::ColWarn : (bMuted ? FTryllEditorStyle::Dim : FLinearColor::White);
		return SNew(SBox)
			.Padding(FMargin(0, 0, 6, 0))
			[
				SNew(STextBlock)
				.Text(FText::FromString(Text))
				.ColorAndOpacity(FSlateColor(Color))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.Justification(bRight ? ETextJustify::Right : ETextJustify::Left)
				.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
			];
	}

	TSharedRef<SWidget> HeaderCell(const FString& Text, bool bRight = false)
	{
		return SNew(SBox)
			.Padding(FMargin(0, 0, 6, 0))
			[
				SNew(STextBlock)
				.Text(FText::FromString(Text))
				.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.Justification(bRight ? ETextJustify::Right : ETextJustify::Left)
			];
	}

	FLinearColor ToneColor(FTryllMemoryPresentation::ETone Tone)
	{
		switch (Tone)
		{
		case FTryllMemoryPresentation::ETone::Ok: return FTryllEditorStyle::ColOk;
		case FTryllMemoryPresentation::ETone::Accent: return FTryllEditorStyle::Accent;
		case FTryllMemoryPresentation::ETone::Warn: return FTryllEditorStyle::ColWarn;
		case FTryllMemoryPresentation::ETone::Err: return FTryllEditorStyle::ColErr;
		default: return FTryllEditorStyle::Dim;
		}
	}

	TSharedRef<SWidget> Note(const FString& Text)
	{
		return SNew(STextBlock)
			.Text(FText::FromString(Text))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			.AutoWrapText(true);
	}

	int64 UnrealProcessRamBytes()
	{
		return static_cast<int64>(FPlatformMemory::GetStats().UsedPhysical);
	}
} // namespace

void STryllMemoryPanel::Construct(const FArguments& InArgs)
{
	EditorSubsystemOverride = InArgs._EditorSubsystemOverride;
	AutofindSettings();

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 6)[ BuildToolbar() ]
		+ SVerticalBox::Slot().AutoHeight()[ SNew(SSeparator) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 6, 8, 0)[ BuildBanner() ]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(8, 4)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(BodyBox, SVerticalBox)
			]
		]
	];

	Start();
	RebuildBody();
	HostColorsHandle = FTryllEditorStyle::OnHostColorsRefreshed.AddRaw(
		this, &STryllMemoryPanel::HandleHostColorsRefreshed);
}

STryllMemoryPanel::~STryllMemoryPanel()
{
	if (HostColorsHandle.IsValid())
	{
		FTryllEditorStyle::OnHostColorsRefreshed.Remove(HostColorsHandle);
		HostColorsHandle.Reset();
	}
	if (State != EState::Idle)
	{
		Stop();
	}
}

void STryllMemoryPanel::HandleHostColorsRefreshed()
{
	RebuildBody();
	RefreshStatusChrome();
}

TSharedRef<SWidget> STryllMemoryPanel::BuildToolbar()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Title", "Tryll Memory"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
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
			.AddMetaData<FTagMetaData>(FTagMetaData(TryllEditorWidgetIds::Refresh))
			.Text(LOCTEXT("Refresh", "Refresh"))
			.IsEnabled(this, &STryllMemoryPanel::CanRefresh)
			.OnClicked(this, &STryllMemoryPanel::OnRefreshClicked)
		];
}

TSharedRef<SWidget> STryllMemoryPanel::BuildBanner()
{
	return SAssignNew(Banner, STryllBanner);
}

void STryllMemoryPanel::RebuildBody()
{
	if (!BodyBox.IsValid())
	{
		return;
	}
	BodyBox->ClearChildren();

	const bool bReady = State == EState::Ready;
	const int64 ProcessRam = UnrealProcessRamBytes();
	const int64 NowMs = FDateTime::UtcNow().ToUnixTimestamp() * 1000;
	FTryllMemoryPresentation::FTables Tables;
	if (Snapshot.IsSet())
	{
		Tables = FTryllMemoryPresentation::Project(Snapshot.GetValue(), NowMs);
	}

	auto AddSlot = [this](const TSharedRef<SWidget>& W, float BottomPad = 14.f)
	{
		BodyBox->AddSlot().AutoHeight().Padding(0, 0, 0, BottomPad)[ W ];
	};

	if (!bReady && !Snapshot.IsSet())
	{
		AddSlot(Note(TEXT("Not connected. The window will launch or reuse the shared Tryll session. Unreal process RAM is shown below.")));
	}
	else if (bReady && !Snapshot.IsSet())
	{
		AddSlot(Note(TEXT("Click Refresh to take a snapshot. This is a pull — nothing updates until you ask.")));
	}

	// ── System ──────────────────────────────────────────────────────────────
	{
		TSharedRef<SVerticalBox> Sys = SNew(SVerticalBox);
		Sys->AddSlot().AutoHeight().Padding(0, 0, 0, 2)[ FTryllEditorStyle::Heading(FText::FromString(TEXT("System memory"))) ];

		TSharedRef<SVerticalBox> Table = SNew(SVerticalBox);
		auto AddRow = [&](const TSharedRef<SWidget>& A, const TSharedRef<SWidget>& B, const TSharedRef<SWidget>& C, bool bHeader)
		{
			Table->AddSlot().AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FSlateColor(bHeader ? FTryllEditorStyle::HeaderBg : FLinearColor::Transparent))
				.Padding(FMargin(8, 6))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::System[0].Weight).VAlign(VAlign_Center)[ A ]
					+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::System[1].Weight).VAlign(VAlign_Center)[ B ]
					+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::System[2].Weight).VAlign(VAlign_Center)[ C ]
				]
			];
		};
		AddRow(
			HeaderCell(FTryllMemoryTableColumns::System[0].Header),
			HeaderCell(FTryllMemoryTableColumns::System[1].Header, true),
			HeaderCell(FTryllMemoryTableColumns::System[2].Header),
			true);
		AddRow(
			Cell(TEXT("Unreal process RAM")),
			Cell(FTryllMemoryPresentation::FmtBinaryBytes(ProcessRam), false, true),
			Cell(TEXT("This editor process — RAM, not editor VRAM"), true),
			false);
		if (Snapshot.IsSet())
		{
			const FTryllMemorySnapshot& M = Snapshot.GetValue();
			AddRow(
				Cell(TEXT("Working set")),
				Cell(FTryllMemoryPresentation::FmtBinaryBytes(M.ProcessWorkingSetBytes), false, true),
				Cell(TEXT("Resident pages for tryll_server"), true),
				false);
			AddRow(
				Cell(TEXT("Private")),
				Cell(FTryllMemoryPresentation::FmtBinaryBytes(M.ProcessPrivateBytes), false, true),
				Cell(TEXT("Commit that is not shareable"), true),
				false);
			AddRow(
				Cell(TEXT("Unattributed")),
				Cell(FTryllMemoryPresentation::FmtBinaryBytes(M.UnattributedRamBytes), false, true),
				Cell(TEXT("Working set minus attributed RAM"), true),
				false);
		}

		Sys->AddSlot().AutoHeight()
		[
			SNew(SBorder).BorderImage(TableBrush()).BorderBackgroundColor(FSlateColor(FTryllEditorStyle::PanelTint)).Padding(0)[ Table ]
		];

		for (const FString& SysNote : Tables.SystemNotes)
		{
			Sys->AddSlot().AutoHeight().Padding(0, 6, 0, 0)
			[ Note(SysNote) ];
		}

		AddSlot(Sys);
	}

	if (!Snapshot.IsSet())
	{
		return;
	}

	// ── Devices ─────────────────────────────────────────────────────────────
	{
		TSharedRef<SVerticalBox> Dev = SNew(SVerticalBox);
		Dev->AddSlot().AutoHeight().Padding(0, 0, 0, 2)[ FTryllEditorStyle::Heading(FText::FromString(TEXT("Devices"))) ];
		if (Tables.DevicesKind == FTryllMemoryPresentation::EDeviceSection::Uninitialized)
		{
			Dev->AddSlot().AutoHeight()
			[ Note(FTryllMemoryPresentation::DevicesUninitializedNote) ];
		}
		else if (Tables.DevicesKind == FTryllMemoryPresentation::EDeviceSection::Empty)
		{
			Dev->AddSlot().AutoHeight()
			[ Note(FTryllMemoryPresentation::DevicesEmptyNote) ];
		}
		else
		{
			TSharedRef<SVerticalBox> Table = SNew(SVerticalBox);
			auto AddDevRow = [&](TSharedRef<SWidget> A, TSharedRef<SWidget> B, TSharedRef<SWidget> C,
				TSharedRef<SWidget> D, TSharedRef<SWidget> E, TSharedRef<SWidget> F, bool bHeader)
			{
				Table->AddSlot().AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FSlateColor(bHeader ? FTryllEditorStyle::HeaderBg : FLinearColor::Transparent))
					.Padding(FMargin(8, 6))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Devices[0].Weight).VAlign(VAlign_Center)[ A ]
						+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Devices[1].Weight).VAlign(VAlign_Center)[ B ]
						+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Devices[2].Weight).VAlign(VAlign_Center)[ C ]
						+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Devices[3].Weight).VAlign(VAlign_Center)[ D ]
						+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Devices[4].Weight).VAlign(VAlign_Center)[ E ]
						+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Devices[5].Weight).VAlign(VAlign_Center)[ F ]
					]
				];
			};
			AddDevRow(
				HeaderCell(FTryllMemoryTableColumns::Devices[0].Header),
				HeaderCell(FTryllMemoryTableColumns::Devices[1].Header),
				HeaderCell(FTryllMemoryTableColumns::Devices[2].Header, true),
				HeaderCell(FTryllMemoryTableColumns::Devices[3].Header, true),
				HeaderCell(FTryllMemoryTableColumns::Devices[4].Header, true),
				HeaderCell(FTryllMemoryTableColumns::Devices[5].Header, true),
				true);
			for (const FTryllMemoryPresentation::FDeviceRow& D : Tables.Devices)
			{
				AddDevRow(
					Cell(D.Label),
					MakePill(D.Kind, FTryllEditorStyle::Dim),
					Cell(D.Self, false, true),
					Cell(D.Other, false, true),
					Cell(D.Free, false, true),
					Cell(D.Total, false, true),
					false);
			}
			Dev->AddSlot().AutoHeight()
			[ SNew(SBorder).BorderImage(TableBrush()).BorderBackgroundColor(FSlateColor(FTryllEditorStyle::PanelTint)).Padding(0)[ Table ] ];
			Dev->AddSlot().AutoHeight().Padding(0, 6, 0, 0)
			[ Note(FTryllMemoryPresentation::DevicesFooterNote) ];
		}
		AddSlot(Dev);
	}

	// ── KV by agent ─────────────────────────────────────────────────────────
	{
		TSharedRef<SVerticalBox> Block = SNew(SVerticalBox);
		Block->AddSlot().AutoHeight().Padding(0, 0, 0, 2)
		[
			FTryllEditorStyle::Heading(FText::FromString(FString::Printf(
				TEXT("KV cache by agent    %s"), *Tables.KvTotalLabel)))
		];
		Block->AddSlot().AutoHeight().Padding(0, 0, 0, 4)
		[
			Note(FTryllMemoryPresentation::ContextHeadroomNote)
		];

		if (Tables.KvRows.Num() == 0)
		{
			Block->AddSlot().AutoHeight()
			[ Note(FTryllMemoryPresentation::EmptyKvNote) ];
		}
		else
		{
			TSharedRef<SVerticalBox> Table = SNew(SVerticalBox);
			Table->AddSlot().AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::HeaderBg))
				.Padding(FMargin(8, 5))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(static_cast<float>(FTryllMemoryTableColumns::KvGutterPx))[ SNullWidget::NullWidget ]
					]
					+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Kv[0].Weight).VAlign(VAlign_Center)
					[ HeaderCell(FTryllMemoryTableColumns::Kv[0].Header) ]
					+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Kv[1].Weight).VAlign(VAlign_Center)
					[ HeaderCell(FTryllMemoryTableColumns::Kv[1].Header) ]
					+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Kv[2].Weight).VAlign(VAlign_Center)
					[ HeaderCell(FTryllMemoryTableColumns::Kv[2].Header) ]
					+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Kv[3].Weight).VAlign(VAlign_Center)
					[ HeaderCell(FTryllMemoryTableColumns::Kv[3].Header) ]
					+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Kv[4].Weight).VAlign(VAlign_Center)
					[ HeaderCell(FTryllMemoryTableColumns::Kv[4].Header, true) ]
				]
			];

			for (const FTryllMemoryPresentation::FKvRow& C : Tables.KvRows)
			{
				TSharedRef<SWidget> ContextCell = Cell(TEXT("—"), true);
				if (C.CapacityTokens > 0)
				{
					ContextCell = SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 6, 3)
						[
							SNew(SBox).HeightOverride(7.f)
							[
								SNew(SProgressBar)
								.Percent(TOptional<float>(C.OccupancyFrac))
								.FillColorAndOpacity(FSlateColor(C.bOccupancyStale ? FTryllEditorStyle::Dim : FTryllEditorStyle::Accent))
							]
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(C.ContextCaption))
							.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						];
				}

				Table->AddSlot().AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FSlateColor(FLinearColor::Transparent))
					.Padding(FMargin(8, 6))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SBox)
							.WidthOverride(static_cast<float>(FTryllMemoryTableColumns::KvGutterPx))
							[
								SNew(SBorder)
								.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
								.BorderBackgroundColor(FSlateColor(C.bBackground ? FTryllEditorStyle::ColWarn : FLinearColor::Transparent))
							]
						]
						+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Kv[0].Weight).VAlign(VAlign_Center)
						[ Cell(C.Name) ]
						+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Kv[1].Weight).VAlign(VAlign_Center)
						[ MakeCenteredColumnPill(C.ClassLabel, ToneColor(C.ClassTone)) ]
						+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Kv[2].Weight).VAlign(VAlign_Center)
						[ Cell(C.When, true) ]
						+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Kv[3].Weight).VAlign(VAlign_Center)[ ContextCell ]
						+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Kv[4].Weight).VAlign(VAlign_Center)
						[ Cell(C.Reserved, false, true) ]
					]
				];
			}

			Block->AddSlot().AutoHeight()
			[ SNew(SBorder).BorderImage(TableBrush()).BorderBackgroundColor(FSlateColor(FTryllEditorStyle::PanelTint)).Padding(0)[ Table ] ];
			if (!Tables.BackgroundKvNote.IsEmpty())
			{
				Block->AddSlot().AutoHeight().Padding(0, 6, 0, 0)
				[ Note(Tables.BackgroundKvNote) ];
			}
		}
		AddSlot(Block);
	}

	// ── Components ──────────────────────────────────────────────────────────
	{
		TSharedRef<SVerticalBox> Comps = SNew(SVerticalBox);
		Comps->AddSlot().AutoHeight().Padding(0, 0, 0, 2)[ FTryllEditorStyle::Heading(FText::FromString(TEXT("Components"))) ];

		TSharedRef<SVerticalBox> Table = SNew(SVerticalBox);
		Table->AddSlot().AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::HeaderBg))
			.Padding(FMargin(8, 5))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Components[0].Weight).VAlign(VAlign_Center)
				[ HeaderCell(FTryllMemoryTableColumns::Components[0].Header) ]
				+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Components[1].Weight).VAlign(VAlign_Center)
				[ HeaderCell(FTryllMemoryTableColumns::Components[1].Header) ]
				+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Components[2].Weight).VAlign(VAlign_Center)
				[ HeaderCell(FTryllMemoryTableColumns::Components[2].Header) ]
				+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Components[3].Weight).VAlign(VAlign_Center)
				[ HeaderCell(FTryllMemoryTableColumns::Components[3].Header, true) ]
				+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Components[4].Weight).VAlign(VAlign_Center)
				[ HeaderCell(FTryllMemoryTableColumns::Components[4].Header, true) ]
			]
		];

		for (const FTryllMemoryPresentation::FComponentRow& C : Tables.ComponentRows)
		{
			Table->AddSlot().AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FSlateColor(FLinearColor::Transparent))
				.Padding(FMargin(8, 6))
				.ToolTipText(C.Tooltip.IsEmpty() ? FText::GetEmpty() : FText::FromString(C.Tooltip))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Components[0].Weight).VAlign(VAlign_Center)
					[ Cell(C.Kind, true) ]
					+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Components[1].Weight).VAlign(VAlign_Center)
					[ Cell(C.Name) ]
					+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Components[2].Weight).VAlign(VAlign_Center)
					[ MakePill(C.TierLabel, ToneColor(C.TierTone)) ]
					+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Components[3].Weight).VAlign(VAlign_Center)
					[ Cell(C.Ram, false, true, C.bRamWarn) ]
					+ SHorizontalBox::Slot().FillWidth(FTryllMemoryTableColumns::Components[4].Weight).VAlign(VAlign_Center)
					[ Cell(C.Vram, false, true) ]
				]
			];
		}

		Comps->AddSlot().AutoHeight()
		[ SNew(SBorder).BorderImage(TableBrush()).BorderBackgroundColor(FSlateColor(FTryllEditorStyle::PanelTint)).Padding(0)[ Table ] ];
		AddSlot(Comps);
	}
}

void STryllMemoryPanel::RefreshStatusChrome()
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

	const ETryllPanelOperation LocalOp = bRefreshInFlight
		? ETryllPanelOperation::Refreshing
		: ETryllPanelOperation::None;
	StatusBar->Bind(FTryllStatusPresentation::Compose(
		Owner, Detail, LocalOp, ETryllEditorPanel::Memory));

	if (Banner.IsValid())
	{
		const FString Stale = FTryllMemoryPresentation::StaleBannerText(Snapshot, StaleError);
		if (!Stale.IsEmpty())
		{
			Banner->Bind(Stale, ETryllStatusSeverity::Warning);
		}
		else
		{
			Banner->Clear();
		}
	}
}

void STryllMemoryPanel::HandleOwnerPhaseChanged(ETryllOwnerPhase /*Phase*/, const FString& /*Detail*/)
{
	RefreshStatusChrome();
}

bool STryllMemoryPanel::CanRefresh() const
{
	return State == EState::Ready && !bRefreshInFlight;
}

FReply STryllMemoryPanel::OnRefreshClicked()
{
	if (!CanRefresh() || !EditorSubsystem)
	{
		return FReply::Handled();
	}

	{
		FTryllMemoryPresentationState Pres{ Snapshot, StaleError, bRefreshInFlight };
		Pres = FTryllMemoryPresentation::ApplyRefreshStarted(MoveTemp(Pres));
		Snapshot = MoveTemp(Pres.Snapshot);
		StaleError = MoveTemp(Pres.StaleError);
		bRefreshInFlight = Pres.bRefreshInFlight;
	}
	RefreshStatusChrome();
	const int32 Generation = ++FetchGeneration;
	TWeakPtr<STryllMemoryPanel> WeakPanel = SharedThis(this);
	EditorSubsystem->RequestGetMemoryConsumption(
		[WeakPanel, Generation](FTryllMemorySnapshot NewSnap, FTryllError Error)
		{
			TSharedPtr<STryllMemoryPanel> Panel = WeakPanel.Pin();
			if (!Panel.IsValid() || Generation != Panel->FetchGeneration)
			{
				return;
			}
			FTryllMemoryPresentationState Pres{ Panel->Snapshot, Panel->StaleError, Panel->bRefreshInFlight };
			Pres = Error.IsOk()
				? FTryllMemoryPresentation::ApplyRefreshSucceeded(MoveTemp(Pres), MoveTemp(NewSnap))
				: FTryllMemoryPresentation::ApplyRefreshFailed(MoveTemp(Pres), Error.Message);
			Panel->Snapshot = MoveTemp(Pres.Snapshot);
			Panel->StaleError = MoveTemp(Pres.StaleError);
			Panel->bRefreshInFlight = Pres.bRefreshInFlight;
			Panel->RefreshStatusChrome();
			Panel->RebuildBody();
		});
	return FReply::Handled();
}

void STryllMemoryPanel::SetState(EState S, const FString& InStatus)
{
	State  = S;
	Status = InStatus;
	RefreshStatusChrome();
}

void STryllMemoryPanel::Fail(const FString& Reason)
{
	SetState(EState::Error, Reason);
	// Keep the last good table, but mark it stale — otherwise a disconnect
	// leaves numbers on screen that read as current.
	{
		FTryllMemoryPresentationState Pres{ Snapshot, StaleError, bRefreshInFlight };
		Pres = FTryllMemoryPresentation::ApplyDisconnected(MoveTemp(Pres), Reason);
		Snapshot = MoveTemp(Pres.Snapshot);
		StaleError = MoveTemp(Pres.StaleError);
		bRefreshInFlight = Pres.bRefreshInFlight;
	}
	RebuildBody();
}

void STryllMemoryPanel::AutofindSettings()
{
	if (Settings.IsValid())
	{
		return;
	}
	Settings = GetMutableDefault<UTryllRuntimeSettings>();
}

void STryllMemoryPanel::RetryOwner()
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

void STryllMemoryPanel::Start()
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

	EditorSubsystem->OnConnectionChangedNative.AddSP(this, &STryllMemoryPanel::HandleConnectionChanged);
	EditorSubsystem->OnCreateSessionCompleteNative.AddSP(this, &STryllMemoryPanel::HandleCreateSessionComplete);
	EditorSubsystem->OnErrorNative.AddSP(this, &STryllMemoryPanel::HandleError);
	OwnerPhaseHandle = EditorSubsystem->OnOwnerPhaseChangedNative.AddSP(
		this, &STryllMemoryPanel::HandleOwnerPhaseChanged);

	SetState(EState::Connecting, FTryllEditorUiContract::StatusConnecting);
	bAcquiredConnection = true;
	EditorSubsystem->AcquireConnection();

	if (EditorSubsystem->IsConnected())
	{
		HandleConnectionChanged(true);
	}
}

void STryllMemoryPanel::Stop()
{
	++FetchGeneration;
	bRefreshInFlight = false;

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

void STryllMemoryPanel::HandleConnectionChanged(bool bConnected)
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

void STryllMemoryPanel::HandleCreateSessionComplete(bool bSuccess)
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
	RebuildBody();
}

void STryllMemoryPanel::HandleError(const FString& ErrorMessage)
{
	LastErrorMessage = ErrorMessage;
}

FTryllMemoryPresentation::FTables STryllMemoryPanel::DebugTablesForTests() const
{
	return Snapshot.IsSet()
		? FTryllMemoryPresentation::Project(Snapshot.GetValue(), FDateTime::UtcNow().ToUnixTimestamp() * 1000)
		: FTryllMemoryPresentation::FTables();
}

FTryllPanelSnapshot STryllMemoryPanel::DebugSnapshotForTests() const
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
	if (Snapshot.IsSet())
	{
		for (const FTryllMemoryComponent& Component : Snapshot->Components)
		{
			Out.Rows.Add(Component.Name);
		}
	}
	Out.bLeaseHeld = bAcquiredConnection;
	Out.ListGeneration = FetchGeneration;
	return Out;
}

#undef LOCTEXT_NAMESPACE
