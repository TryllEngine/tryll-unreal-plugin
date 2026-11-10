// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllEditorUiContract.h"

const TCHAR* FTryllEditorUiContract::InspectorEmptyLab =
	TEXT("Select a cell to inspect its turn.\n\n")
	TEXT("Pin two columns (⇧-click a cell, or ⊙ in a header) to diff their rendered prompts.");
const TCHAR* FTryllEditorUiContract::InspectorEmptyChat =
	TEXT("Select an assistant answer to inspect its turn.");
const TCHAR* FTryllEditorUiContract::InspectorNoParameters =
	TEXT("No parameters on this turn's nodes.");
const TCHAR* FTryllEditorUiContract::InspectorNoDiffPeer =
	TEXT("No diff — peer carries no diagnostics for this turn.");
const TCHAR* FTryllEditorUiContract::ModelManagerSelect =
	TEXT("Select a model to view its details and controls.");
const TCHAR* FTryllEditorUiContract::ModelLicenceNotice =
	TEXT("Check the model's licence before you ship it with your game.");
const TCHAR* FTryllEditorUiContract::ModelLicenceMissing =
	TEXT("No licence_url in the catalog for this model. Check its licence at the source before you ship it.");
const TCHAR* FTryllEditorUiContract::AgentLogEmpty =
	TEXT("No observations captured yet.");
const TCHAR* FTryllEditorUiContract::KnowledgeSelectStore =
	TEXT("Select a store to inspect its records.");
const TCHAR* FTryllEditorUiContract::KnowledgeSelectRecord =
	TEXT("Select a record to edit it, or write a new one below.");
const TCHAR* FTryllEditorUiContract::KnowledgeFilterHint =
	TEXT("Substring match on this store's loaded records. Does not call the server.");
const TCHAR* FTryllEditorUiContract::KnowledgeQueryHint =
	TEXT("Dense retrieval against the selected store. Results replace the list until you switch back to Filter.");
const TCHAR* FTryllEditorUiContract::KnowledgeTruncated =
	TEXT("This store has more records than the window can show. Showing the first {count}.");

const TCHAR* FTryllEditorUiContract::StatusCreatingAgent = TEXT("Creating agent…");
const TCHAR* FTryllEditorUiContract::StatusCreatingVoiceInput = TEXT("Creating VoiceInput…");
const TCHAR* FTryllEditorUiContract::StatusRefreshing = TEXT("Refreshing…");
const TCHAR* FTryllEditorUiContract::StatusListing = TEXT("Listing…");
const TCHAR* FTryllEditorUiContract::StatusWaiting = TEXT("Waiting…");
const TCHAR* FTryllEditorUiContract::StatusCapturing = TEXT("Capturing…");
const TCHAR* FTryllEditorUiContract::StatusIdle = TEXT("Idle");
const TCHAR* FTryllEditorUiContract::StatusConnecting = TEXT("Connecting…");
const TCHAR* FTryllEditorUiContract::StatusConfiguring = TEXT("Configuring…");
const TCHAR* FTryllEditorUiContract::StatusReady = TEXT("Ready");
const TCHAR* FTryllEditorUiContract::StatusReadyModels = TEXT("Ready · {count} models");
const TCHAR* FTryllEditorUiContract::StatusError = TEXT("Error");
const TCHAR* FTryllEditorUiContract::StatusAgentLogIdle =
	TEXT("Waiting for a shared Tryll connection");

// Welcome and Help & Community windows.
const TCHAR* FTryllEditorUiContract::WelcomeTitle = TEXT("Welcome to Tryll");
const TCHAR* FTryllEditorUiContract::WelcomeIntro = TEXT("Run language, speech and voice models on the player's own machine. Finish the setup below once, then open the Tryll Chat window from Window > Tryll to talk to your first agent.");
const TCHAR* FTryllEditorUiContract::WelcomeGetStarted = TEXT("Get started");
const TCHAR* FTryllEditorUiContract::WelcomeJoinDiscord = TEXT("Join our Discord");
const TCHAR* FTryllEditorUiContract::WelcomeSetupHeader = TEXT("Setup");
const TCHAR* FTryllEditorUiContract::WelcomeSetupProgress = TEXT("{done} of {total} done");
const TCHAR* FTryllEditorUiContract::WelcomeStepDone = TEXT("Done");
const TCHAR* FTryllEditorUiContract::WelcomeStepRequired = TEXT("Required");
const TCHAR* FTryllEditorUiContract::WelcomeStepUnavailable = TEXT("Not available");
const TCHAR* FTryllEditorUiContract::WelcomeLicenceHeader = TEXT("Licence");
const TCHAR* FTryllEditorUiContract::WelcomeLicenceBody = TEXT("The Tryll server is covered by the Tryll Runtime Licence. It includes telemetry, which is always on in the editor. Accept the licence to run the server.");
const TCHAR* FTryllEditorUiContract::WelcomeLicencePlayers = TEXT("Your players are never measured. Shipped games contain no telemetry.");
const TCHAR* FTryllEditorUiContract::WelcomeLicencePending = TEXT("This plugin has no LICENSE.md yet. Until the licence is published, the server is for evaluation only.");
const TCHAR* FTryllEditorUiContract::WelcomeLicenceAccept = TEXT("Accept licence");
const TCHAR* FTryllEditorUiContract::WelcomeLicenceAccepted = TEXT("You accepted the licence.");
const TCHAR* FTryllEditorUiContract::WelcomeLicenceWithdraw = TEXT("Withdraw");
const TCHAR* FTryllEditorUiContract::WelcomeLicenceRead = TEXT("Read the licence");
const TCHAR* FTryllEditorUiContract::WelcomeTelemetryLink = TEXT("Telemetry");
const TCHAR* FTryllEditorUiContract::WelcomeLicenceLocalCopy = TEXT("Open local copy");
const TCHAR* FTryllEditorUiContract::TextEditorOpen = TEXT("Edit in a window");
const TCHAR* FTryllEditorUiContract::TextEditorTitle = TEXT("Edit {field}");
const TCHAR* FTryllEditorUiContract::TextEditorGone = TEXT("This field is gone: its object was deleted or reloaded. Open it again from the inspector.");
const TCHAR* FTryllEditorUiContract::WelcomeServerHeader = TEXT("Server");
const TCHAR* FTryllEditorUiContract::WelcomeServerBundled = TEXT("This plugin bundles its server. Nothing to download.");
const TCHAR* FTryllEditorUiContract::WelcomeServerAfterSetup = TEXT("The server starts after you accept the licence.");
const TCHAR* FTryllEditorUiContract::WelcomeServerNotFound = TEXT("No server found in this plugin. Add the Tryll server zip as the README describes.");
const TCHAR* FTryllEditorUiContract::WelcomeServerInstalled = TEXT("Installed: Tryll server {version} ({variant}).");
const TCHAR* FTryllEditorUiContract::WelcomeServerMissing = TEXT("Not installed. Download it once; every project on this computer shares it.");
const TCHAR* FTryllEditorUiContract::WelcomeServerNoPin = TEXT("This plugin does not name a server build. Use a plugin from a Tryll release.");
const TCHAR* FTryllEditorUiContract::WelcomeDownloadLite = TEXT("Download server ({size})");
const TCHAR* FTryllEditorUiContract::WelcomeDownloadFull = TEXT("Download CUDA build ({size})");
const TCHAR* FTryllEditorUiContract::WelcomeDownloadFullHint = TEXT("For NVIDIA GPUs. Includes the CPU and Vulkan builds.");
const TCHAR* FTryllEditorUiContract::WelcomeInstallZip = TEXT("Install from zip…");
const TCHAR* FTryllEditorUiContract::WelcomeCancel = TEXT("Cancel");
const TCHAR* FTryllEditorUiContract::WelcomeDownloading = TEXT("Downloading… {percent}%");
const TCHAR* FTryllEditorUiContract::WelcomeInstalling = TEXT("Installing…");
const TCHAR* FTryllEditorUiContract::WelcomeNeedsLicence = TEXT("Accept the licence first.");
const TCHAR* FTryllEditorUiContract::ServerNeedsLicence = TEXT("Accept the Tryll Runtime Licence in the Tryll Welcome window first.");
const TCHAR* FTryllEditorUiContract::WelcomeInstallFailed = TEXT("Install failed: {reason}");
const TCHAR* FTryllEditorUiContract::WelcomeInstallDone = TEXT("Server installed.");
const TCHAR* FTryllEditorUiContract::WelcomeCommunityHeader = TEXT("Community");
const TCHAR* FTryllEditorUiContract::WelcomeShowAtStartup = TEXT("Show this window at startup");
const TCHAR* FTryllEditorUiContract::WelcomeOpenHelp = TEXT("Help & Community");
const TCHAR* FTryllEditorUiContract::HelpTitle = TEXT("Help & Community");
const TCHAR* FTryllEditorUiContract::HelpIntro = TEXT("Questions, bugs or ideas? Ask on Discord, read the docs, or email the team.");
const TCHAR* FTryllEditorUiContract::HelpAboutHeader = TEXT("About this install");
const TCHAR* FTryllEditorUiContract::HelpRowClient = TEXT("Tryll client");
const TCHAR* FTryllEditorUiContract::HelpRowServer = TEXT("Tryll server");
const TCHAR* FTryllEditorUiContract::HelpRowProtocol = TEXT("Protocol");
const TCHAR* FTryllEditorUiContract::HelpRowUpdates = TEXT("Updates");
const TCHAR* FTryllEditorUiContract::HelpServerBundled = TEXT("Bundled with the plugin");
const TCHAR* FTryllEditorUiContract::HelpServerMissing = TEXT("Not installed");
const TCHAR* FTryllEditorUiContract::HelpInstallServer = TEXT("Install");
const TCHAR* FTryllEditorUiContract::HelpProtocolMismatch = TEXT("Server uses protocol {protocol}");
const TCHAR* FTryllEditorUiContract::HelpGetHelpHeader = TEXT("Get help");
const TCHAR* FTryllEditorUiContract::HelpReportBug = TEXT("Report a bug");
const TCHAR* FTryllEditorUiContract::HelpOpenWelcome = TEXT("Open the Welcome window");
const TCHAR* FTryllEditorUiContract::HelpCheckUpdates = TEXT("Check for updates");
const TCHAR* FTryllEditorUiContract::HelpChecking = TEXT("Checking…");
const TCHAR* FTryllEditorUiContract::HelpUpToDate = TEXT("You have the latest version.");
const TCHAR* FTryllEditorUiContract::HelpUpdateAvailable = TEXT("Tryll {version} is available.");
const TCHAR* FTryllEditorUiContract::HelpViewRelease = TEXT("View release");
const TCHAR* FTryllEditorUiContract::HelpUpdateFailed = TEXT("Could not check for updates.");
const TCHAR* FTryllEditorUiContract::HelpOpenLogs = TEXT("Open logs folder");
const TCHAR* FTryllEditorUiContract::HelpLogsNotCreated = TEXT("This project has no Tryll logs yet. Opened {folder} instead.");
const TCHAR* FTryllEditorUiContract::HelpUpdatesUnavailable = TEXT("Not available for this build.");
const TCHAR* FTryllEditorUiContract::LinkDocs = TEXT("Documentation");
const TCHAR* FTryllEditorUiContract::LinkDiscord = TEXT("Discord");
const TCHAR* FTryllEditorUiContract::LinkWebsite = TEXT("Website");
const TCHAR* FTryllEditorUiContract::LinkGithub = TEXT("GitHub");
const TCHAR* FTryllEditorUiContract::LinkReportIssue = TEXT("Report an issue");
const TCHAR* FTryllEditorUiContract::LinkYoutube = TEXT("YouTube");
const TCHAR* FTryllEditorUiContract::LinkX = TEXT("X");
const TCHAR* FTryllEditorUiContract::LinkLinkedin = TEXT("LinkedIn");
const TCHAR* FTryllEditorUiContract::LinkInstagram = TEXT("Instagram");
const TCHAR* FTryllEditorUiContract::LinkTiktok = TEXT("TikTok");
const TCHAR* FTryllEditorUiContract::LinkEmail = TEXT("Email us");

const TCHAR* FTryllEditorUiContract::IconCaptureGap = TEXT("·");
const TCHAR* FTryllEditorUiContract::IconExpand = TEXT("▶");
const TCHAR* FTryllEditorUiContract::IconCollapse = TEXT("▼");

FString FTryllEditorUiContract::UnityIconFromCodepoint(const uint32 Codepoint)
{
	FString Out;
	if (Codepoint <= 0xFFFF)
	{
		Out.AppendChar(static_cast<TCHAR>(Codepoint));
	}
	else
	{
		const uint32 U = Codepoint - 0x10000;
		Out.AppendChar(static_cast<TCHAR>(0xD800 + (U >> 10)));
		Out.AppendChar(static_cast<TCHAR>(0xDC00 + (U & 0x3FF)));
	}
	return Out;
}

const TCHAR* FTryllEditorUiContract::IconWarningUnreal = TEXT("Icons.Warning");
const TCHAR* FTryllEditorUiContract::IconDownloadUnreal = TEXT("Icons.Download");
const TCHAR* FTryllEditorUiContract::IconCaptureGapUnreal = TEXT("·");
const TCHAR* FTryllEditorUiContract::IconVoiceInputUnreal = TEXT("ClassIcon.AudioComponent");
const TCHAR* FTryllEditorUiContract::IconExpandUnreal = TEXT("▶");
const TCHAR* FTryllEditorUiContract::IconCollapseUnreal = TEXT("▼");

FString FTryllEditorUiContract::FormatReadyModels(const int32 Count)
{
	return FString(StatusReadyModels).Replace(TEXT("{count}"), *FString::FromInt(Count));
}

FString FTryllEditorUiContract::FormatKnowledgeTruncated(const int32 Count)
{
	return FString(KnowledgeTruncated).Replace(TEXT("{count}"), *FString::FromInt(Count));
}
