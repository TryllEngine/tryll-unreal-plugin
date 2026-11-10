// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "Welcome/STryllWelcomePanel.h"

#include "Welcome/STryllHelpPanel.h"
#include "Welcome/TryllCommunityLinks.h"
#include "Welcome/TryllCommunityWidgets.h"
#include "Welcome/TryllWelcomeStartup.h"
#include "TryllEditorUiContract.h"
#include "TryllLicence.h"
#include "TryllRuntimeSettings.h"
#include "TryllSubsystem.h"
#include "UI/TryllEditorStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Types/ISlateMetaData.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

// Named, not anonymous: UBT unity builds merge this file with others in the module.
namespace TryllWelcomePanelPrivate
{
/** FTagMetaData tags on the controls automation touches. Engine-local. */
const TCHAR* const TagAccept = TEXT("Tryll.Welcome.LicenceAccept");
const TCHAR* const TagWithdraw = TEXT("Tryll.Welcome.LicenceWithdraw");
const TCHAR* const TagRead = TEXT("Tryll.Welcome.LicenceRead");
const TCHAR* const TagLocalCopy = TEXT("Tryll.Welcome.LicenceLocalCopy");
const TCHAR* const TagTelemetry = TEXT("Tryll.Welcome.Telemetry");
const TCHAR* const TagOpenHelp = TEXT("Tryll.Welcome.OpenHelp");

// Status markers: ASCII, never colour alone (editor-window-checklist "Theme").
const TCHAR* const MarkDone = TEXT("* ");
const TCHAR* const MarkOpen = TEXT("o ");

// Text, buttons, chips, pills and rows shared with the Help & Community tab.
using namespace TryllCommunityWidgets;

FText ContractText(const TCHAR* Value)
{
	return FText::FromString(Value);
}

/** Card with a numbered badge, a title, a status pill and an accent edge (ok when done). */
TSharedRef<SWidget> StepCard(int32 Number, const TCHAR* Header, bool bDone, const TCHAR* PillText, ETone Tone,
	const TSharedRef<SWidget>& Content)
{
	const float R = FTryllEditorStyle::RadiusCell;
	const FLinearColor Edge = bDone ? FTryllEditorStyle::ColOk : FTryllEditorStyle::Accent;
	return SNew(SBorder)
		.BorderImage(FTryllEditorStyle::Rounded(R, FTryllEditorStyle::Line, 1.0f))
		.BorderBackgroundColor(FSlateColor(FTryllEditorStyle::Panel))
		.Padding(FMargin(1.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(3.0f)
				[
					SNew(SBorder)
					.BorderImage(FTryllEditorStyle::RoundedCorners(FVector4(R, 0.0f, 0.0f, R)))
					.BorderBackgroundColor(FSlateColor(Edge))
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(FMargin(12.0f, 9.0f, 13.0f, 6.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(20.0f).HeightOverride(20.0f)
						[
							SNew(SBorder)
							.BorderImage(FTryllEditorStyle::Rounded(10.0f))
							.BorderBackgroundColor(FSlateColor(Edge))
							.Padding(0.0f)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::AsNumber(Number))
								.ColorAndOpacity(FSlateColor(FTryllEditorStyle::BandText))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
							]
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(ContractText(Header))
						.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Txt))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						Pill(PillText, Tone)
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					Content
				]
			]
		];
}

} // namespace TryllWelcomePanelPrivate

// ── Construction ───────────────────────────────────────────────────────────────

TSharedRef<SDockTab> STryllWelcomePanel::SpawnTab(const FSpawnTabArgs& /*Args*/)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(STryllWelcomePanel)
		];
}

void STryllWelcomePanel::Construct(const FArguments& /*InArgs*/)
{
	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot().Padding(FMargin(16.0f, 14.0f, 16.0f, 12.0f))
		[
			SAssignNew(BodyBox, SVerticalBox)
		]
	];

	// Withdraw and accept from anywhere (this tab, the viewer, another window) land here.
	LicenceChangedHandle = FTryllLicence::OnChanged().AddSP(this, &STryllWelcomePanel::Refresh, true);
	HostColorsHandle = FTryllEditorStyle::OnHostColorsRefreshed.AddSP(
		this, &STryllWelcomePanel::HandleHostColorsRefreshed);
	Refresh(/*bForce=*/true);
}

STryllWelcomePanel::~STryllWelcomePanel()
{
	if (LicenceChangedHandle.IsValid())
	{
		FTryllLicence::OnChanged().Remove(LicenceChangedHandle);
		LicenceChangedHandle.Reset();
	}
	if (HostColorsHandle.IsValid())
	{
		FTryllEditorStyle::OnHostColorsRefreshed.Remove(HostColorsHandle);
		HostColorsHandle.Reset();
	}
}

void STryllWelcomePanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	// A server zip unpacked, or a licence accepted in another editor: re-read now and then.
	// The page rebuilds only when what it shows changed.
	if (InCurrentTime >= NextPollTime)
	{
		NextPollTime = InCurrentTime + 1.0;
		Refresh(/*bForce=*/false);
	}
}

void STryllWelcomePanel::HandleHostColorsRefreshed()
{
	// Colours are baked into the built widgets; rebuild with the same state.
	RebuildBody();
}

// ── State ──────────────────────────────────────────────────────────────────────

STryllWelcomePanel::EServerCard STryllWelcomePanel::ServerCardFor(bool bLicenceAccepted, bool bServerExeFound)
{
	if (!bLicenceAccepted)
	{
		return EServerCard::AfterLicence;
	}
	return bServerExeFound ? EServerCard::Bundled : EServerCard::NotFound;
}

int32 STryllWelcomePanel::DoneSteps(bool bLicenceAccepted, EServerCard Server)
{
	return (bLicenceAccepted ? 1 : 0) + (Server == EServerCard::Bundled ? 1 : 0);
}

bool STryllWelcomePanel::IsServerExeFound() const
{
	if (DebugServerFound.IsSet())
	{
		return DebugServerFound.GetValue();
	}
	const UTryllRuntimeSettings* Settings = GetDefault<UTryllRuntimeSettings>();
	const FString Exe = UTryllSubsystem::ResolveServerExePath(
		Settings != nullptr ? Settings->ServerBuildVariant : EServerBuildVariant::Default);
	return !Exe.IsEmpty() && FPaths::FileExists(Exe);
}

STryllWelcomePanel::FState STryllWelcomePanel::ReadState() const
{
	FState Next;
	Next.bAccepted = FTryllLicence::IsAccepted();
	Next.bHasLicenceFile = FTryllLicence::HasLicenceFile();
	Next.Server = ServerCardFor(Next.bAccepted, Next.bAccepted && IsServerExeFound());
	return Next;
}

void STryllWelcomePanel::Refresh(bool bForce)
{
	const FState Next = ReadState();
	if (!bForce && Next == State)
	{
		return;
	}
	State = Next;
	RebuildBody();
}

void STryllWelcomePanel::DebugSetServerFoundForTests(TOptional<bool> bFound)
{
	DebugServerFound = bFound;
	Refresh(/*bForce=*/true);
}

// ── Layout ─────────────────────────────────────────────────────────────────────

void STryllWelcomePanel::RebuildBody()
{
	if (!BodyBox.IsValid())
	{
		return;
	}
	BodyBox->ClearChildren();

	auto Add = [this](const TSharedRef<SWidget>& Widget, float Bottom)
	{
		BodyBox->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, Bottom)[ Widget ];
	};
	Add(BuildBrandHeader(), 14.0f);
	Add(BuildHero(), 12.0f);
	Add(BuildSetupHeader(), 6.0f);
	Add(BuildLicenceStep(), 10.0f);
	Add(BuildServerStep(), 10.0f);
	Add(BuildCommunity(), 8.0f);
	Add(BuildFooter(), 0.0f);
}

TSharedRef<SWidget> STryllWelcomePanel::BuildBrandHeader()
{
	return TryllCommunityWidgets::BrandHeader(bLogoShown);
}

TSharedRef<SWidget> STryllWelcomePanel::BuildHero()
{
	using namespace TryllWelcomePanelPrivate;
	TSharedRef<SWrapBox> Actions = Row();
	Actions->AddSlot()
	[
		PrimaryButton(FTryllEditorUiContract::WelcomeGetStarted,
			[]() { FTryllCommunityLinks::OpenUrl(FTryllCommunityLinks::GettingStarted); })
	];
	if (*FTryllCommunityLinks::Discord != TEXT('\0'))
	{
		Actions->AddSlot()
		[
			SecondaryButton(FTryllEditorUiContract::WelcomeJoinDiscord,
				[]() { FTryllCommunityLinks::OpenUrl(FTryllCommunityLinks::Discord); },
				nullptr, FTryllCommunityLinks::Discord)
		];
	}

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(ContractText(FTryllEditorUiContract::WelcomeTitle))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Txt))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			Paragraph(FTryllEditorUiContract::WelcomeIntro, FTryllEditorStyle::Dim)
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			Actions
		];
}

TSharedRef<SWidget> STryllWelcomePanel::BuildSetupHeader()
{
	using namespace TryllWelcomePanelPrivate;
	const int32 Done = DoneSteps(State.bAccepted, State.Server);
	SetupProgressText = FString(FTryllEditorUiContract::WelcomeSetupProgress)
		.Replace(TEXT("{done}"), *FString::FromInt(Done))
		.Replace(TEXT("{total}"), *FString::FromInt(SetupSteps));

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
		[
			SectionLabel(FTryllEditorUiContract::WelcomeSetupHeader)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SectionLabel(SetupProgressText)
		];
}

void STryllWelcomePanel::AddLicenceLinks(const TSharedRef<SWrapBox>& Target, bool bAsLinks)
{
	using namespace TryllWelcomePanelPrivate;
	auto Make = [bAsLinks](const TCHAR* Label, TFunction<void()> OnClick, const TCHAR* MetaTag, const FString& Tooltip)
	{
		return bAsLinks
			? LinkButton(Label, MoveTemp(OnClick), MetaTag, Tooltip)
			: SecondaryButton(Label, MoveTemp(OnClick), MetaTag, Tooltip);
	};
	TWeakPtr<STryllWelcomePanel> Weak = SharedThis(this);

	// Without a LICENSE.md there is nothing to read; Accept still works. "Read" opens this
	// version's docs page, which renders the same file; the local copy works offline.
	bReadLicenceShown = State.bHasLicenceFile;
	if (bReadLicenceShown)
	{
		Target->AddSlot().VAlign(VAlign_Center)
		[
			Make(FTryllEditorUiContract::WelcomeLicenceRead,
				[Weak]() { if (TSharedPtr<STryllWelcomePanel> Self = Weak.Pin()) { Self->OnReadLicence(); } },
				TagRead, FTryllCommunityLinks::Licence)
		];
		Target->AddSlot().VAlign(VAlign_Center)
		[
			Make(FTryllEditorUiContract::WelcomeLicenceLocalCopy,
				[Weak]() { if (TSharedPtr<STryllWelcomePanel> Self = Weak.Pin()) { Self->OnOpenLocalLicence(); } },
				TagLocalCopy, FTryllLicence::LicenceFilePath())
		];
	}
	bTelemetryShown = true;
	Target->AddSlot().VAlign(VAlign_Center)
	[
		Make(FTryllEditorUiContract::WelcomeTelemetryLink,
			[Weak]() { if (TSharedPtr<STryllWelcomePanel> Self = Weak.Pin()) { Self->OnOpenTelemetry(); } },
			TagTelemetry, FTryllCommunityLinks::Telemetry)
	];
}

TSharedRef<SWidget> STryllWelcomePanel::BuildLicenceStep()
{
	using namespace TryllWelcomePanelPrivate;
	TWeakPtr<STryllWelcomePanel> Weak = SharedThis(this);
	const bool bAccepted = State.bAccepted;

	TSharedRef<SVerticalBox> Content = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			Paragraph(FTryllEditorUiContract::WelcomeLicenceBody, FTryllEditorStyle::Txt)
		]
		// Its own bold line: developers worry most about what the shipped game sends.
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FTryllEditorUiContract::WelcomeLicencePlayers))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Txt))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			.AutoWrapText(true)
		];

	bLicencePendingShown = !State.bHasLicenceFile;
	if (bLicencePendingShown)
	{
		Content->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			Paragraph(FTryllEditorUiContract::WelcomeLicencePending, FTryllEditorStyle::Dim)
		];
	}

	TSharedRef<SWrapBox> Actions = Row();
	bAcceptShown = !bAccepted;
	bWithdrawShown = bAccepted;
	if (!bAccepted)
	{
		Actions->AddSlot().VAlign(VAlign_Center)
		[
			PrimaryButton(FTryllEditorUiContract::WelcomeLicenceAccept,
				[Weak]() { if (TSharedPtr<STryllWelcomePanel> Self = Weak.Pin()) { Self->OnLicenceButton(true); } },
				TagAccept)
		];
		AddLicenceLinks(Actions, /*bAsLinks=*/false);
	}
	else
	{
		Actions->AddSlot().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 4.0f, 0.0f)
		[
			Paragraph(FString(MarkDone) + FTryllEditorUiContract::WelcomeLicenceAccepted, FTryllEditorStyle::ColOk)
		];
		Actions->AddSlot().VAlign(VAlign_Center)
		[
			LinkButton(FTryllEditorUiContract::WelcomeLicenceWithdraw,
				[Weak]() { if (TSharedPtr<STryllWelcomePanel> Self = Weak.Pin()) { Self->OnLicenceButton(false); } },
				TagWithdraw)
		];
		AddLicenceLinks(Actions, /*bAsLinks=*/true);
	}
	Content->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)[ Actions ];

	return StepCard(1, FTryllEditorUiContract::WelcomeLicenceHeader, bAccepted,
		bAccepted ? FTryllEditorUiContract::WelcomeStepDone : FTryllEditorUiContract::WelcomeStepRequired,
		bAccepted ? ETone::Ok : ETone::Warn,
		Content);
}

TSharedRef<SWidget> STryllWelcomePanel::BuildServerStep()
{
	using namespace TryllWelcomePanelPrivate;
	FLinearColor Color = FTryllEditorStyle::Dim;
	switch (State.Server)
	{
	case EServerCard::Bundled:
		ServerStatusText = FString(MarkDone) + FTryllEditorUiContract::WelcomeServerBundled;
		Color = FTryllEditorStyle::ColOk;
		break;
	case EServerCard::NotFound:
		ServerStatusText = FString(MarkOpen) + FTryllEditorUiContract::WelcomeServerNotFound;
		Color = FTryllEditorStyle::ColWarn;
		break;
	default:
		// A server on disk starts only after the licence is accepted.
		ServerStatusText = FString(MarkOpen) + FTryllEditorUiContract::WelcomeServerAfterSetup;
		break;
	}
	const bool bReady = State.Server == EServerCard::Bundled;
	return StepCard(2, FTryllEditorUiContract::WelcomeServerHeader, bReady,
		bReady ? FTryllEditorUiContract::WelcomeStepDone : FTryllEditorUiContract::WelcomeStepRequired,
		bReady ? ETone::Ok : ETone::Warn,
		SNew(SBox).Padding(FMargin(0.0f, 0.0f, 0.0f, 6.0f))
		[
			Paragraph(ServerStatusText, Color)
		]);
}

TSharedRef<SWidget> STryllWelcomePanel::BuildCommunity()
{
	using namespace TryllWelcomePanelPrivate;
	TSharedRef<SWrapBox> Chips = Row();
	const TArray<TPair<FString, FString>> Links = FTryllCommunityLinks::CommunityLinks();
	CommunityLinkCount = Links.Num();
	for (const TPair<FString, FString>& Link : Links)
	{
		Chips->AddSlot()[ LinkChip(Link.Key, Link.Value) ];
	}
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			SectionLabel(FTryllEditorUiContract::WelcomeCommunityHeader)
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			Chips
		];
}

TSharedRef<SWidget> STryllWelcomePanel::BuildFooter()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			SNew(SSeparator).Thickness(1.0f).ColorAndOpacity(FSlateColor(FTryllEditorStyle::Line))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([]()
				{
					return FTryllWelcomeStartup::GetShowAtStartup() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([](ECheckBoxState NewState)
				{
					FTryllWelcomeStartup::SetShowAtStartup(NewState == ECheckBoxState::Checked);
				})
				[
					TryllWelcomePanelPrivate::Paragraph(FTryllEditorUiContract::WelcomeShowAtStartup, FTryllEditorStyle::Txt)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				TryllWelcomePanelPrivate::LinkButton(FTryllEditorUiContract::WelcomeOpenHelp,
					[]() { STryllHelpPanel::Open(); }, TryllWelcomePanelPrivate::TagOpenHelp)
			]
		];
}

// ── Actions ────────────────────────────────────────────────────────────────────

void STryllWelcomePanel::OnLicenceButton(bool bAccept)
{
	// OnChanged refreshes this tab. The editor subsystem stops the server on a
	// withdraw, and starts it for windows waiting on a blocked start on accept.
	FTryllLicence::SetAccepted(bAccept);
}

void STryllWelcomePanel::OnReadLicence()
{
	// The docs page of this plugin's version: the shipped LICENSE.md, rendered.
	FTryllCommunityLinks::OpenUrl(FTryllCommunityLinks::Licence);
}

void STryllWelcomePanel::OnOpenLocalLicence()
{
	// Offline fallback: the shipped file itself, in the OS default app.
	FTryllCommunityLinks::OpenFile(FTryllLicence::LicenceFilePath());
}

void STryllWelcomePanel::OnOpenTelemetry()
{
	// The online telemetry page: what the server sends and why.
	FTryllCommunityLinks::OpenUrl(FTryllCommunityLinks::Telemetry);
}
