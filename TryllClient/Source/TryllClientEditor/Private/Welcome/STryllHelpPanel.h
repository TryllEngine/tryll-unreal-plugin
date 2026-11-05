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
 * Window > Tryll > Tryll Help & Community: what is installed, where to get help, and the
 * community links. The Unreal port of Unity's TryllHelpWindow. Every value in "About this
 * install" is read live: the client version (the plugin's VersionName), the server version
 * (the connected server's report, else the bundled server's tryll-build.json), the wire
 * protocol, and the newest release of the repo in TryllRelease.json (FTryllUpdateCheck), so
 * the tab needs no edit for a release. The footer opens the Welcome tab and the logs folder.
 *
 * Follows docs/dev/editor-window-checklist.md: theme roles only, copy from
 * FTryllEditorUiContract, and a debug surface for headless tests.
 */
class STryllHelpPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STryllHelpPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~STryllHelpPanel() override;

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	/** Nomad tab spawner for FTryllClientEditorModule::HelpTabName. */
	static TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args);

	/** Opens the tab, or brings it forward. */
	static void Open();

	enum class EUpdate : uint8 { Unavailable, Idle, Checking, Failed, UpToDate, Available };

	/** What the server row says, pure for tests. FileVersion comes from the server folder. */
	static FString ServerText(bool bConnected, const FString& ConnectedVersion, bool bExeFound,
		const FString& FileVersion);

	/** The update row state, pure for tests. */
	static EUpdate UpdateStateFor(bool bRepoKnown, bool bChecking, bool bFailed, const FString& Latest,
		const FString& Current);

	/** The version in a server folder's tryll-build.json or manifest.json, or empty. */
	static FString ServerFileVersion(const FString& ServerDir);

	/** Newest release found this editor session: one network check per session. */
	static FString LatestThisSession;

	// ── Debug surface for headless panel tests ─────────────────────────────────

	const TArray<TPair<FString, FString>>& DebugRowsForTests() const { return Rows; }
	int32 DebugHelpLinkCountForTests() const { return HelpLinkCount; }
	int32 DebugCommunityLinkCountForTests() const { return CommunityLinkCount; }
	EUpdate DebugUpdateStateForTests() const { return CurrentUpdateState(); }
	FString DebugUpdateTextForTests() const { return UpdateText; }
	bool DebugHasLogoForTests() const { return bLogoShown; }
	void DebugSetRepoForTests(const FString& InRepo);
	void DebugCheckForUpdatesForTests() { CheckForUpdates(); }

private:
	struct FServerFacts
	{
		bool bConnected = false;
		FString ConnectedVersion;
		bool bExeFound = false;
		FString FileVersion;
	};

	static FServerFacts ReadServerFacts();
	EUpdate CurrentUpdateState() const;

	void RebuildBody();
	TSharedRef<SWidget> BuildIntro();
	TSharedRef<SWidget> BuildAbout();
	TSharedRef<SWidget> BuildLinks(const FString& Header, const TArray<TPair<FString, FString>>& Links, int32& OutCount);
	TSharedRef<SWidget> BuildFooter();
	TSharedRef<SWidget> AboutRow(const TCHAR* Label, const FString& Value, const FLinearColor& Color,
		const TArray<TSharedRef<SWidget>>& Extras = {});

	void CheckForUpdates();
	void HandleHostColorsRefreshed();

	FString Repo;
	FString Latest;
	bool bChecking = false;
	bool bCheckFailed = false;
	FString ServerRowText;
	double NextPollTime = 0.0;

	TSharedPtr<SVerticalBox> BodyBox;
	FDelegateHandle HostColorsHandle;

	// What the last RebuildBody showed, for the debug surface.
	TArray<TPair<FString, FString>> Rows;
	FString UpdateText;
	int32 HelpLinkCount = 0;
	int32 CommunityLinkCount = 0;
	bool bLogoShown = false;
};
