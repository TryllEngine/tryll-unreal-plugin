// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "TryllEditorStatus.h"
#include "TryllMemorySnapshot.h"
#include "TryllMemoryPresentation.h"
#include "UI/TryllPanelSnapshot.h"

class UTryllRuntimeSettings;
class UTryllEditorSubsystem;
class SVerticalBox;
class STextBlock;

/**
 * Editor-only Memory panel. Pulls RequestGetMemoryConsumption on Refresh and
 * renders the same snapshot the server monitor shows — four tables, no graphs.
 *
 * Session lifetime copies Model Manager: AcquireConnection + CreateSession,
 * ReleaseConnection on close. Does not kill the server.
 */
class STryllMemoryPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STryllMemoryPanel)
		: _EditorSubsystemOverride(nullptr)
	{}
		/** Test-only: bind this subsystem instead of the live GEditor one. */
		SLATE_ARGUMENT(UTryllEditorSubsystem*, EditorSubsystemOverride)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~STryllMemoryPanel() override;

	FTryllPanelSnapshot DebugSnapshotForTests() const;
	/** The tables the panel draws from the last snapshot; empty before the first. */
	FTryllMemoryPresentation::FTables DebugTablesForTests() const;
	bool DebugCanRefreshForTests() const { return CanRefresh(); }
	bool DebugHasSnapshotForTests() const { return Snapshot.IsSet(); }
	/** Press Refresh as a click would (no-op while disabled, like the button). */
	void DebugRefreshForTests() { OnRefreshClicked(); }

private:
	enum class EState : uint8 { Idle, Connecting, Configuring, Ready, Error };

	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildBanner();
	void RebuildBody();

	void        RefreshStatusChrome();
	void        HandleHostColorsRefreshed();
	void        HandleOwnerPhaseChanged(ETryllOwnerPhase Phase, const FString& Detail);
	bool        CanRefresh() const;
	FReply      OnRefreshClicked();

	void SetState(EState S, const FString& InStatus);
	void Fail(const FString& Reason);

	void Start();
	void Stop();
	void RetryOwner();
	void AutofindSettings();

	void HandleConnectionChanged(bool bConnected);
	void HandleCreateSessionComplete(bool bSuccess);
	void HandleError(const FString& ErrorMessage);

	TWeakObjectPtr<UTryllRuntimeSettings> Settings;
	UTryllEditorSubsystem* EditorSubsystem = nullptr;
	UTryllEditorSubsystem* EditorSubsystemOverride = nullptr;
	bool bAcquiredConnection = false;

	EState  State  = EState::Idle;
	FString Status = TEXT("Idle");
	FString LastErrorMessage;

	bool bRefreshInFlight = false;
	int32 FetchGeneration = 0;
	FString StaleError;
	TOptional<FTryllMemorySnapshot> Snapshot;

	TSharedPtr<SVerticalBox> BodyBox;
	TSharedPtr<class STryllStatusBar> StatusBar;
	TSharedPtr<class STryllBanner>    Banner;
	FDelegateHandle OwnerPhaseHandle;
	FDelegateHandle HostColorsHandle;
};
