// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "Welcome/STryllHelpPanel.h"

#include "Welcome/TryllCommunityLinks.h"
#include "Welcome/TryllCommunityWidgets.h"
#include "Welcome/TryllUpdateCheck.h"
#include "TryllClientEditorModule.h"
#include "TryllEditorSubsystem.h"
#include "TryllEditorUiContract.h"
#include "TryllLogsFolder.h"
#include "TryllRuntimeSettings.h"
#include "TryllSubsystem.h"
#include "UI/TryllEditorStyle.h"
#include "tryll/Protocol.h"

#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Framework/Docking/TabManager.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

FString STryllHelpPanel::LatestThisSession;

// Named, not anonymous: UBT unity builds merge this file with others in the module.
namespace TryllHelpPanelPrivate
{
const TCHAR* const TagCheckUpdates = TEXT("Tryll.Help.CheckUpdates");
const TCHAR* const TagViewRelease = TEXT("Tryll.Help.ViewRelease");
const TCHAR* const TagOpenWelcome = TEXT("Tryll.Help.OpenWelcome");
const TCHAR* const TagOpenLogs = TEXT("Tryll.Help.OpenLogs");
const TCHAR* const TagInstallServer = TEXT("Tryll.Help.InstallServer");
const TCHAR* const TagReportBug = TEXT("Tryll.Help.ReportBug");

constexpr float RowLabelWidth = 100.0f;
constexpr double ServerPollSeconds = 2.0;

bool IsNonInteractive()
{
	return IsRunningCommandlet() || FApp::IsUnattended() || GIsAutomationTesting;
}

void OpenWelcome()
{
	FGlobalTabmanager::Get()->TryInvokeTab(FTryllClientEditorModule::WelcomeTabName);
}
} // namespace TryllHelpPanelPrivate

// ── Construction ───────────────────────────────────────────────────────────────

TSharedRef<SDockTab> STryllHelpPanel::SpawnTab(const FSpawnTabArgs& /*Args*/)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(STryllHelpPanel)
		];
}

void STryllHelpPanel::Open()
{
	FGlobalTabmanager::Get()->TryInvokeTab(FTryllClientEditorModule::HelpTabName);
}

void STryllHelpPanel::Construct(const FArguments& /*InArgs*/)
{
	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot().Padding(FMargin(16.0f, 14.0f, 16.0f, 12.0f))
		[
			SAssignNew(BodyBox, SVerticalBox)
		]
	];

	Repo = FTryllUpdateCheck::ReleasesRepo();
	Latest = LatestThisSession;
	HostColorsHandle = FTryllEditorStyle::OnHostColorsRefreshed.AddSP(this, &STryllHelpPanel::HandleHostColorsRefreshed);
	RebuildBody();
	if (Latest.IsEmpty() && !Repo.IsEmpty() && !TryllHelpPanelPrivate::IsNonInteractive())
	{
		CheckForUpdates();
	}
}

STryllHelpPanel::~STryllHelpPanel()
{
	if (HostColorsHandle.IsValid())
	{
		FTryllEditorStyle::OnHostColorsRefreshed.Remove(HostColorsHandle);
		HostColorsHandle.Reset();
	}
}

void STryllHelpPanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	// A server can start or stop while the tab is open. Rebuild only when its row changed.
	if (InCurrentTime >= NextPollTime)
	{
		NextPollTime = InCurrentTime + TryllHelpPanelPrivate::ServerPollSeconds;
		const FServerFacts Facts = ReadServerFacts();
		if (ServerText(Facts.bConnected, Facts.ConnectedVersion, Facts.bExeFound, Facts.FileVersion) != ServerRowText)
		{
			RebuildBody();
		}
	}
}

void STryllHelpPanel::HandleHostColorsRefreshed()
{
	RebuildBody();
}

// ── State ──────────────────────────────────────────────────────────────────────

FString STryllHelpPanel::ServerText(bool bConnected, const FString& ConnectedVersion, bool bExeFound,
	const FString& FileVersion)
{
	if (bConnected && !ConnectedVersion.IsEmpty())
	{
		return ConnectedVersion;
	}
	if (!bExeFound)
	{
		return FTryllEditorUiContract::HelpServerMissing;
	}
	return FileVersion.IsEmpty() ? FString(FTryllEditorUiContract::HelpServerBundled) : FileVersion;
}

STryllHelpPanel::EUpdate STryllHelpPanel::UpdateStateFor(bool bRepoKnown, bool bInChecking, bool bFailed,
	const FString& InLatest, const FString& Current)
{
	if (!bRepoKnown)
	{
		return EUpdate::Unavailable;
	}
	if (bInChecking)
	{
		return EUpdate::Checking;
	}
	if (bFailed)
	{
		return EUpdate::Failed;
	}
	if (InLatest.IsEmpty())
	{
		return EUpdate::Idle;
	}
	return FTryllUpdateCheck::IsNewer(InLatest, Current) ? EUpdate::Available : EUpdate::UpToDate;
}

FString STryllHelpPanel::ServerFileVersion(const FString& ServerDir)
{
	for (const TCHAR* Name : { TEXT("manifest.json"), TEXT("tryll-build.json") })
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *FPaths::Combine(ServerDir, Name)))
		{
			continue;
		}
		TSharedPtr<FJsonObject> Root;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
		FString Version;
		if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid()
			&& Root->TryGetStringField(TEXT("version"), Version) && !Version.IsEmpty())
		{
			return Version;
		}
	}
	return FString();
}

STryllHelpPanel::FServerFacts STryllHelpPanel::ReadServerFacts()
{
	FServerFacts Facts;
	if (GEditor != nullptr)
	{
		if (const UTryllEditorSubsystem* Sub = GEditor->GetEditorSubsystem<UTryllEditorSubsystem>())
		{
			Facts.bConnected = Sub->IsConnected();
			Facts.ConnectedVersion = Sub->ServerVersion;
		}
	}
	const UTryllRuntimeSettings* Settings = GetDefault<UTryllRuntimeSettings>();
	const FString Exe = UTryllSubsystem::ResolveServerExePath(
		Settings != nullptr ? Settings->ServerBuildVariant : EServerBuildVariant::Default);
	Facts.bExeFound = !Exe.IsEmpty() && FPaths::FileExists(Exe);
	if (Facts.bExeFound)
	{
		Facts.FileVersion = ServerFileVersion(FPaths::GetPath(Exe));
	}
	return Facts;
}

STryllHelpPanel::EUpdate STryllHelpPanel::CurrentUpdateState() const
{
	return UpdateStateFor(!Repo.IsEmpty(), bChecking, bCheckFailed, Latest, TryllCommunityWidgets::ClientVersion());
}

void STryllHelpPanel::DebugSetRepoForTests(const FString& InRepo)
{
	Repo = InRepo;
	Latest.Reset();
	bCheckFailed = false;
	RebuildBody();
}

// ── Layout ─────────────────────────────────────────────────────────────────────

void STryllHelpPanel::RebuildBody()
{
	if (!BodyBox.IsValid())
	{
		return;
	}
	BodyBox->ClearChildren();
	Rows.Reset();

	auto Add = [this](const TSharedRef<SWidget>& Widget, float Bottom)
	{
		BodyBox->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, Bottom)[ Widget ];
	};
	Add(TryllCommunityWidgets::BrandHeader(bLogoShown), 14.0f);
	Add(BuildIntro(), 12.0f);
	Add(BuildAbout(), 10.0f);
	Add(BuildLinks(FTryllEditorUiContract::HelpGetHelpHeader, FTryllCommunityLinks::HelpLinks(), HelpLinkCount), 10.0f);
	Add(BuildLinks(FTryllEditorUiContract::WelcomeCommunityHeader, FTryllCommunityLinks::CommunityLinks(),
		CommunityLinkCount), 10.0f);
	Add(BuildFooter(), 0.0f);
}

TSharedRef<SWidget> STryllHelpPanel::BuildIntro()
{
	using namespace TryllCommunityWidgets;
	TSharedRef<SVerticalBox> Intro = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FTryllEditorUiContract::HelpTitle))
			.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Txt))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			Paragraph(FTryllEditorUiContract::HelpIntro, FTryllEditorStyle::Dim)
		];
	TSharedRef<SWrapBox> Hero = Row();
	if (*FTryllCommunityLinks::Discord != TEXT('\0'))
	{
		Hero->AddSlot()
		[
			PrimaryButton(FTryllEditorUiContract::WelcomeJoinDiscord,
				[]() { FTryllCommunityLinks::OpenUrl(FTryllCommunityLinks::Discord); })
		];
	}
	Hero->AddSlot()
	[
		SecondaryButton(FTryllEditorUiContract::HelpReportBug,
			[]() { FTryllCommunityLinks::OpenUrl(FTryllCommunityLinks::ReportBug); },
			TryllHelpPanelPrivate::TagReportBug, FTryllCommunityLinks::ReportBug)
	];
	Intro->AddSlot().AutoHeight()[ Hero ];
	return Intro;
}

TSharedRef<SWidget> STryllHelpPanel::AboutRow(const TCHAR* Label, const FString& Value, const FLinearColor& Color,
	const TArray<TSharedRef<SWidget>>& Extras)
{
	Rows.Emplace(Label, Value);
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(TryllHelpPanelPrivate::RowLabelWidth)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 8.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Value))
			.ColorAndOpacity(FSlateColor(Color))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
		];
	for (const TSharedRef<SWidget>& Extra : Extras)
	{
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center)[ Extra ];
	}
	return SNew(SBox).MinDesiredHeight(24.0f)[ Row ];
}

TSharedRef<SWidget> STryllHelpPanel::BuildAbout()
{
	using namespace TryllCommunityWidgets;
	using namespace TryllHelpPanelPrivate;
	TWeakPtr<STryllHelpPanel> Weak = SharedThis(this);
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	auto AddRow = [&Box](const TSharedRef<SWidget>& Row) { Box->AddSlot().AutoHeight()[ Row ]; };

	// Technical values only (versions); every sentence comes from FTryllEditorUiContract.
	const FString Client = ClientVersion();
	AddRow(AboutRow(FTryllEditorUiContract::HelpRowClient, Client, FTryllEditorStyle::Txt));

	const FServerFacts Facts = ReadServerFacts();
	ServerRowText = ServerText(Facts.bConnected, Facts.ConnectedVersion, Facts.bExeFound, Facts.FileVersion);
	if (!Facts.bExeFound && !(Facts.bConnected && !Facts.ConnectedVersion.IsEmpty()))
	{
		// A source install without the server zip: the Welcome tab says where it goes.
		AddRow(AboutRow(FTryllEditorUiContract::HelpRowServer, ServerRowText, FTryllEditorStyle::ColWarn,
			{ LinkButton(FTryllEditorUiContract::HelpInstallServer, []() { OpenWelcome(); }, TagInstallServer) }));
	}
	else
	{
		AddRow(AboutRow(FTryllEditorUiContract::HelpRowServer, ServerRowText, FTryllEditorStyle::Txt));
	}

	AddRow(AboutRow(FTryllEditorUiContract::HelpRowProtocol, FString::FromInt(Tryll::ProtocolVersion),
		FTryllEditorStyle::Txt));

	const EUpdate State = CurrentUpdateState();
	FLinearColor Color = FTryllEditorStyle::Dim;
	switch (State)
	{
	case EUpdate::Unavailable: UpdateText = FTryllEditorUiContract::HelpUpdatesUnavailable; break;
	case EUpdate::Checking:    UpdateText = FTryllEditorUiContract::HelpChecking; break;
	case EUpdate::Failed:      UpdateText = FTryllEditorUiContract::HelpUpdateFailed; Color = FTryllEditorStyle::ColWarn; break;
	case EUpdate::UpToDate:    UpdateText = FTryllEditorUiContract::HelpUpToDate; Color = FTryllEditorStyle::ColOk; break;
	case EUpdate::Available:
		UpdateText = FString(FTryllEditorUiContract::HelpUpdateAvailable).Replace(TEXT("{version}"), *Latest);
		Color = FTryllEditorStyle::Accent;
		break;
	default: UpdateText.Reset(); break;
	}
	TArray<TSharedRef<SWidget>> UpdateActions;
	if (State == EUpdate::Available)
	{
		const FString ReleaseUrl = Repo + TEXT("/releases/latest");
		UpdateActions.Add(LinkButton(FTryllEditorUiContract::HelpViewRelease,
			[ReleaseUrl]() { FTryllCommunityLinks::OpenUrl(ReleaseUrl); }, TagViewRelease, ReleaseUrl));
	}
	if (State != EUpdate::Unavailable && State != EUpdate::Checking)
	{
		UpdateActions.Add(LinkButton(FTryllEditorUiContract::HelpCheckUpdates,
			[Weak]() { if (TSharedPtr<STryllHelpPanel> Self = Weak.Pin()) { Self->CheckForUpdates(); } },
			TagCheckUpdates));
	}
	AddRow(AboutRow(FTryllEditorUiContract::HelpRowUpdates, UpdateText, Color, UpdateActions));

	return Card(FTryllEditorUiContract::HelpAboutHeader, Box);
}

TSharedRef<SWidget> STryllHelpPanel::BuildLinks(const FString& Header, const TArray<TPair<FString, FString>>& Links,
	int32& OutCount)
{
	using namespace TryllCommunityWidgets;
	TSharedRef<SWrapBox> Chips = Row();
	OutCount = Links.Num();
	for (const TPair<FString, FString>& Link : Links)
	{
		Chips->AddSlot()[ LinkChip(Link.Key, Link.Value) ];
	}
	return Card(Header, Chips);
}

TSharedRef<SWidget> STryllHelpPanel::BuildFooter()
{
	using namespace TryllCommunityWidgets;
	using namespace TryllHelpPanelPrivate;
	TSharedRef<SWrapBox> Footer = Row();
	Footer->AddSlot()
	[
		SecondaryButton(FTryllEditorUiContract::HelpOpenWelcome, []() { OpenWelcome(); }, TagOpenWelcome)
	];
	Footer->AddSlot()
	[
		SecondaryButton(FTryllEditorUiContract::HelpOpenLogs, []() { TryllLogsFolder::Open(); }, TagOpenLogs)
	];
	return Footer;
}

// ── Updates ────────────────────────────────────────────────────────────────────

void STryllHelpPanel::CheckForUpdates()
{
	if (bChecking || Repo.IsEmpty())
	{
		return;
	}
	bChecking = true;
	bCheckFailed = false;
	RebuildBody();
	TWeakPtr<STryllHelpPanel> Weak = SharedThis(this);
	FTryllUpdateCheck::LatestVersionAsync(Repo, [Weak](const FString& Found)
	{
		TSharedPtr<STryllHelpPanel> Self = Weak.Pin();
		if (!Self.IsValid())
		{
			return;   // tab closed while waiting
		}
		Self->bChecking = false;
		Self->bCheckFailed = Found.IsEmpty();
		if (!Found.IsEmpty())
		{
			Self->Latest = Found;
			LatestThisSession = Found;
		}
		Self->RebuildBody();
	});
}
