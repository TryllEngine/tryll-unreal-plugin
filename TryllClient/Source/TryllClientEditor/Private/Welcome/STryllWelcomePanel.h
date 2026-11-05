// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SDockTab;
class FSpawnTabArgs;
class SVerticalBox;

/**
 * First-run tab: licence acceptance (the licence includes telemetry, which is
 * always on), the bundled server, and community links. Two setup steps:
 * 1 Licence, 2 Server. Opens on its own at startup (FTryllWelcomeStartup), when
 * the licence gate blocks a server start (UTryllEditorSubsystem), and from
 * Window > Tryll > Tryll Welcome. The Unreal port of Unity's TryllWelcomeWindow,
 * without its download step: the server comes inside the plugin zip.
 *
 * Follows docs/dev/editor-window-checklist.md: theme roles only, copy from
 * FTryllEditorUiContract, ASCII status markers, and a debug surface for
 * headless tests. The panel only calls FTryllLicence::SetAccepted; the editor
 * subsystem stops or starts the server on FTryllLicence::OnChanged.
 */
class STryllWelcomePanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STryllWelcomePanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~STryllWelcomePanel() override;

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	/** Nomad tab spawner for FTryllClientEditorModule::WelcomeTabName. */
	static TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args);

	/** What the server card says. */
	enum class EServerCard : uint8
	{
		/** The licence is not accepted yet: the server starts after it is. */
		AfterLicence,
		/** Accepted, and the plugin has its server exe. */
		Bundled,
		/** Accepted, but no server exe in the plugin. */
		NotFound,
	};

	/** The server card for these facts, pure for tests. */
	static EServerCard ServerCardFor(bool bLicenceAccepted, bool bServerExeFound);

	/** "N of 2 done": the licence counts once accepted, the server once Bundled. */
	static int32 DoneSteps(bool bLicenceAccepted, EServerCard Server);

	static constexpr int32 SetupSteps = 2;

	// ── Debug surface for headless panel tests ─────────────────────────────────

	FString DebugSetupProgressForTests() const { return SetupProgressText; }
	FString DebugServerStatusForTests() const { return ServerStatusText; }
	bool DebugAcceptVisibleForTests() const { return bAcceptShown; }
	bool DebugWithdrawVisibleForTests() const { return bWithdrawShown; }
	bool DebugReadLicenceVisibleForTests() const { return bReadLicenceShown; }
	bool DebugLicencePendingVisibleForTests() const { return bLicencePendingShown; }
	bool DebugTelemetryVisibleForTests() const { return bTelemetryShown; }
	bool DebugHasLogoForTests() const { return bLogoShown; }
	int32 DebugCommunityLinkCountForTests() const { return CommunityLinkCount; }
	/** Server exe found (true/false), or unset for the real check. Refreshes. */
	void DebugSetServerFoundForTests(TOptional<bool> bFound);
	/** Re-read the licence file and state, after a test swapped the file. */
	void DebugRefreshForTests() { Refresh(/*bForce=*/true); }
	/** Press a button as a click would. */
	void DebugAcceptForTests() { OnLicenceButton(true); }
	void DebugWithdrawForTests() { OnLicenceButton(false); }
	void DebugReadLicenceForTests() { OnReadLicence(); }
	void DebugOpenTelemetryForTests() { OnOpenTelemetry(); }

private:
	struct FState
	{
		bool bAccepted = false;
		bool bHasLicenceFile = false;
		EServerCard Server = EServerCard::AfterLicence;

		bool operator==(const FState& Other) const
		{
			return bAccepted == Other.bAccepted && bHasLicenceFile == Other.bHasLicenceFile && Server == Other.Server;
		}
	};

	FState ReadState() const;
	bool IsServerExeFound() const;

	/** Re-read the state; rebuild the page when it changed (or always when bForce). */
	void Refresh(bool bForce);
	void RebuildBody();

	TSharedRef<SWidget> BuildBrandHeader();
	TSharedRef<SWidget> BuildHero();
	TSharedRef<SWidget> BuildSetupHeader();
	TSharedRef<SWidget> BuildLicenceStep();
	TSharedRef<SWidget> BuildServerStep();
	TSharedRef<SWidget> BuildCommunity();
	TSharedRef<SWidget> BuildFooter();
	/** "Read the licence" (only with a LICENSE.md) and "Telemetry", as links or buttons. */
	void AddLicenceLinks(const TSharedRef<class SWrapBox>& Target, bool bAsLinks);

	void OnLicenceButton(bool bAccept);
	void OnReadLicence();
	void OnOpenTelemetry();
	void HandleHostColorsRefreshed();

	FState State;
	TOptional<bool> DebugServerFound;
	double NextPollTime = 0.0;

	TSharedPtr<SVerticalBox> BodyBox;
	FDelegateHandle LicenceChangedHandle;
	FDelegateHandle HostColorsHandle;

	// What the last RebuildBody showed, for the debug surface.
	FString SetupProgressText;
	FString ServerStatusText;
	bool bAcceptShown = false;
	bool bWithdrawShown = false;
	bool bReadLicenceShown = false;
	bool bLicencePendingShown = false;
	bool bTelemetryShown = false;
	bool bLogoShown = false;
	int32 CommunityLinkCount = 0;
};
