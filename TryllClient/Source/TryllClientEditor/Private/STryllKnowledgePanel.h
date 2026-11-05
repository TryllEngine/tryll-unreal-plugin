// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Knowledge/TryllKnowledgePresentation.h"
#include "UI/TryllPanelSnapshot.h"
#include "TryllEditorStatus.h"
#include "TryllMutableKb.h"
#include "TryllMutableKbTypes.h"

class UTryllRuntimeSettings;
class UTryllEditorSubsystem;
class SVerticalBox;
class STextBlock;
class SEditableTextBox;
class SMultiLineEditableTextBox;

/**
 * Pull-only inspector for session mutable knowledge bases.
 * Stores · records (filter or query) · write/export.
 */
class STryllKnowledgePanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STryllKnowledgePanel)
		: _EditorSubsystemOverride(nullptr)
		, _PersistLayout(true)
	{}
		/** Test-only: bind this subsystem instead of the live GEditor one. */
		SLATE_ARGUMENT(UTryllEditorSubsystem*, EditorSubsystemOverride)
		/** False: neither read nor write FTryllKnowledgeConfig. */
		SLATE_ARGUMENT(bool, PersistLayout)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~STryllKnowledgePanel() override;

	/** Rows are store names; the banner is the joined stale / warning text. */
	FTryllPanelSnapshot DebugSnapshotForTests() const;
	const FTryllKnowledgePresentation& DebugPresentationForTests() const { return Presentation; }
	bool DebugReadyForTests() const { return IsSessionReady(); }
	bool DebugBusyForTests() const { return bRefreshInFlight; }
	/** Fill the Ensure fields and press Ensure. */
	void DebugEnsureForTests(const FString& Name, const FString& Schema)
	{
		EnsureName = Name;
		EnsureSchema = Schema;
		OnEnsureClicked();
	}
	/** Fill the upsert fields and press Write on the selected store. */
	void DebugWriteForTests(const FString& Id, const FString& Text)
	{
		UpsertId = Id;
		UpsertText = Text;
		OnWriteClicked();
	}
	/** Press "Clear store" as a click would; the confirm goes through FTryllEditorPrompts. */
	void DebugClearForTests() { OnClearClicked(); }
	/** Press Import as a click would; the file dialog goes through FTryllEditorPrompts. */
	void DebugImportForTests() { OnImportClicked(); }

private:
	enum class EState : uint8 { Idle, Connecting, Configuring, Ready, Error };

	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildLeft();
	TSharedRef<SWidget> BuildCenter();
	TSharedRef<SWidget> BuildRight();
	TSharedRef<SWidget> SectionBox(const FString& Title, const TSharedRef<SWidget>& Body);
	TSharedRef<SWidget> Hint(const FString& Text);
	TSharedRef<SWidget> LabeledField(const FString& Label, const TSharedRef<SWidget>& Field);

	void RefreshStatusChrome();
	void RefreshBanner();
	void RebuildStoreList();
	void RebuildRecordList();
	void RefreshStatsLabel();
	void HandleHostColorsRefreshed();
	void HandleOwnerPhaseChanged(ETryllOwnerPhase Phase, const FString& Detail);

	bool CanRefresh() const;
	FReply OnRefreshClicked();
	FReply OnEnsureClicked();
	FReply OnSaveGameClicked();
	FReply OnLoadGameClicked();
	FReply OnRunQueryClicked();
	FReply OnWriteClicked();
	FReply OnDeleteClicked();
	FReply OnClearClicked();
	FReply OnExportClicked();
	FReply OnImportClicked();
	FReply OnFilterModeClicked();
	FReply OnQueryModeClicked();

	void Start();
	void Stop();
	void RetryOwner();
	void AutofindSettings();
	void SetState(EState S, const FString& InStatus);
	void Fail(const FString& Reason);

	void HandleConnectionChanged(bool bConnected);
	void HandleCreateSessionComplete(bool bSuccess);
	void HandleError(const FString& ErrorMessage);

	void SurfaceError(const FTryllError& Error, const TArray<FString>& Extra = {});
	void RequireSelected(TFunction<void(TSharedPtr<FTryllMutableKb>)> Then);
	void LoadSelectedStore(int32 Generation);
	void RefreshStores(int32 Generation);
	void RefreshLists();
	void LoadLayoutConfig();
	void SaveLayoutConfig() const;
	void InitCatalogOptions();
	FString SchemaOf(const FString& Name) const;
	bool IsSessionReady() const;

	TWeakObjectPtr<UTryllRuntimeSettings> Settings;
	UTryllEditorSubsystem* EditorSubsystem = nullptr;
	UTryllEditorSubsystem* EditorSubsystemOverride = nullptr;
	bool bPersistLayout = true;
	bool bAcquiredConnection = false;
	EState State = EState::Idle;
	FString Status = TEXT("Idle");

	FTryllKnowledgePresentation Presentation;
	bool bRefreshInFlight = false;
	bool bListing = false;
	int32 QueryEpoch = 0;
	TMap<FString, TSharedPtr<FTryllMutableKb>> Handles;

	FString EnsureName = TEXT("npc.memory.nessa");
	FString EnsureSchema = TEXT("npc_memory");
	FString QueryText;
	int32 QueryTopK = 4;
	FString UpsertId;
	FString UpsertText;
	FString GamePath = TEXT("memory/game.tmkb");
	FString SaveId = TEXT("slot-1");
	FString StoreExportHint = TEXT("store.tmkb");
	FString LastErrorMessage;
	ETryllMutableKbEmbedTiming EmbedTiming = ETryllMutableKbEmbedTiming::Lazy;
	ETryllMutableKbCompatibility Compatibility = ETryllMutableKbCompatibility::Strict;

	float StoreWidth = 260.0f;
	float InspectorWidth = 320.0f;

	TArray<TSharedPtr<FString>> SchemaOptions;
	TArray<TSharedPtr<FString>> EmbedOptions;
	TArray<TSharedPtr<FString>> CompatOptions;
	TSharedPtr<FString> SelectedSchemaOption;
	TSharedPtr<FString> SelectedEmbedOption;
	TSharedPtr<FString> SelectedCompatOption;

	TSharedPtr<class STryllStatusBar> StatusBar;
	TSharedPtr<class STryllBanner> Banner;
	TSharedPtr<SVerticalBox> StoreList;
	TSharedPtr<SVerticalBox> RecordList;
	TSharedPtr<STextBlock> StatsLabel;
	TSharedPtr<SWidget> FilterRow;
	TSharedPtr<SWidget> QueryRow;
	TSharedPtr<SEditableTextBox> UpsertIdBox;
	TSharedPtr<SMultiLineEditableTextBox> UpsertTextBox;

	FDelegateHandle OwnerPhaseHandle;
	FDelegateHandle HostColorsHandle;
};
