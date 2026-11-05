// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

/**
 * Exported editor UI contract values. Parity tests compare these with
 * tryll/schema/editor_ui.json. Not a theme or widget layer.
 */
struct TRYLLCLIENTEDITOR_API FTryllEditorUiContract
{
	static constexpr float AgentListMinWidth = 160.0f;
	static constexpr float InspectorMinWidth = 260.0f;
	static constexpr float InspectorDefaultWidth = 400.0f;
	static constexpr float ChatMinWidth = 280.0f;
	static constexpr float TimelineMinWidth = 240.0f;
	static constexpr float StageMinWidth = 200.0f;
	static constexpr float PanelMinWidth = 760.0f;
	static constexpr float PanelMinHeight = 520.0f;
	static constexpr float CellWidthSmall = 212.0f;
	static constexpr float CellWidthMedium = 276.0f;
	static constexpr float CellWidthLarge = 360.0f;
	static constexpr float FollowTailThreshold = 24.0f;
	static constexpr float ModelFilterMinWidth = 250.0f;
	static constexpr float ModelFilterDefaultWidth = 250.0f;
	static constexpr float ModelListMinWidth = 340.0f;
	static constexpr float ModelPropsMinWidth = 280.0f;
	static constexpr float ModelPropsDefaultWidth = 360.0f;
	static constexpr float KnowledgeStoreMinWidth = 220.0f;
	static constexpr float KnowledgeStoreDefaultWidth = 260.0f;
	static constexpr float KnowledgeRecordsMinWidth = 280.0f;
	static constexpr float KnowledgeInspectorMinWidth = 280.0f;
	static constexpr float KnowledgeInspectorDefaultWidth = 320.0f;

	static constexpr int32 DensityLines[3] = {3, 8, 0};

	static constexpr float MemorySystemWeights[] = {2.2f, 1.2f, 2.6f};
	static constexpr float MemoryDevicesWeights[] = {2.6f, 0.7f, 0.9f, 0.9f, 0.9f, 0.9f};
	static constexpr float MemoryKvWeights[] = {2.0f, 1.1f, 0.9f, 2.2f, 0.9f};
	static constexpr float MemoryComponentsWeights[] = {1.4f, 2.2f, 1.2f, 1.4f, 1.0f};

	static constexpr int32 BinaryByteBase = 1024;
	static constexpr int32 DecimalByteBase = 1000;

	static const TCHAR* InspectorEmptyLab;
	static const TCHAR* InspectorEmptyChat;
	static const TCHAR* InspectorNoParameters;
	static const TCHAR* InspectorNoDiffPeer;
	static const TCHAR* ModelManagerSelect;
	static const TCHAR* ModelLicenceNotice;
	static const TCHAR* ModelLicenceMissing;
	static const TCHAR* AgentLogEmpty;
	static const TCHAR* KnowledgeSelectStore;
	static const TCHAR* KnowledgeSelectRecord;
	static const TCHAR* KnowledgeFilterHint;
	static const TCHAR* KnowledgeQueryHint;
	static const TCHAR* KnowledgeTruncated;

	static const TCHAR* StatusCreatingAgent;
	static const TCHAR* StatusCreatingVoiceInput;
	static const TCHAR* StatusRefreshing;
	static const TCHAR* StatusListing;
	static const TCHAR* StatusWaiting;
	static const TCHAR* StatusCapturing;
	static const TCHAR* StatusIdle;
	static const TCHAR* StatusConnecting;
	static const TCHAR* StatusConfiguring;
	static const TCHAR* StatusReady;
	static const TCHAR* StatusReadyModels;
	static const TCHAR* StatusError;
	static const TCHAR* StatusAgentLogIdle;

	// Welcome and Help & Community windows.
	static const TCHAR* WelcomeTitle;
	static const TCHAR* WelcomeIntro;
	static const TCHAR* WelcomeGetStarted;
	static const TCHAR* WelcomeJoinDiscord;
	static const TCHAR* WelcomeSetupHeader;
	static const TCHAR* WelcomeSetupProgress;
	static const TCHAR* WelcomeStepDone;
	static const TCHAR* WelcomeStepRequired;
	static const TCHAR* WelcomeStepUnavailable;
	static const TCHAR* WelcomeLicenceHeader;
	static const TCHAR* WelcomeLicenceBody;
	static const TCHAR* WelcomeLicencePlayers;
	static const TCHAR* WelcomeLicencePending;
	static const TCHAR* WelcomeLicenceAccept;
	static const TCHAR* WelcomeLicenceAccepted;
	static const TCHAR* WelcomeLicenceWithdraw;
	static const TCHAR* WelcomeLicenceRead;
	static const TCHAR* WelcomeTelemetryLink;
	static const TCHAR* WelcomeLicenceViewerTitle;
	static const TCHAR* TextEditorOpen;
	static const TCHAR* TextEditorTitle;
	static const TCHAR* TextEditorGone;
	static const TCHAR* WelcomeServerHeader;
	static const TCHAR* WelcomeServerBundled;
	static const TCHAR* WelcomeServerAfterSetup;
	static const TCHAR* WelcomeServerNotFound;
	static const TCHAR* WelcomeServerInstalled;
	static const TCHAR* WelcomeServerMissing;
	static const TCHAR* WelcomeServerNoPin;
	static const TCHAR* WelcomeDownloadLite;
	static const TCHAR* WelcomeDownloadFull;
	static const TCHAR* WelcomeDownloadFullHint;
	static const TCHAR* WelcomeInstallZip;
	static const TCHAR* WelcomeCancel;
	static const TCHAR* WelcomeDownloading;
	static const TCHAR* WelcomeInstalling;
	static const TCHAR* WelcomeNeedsLicence;
	static const TCHAR* ServerNeedsLicence;
	static const TCHAR* WelcomeInstallFailed;
	static const TCHAR* WelcomeInstallDone;
	static const TCHAR* WelcomeCommunityHeader;
	static const TCHAR* WelcomeShowAtStartup;
	static const TCHAR* WelcomeOpenHelp;
	static const TCHAR* HelpTitle;
	static const TCHAR* HelpIntro;
	static const TCHAR* HelpAboutHeader;
	static const TCHAR* HelpRowClient;
	static const TCHAR* HelpRowServer;
	static const TCHAR* HelpRowProtocol;
	static const TCHAR* HelpRowUpdates;
	static const TCHAR* HelpServerBundled;
	static const TCHAR* HelpServerMissing;
	static const TCHAR* HelpInstallServer;
	static const TCHAR* HelpProtocolMismatch;
	static const TCHAR* HelpGetHelpHeader;
	static const TCHAR* HelpReportBug;
	static const TCHAR* HelpOpenWelcome;
	static const TCHAR* HelpCheckUpdates;
	static const TCHAR* HelpChecking;
	static const TCHAR* HelpUpToDate;
	static const TCHAR* HelpUpdateAvailable;
	static const TCHAR* HelpViewRelease;
	static const TCHAR* HelpUpdateFailed;
	static const TCHAR* HelpOpenLogs;
	static const TCHAR* HelpLogsNotCreated;
	static const TCHAR* HelpUpdatesUnavailable;
	static const TCHAR* LinkDocs;
	static const TCHAR* LinkDiscord;
	static const TCHAR* LinkWebsite;
	static const TCHAR* LinkGithub;
	static const TCHAR* LinkReportIssue;
	static const TCHAR* LinkYoutube;
	static const TCHAR* LinkX;
	static const TCHAR* LinkLinkedin;
	static const TCHAR* LinkInstagram;
	static const TCHAR* LinkTiktok;
	static const TCHAR* LinkEmail;

	static FString FormatReadyModels(int32 Count);
	static FString FormatKnowledgeTruncated(int32 Count);

	static constexpr const TCHAR* Roles[] = {
		TEXT("ok"), TEXT("warn"), TEXT("error"), TEXT("accent"),
		TEXT("diff_added"), TEXT("diff_removed"), TEXT("loaded"),
	};
	static constexpr const TCHAR* Components[] = {
		TEXT("bubble"), TEXT("chip"), TEXT("heading"), TEXT("section"),
		TEXT("placeholder"), TEXT("banner"), TEXT("timing_segment"), TEXT("code_block"),
	};
	static constexpr const TCHAR* IconIds[] = {
		TEXT("warning"), TEXT("download"), TEXT("capture_gap"),
		TEXT("voice_input"), TEXT("expand"), TEXT("collapse"),
	};

	// Unity tofu glyphs are stored as codepoints so this module never puts
	// uncovered Slate characters in a string literal.
	static constexpr uint32 IconWarningUnity = 0x26A0;
	static constexpr uint32 IconDownloadUnity = 0x2913;
	static constexpr uint32 IconVoiceInputUnity = 0x1F3A4;

	static const TCHAR* IconCaptureGap;
	static const TCHAR* IconExpand;
	static const TCHAR* IconCollapse;

	static FString UnityIconFromCodepoint(uint32 Codepoint);

	static const TCHAR* IconWarningUnreal;
	static const TCHAR* IconDownloadUnreal;
	static const TCHAR* IconCaptureGapUnreal;
	static const TCHAR* IconVoiceInputUnreal;
	static const TCHAR* IconExpandUnreal;
	static const TCHAR* IconCollapseUnreal;
	static constexpr const TCHAR* Nodes[] = {
		TEXT("RetrieveNode"), TEXT("TransformNode"), TEXT("GenerateNode"),
		TEXT("CannedResponseNode"), TEXT("BranchNode"), TEXT("GenerateAndSpeakNode"),
		TEXT("ToolCallNode"), TEXT("RegexGuardrailNode"), TEXT("ClassifyIntentNode"),
		TEXT("ClassifyIntentLLMNode"), TEXT("ClassifyAttributesLLMNode"), TEXT("IntentToInstructionNode"),
		TEXT("InstructionNode"), TEXT("SpeakNode"), TEXT("PauseNode"),
	};
};
