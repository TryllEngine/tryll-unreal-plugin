// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "TryllEditorStatus.h"
#include "TryllVoiceInput.h"
#include "TryllChatTurnState.h"
#include "TryllChatVoiceSource.h"
#include "UI/TryllTailFollow.h"
#include "UI/TryllPanelSnapshot.h"

class UTryllRuntimeSettings;
class UTryllAgentComponent;
class UTryllVoiceInputComponent;
class UTryllEditorSubsystem;
class FTryllAgent;
class FTryllVoiceInput;
class SEditableTextBox;
class SScrollBox;
class SButton;
class SComboButton;
struct FTryllChatMicCapture;
class FTryllModelDownloadGuard;
class STryllTurnInspectorView;
class AActor;

/**
 * Editor-only chat panel mirroring the Unity TryllChatWindow.
 *
 * Selects a scene UTryllAgentComponent, then creates an editor-owned agent from
 * that component's resolved graph / declarations / overrides via
 * UTryllEditorSubsystem. A transient weak preview link on the component lets
 * Details live-apply Variable Overrides without PIE. Chat remains the sole
 * owner of the TSharedPtr agent and connection acquisition.
 */
class STryllChatPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STryllChatPanel)
		: _EditorSubsystemOverride(nullptr)
		, _PersistLayout(true)
		, _AgentComponentOverride(nullptr)
	{}
		/** Test-only: bind this subsystem instead of the live GEditor one. */
		SLATE_ARGUMENT(UTryllEditorSubsystem*, EditorSubsystemOverride)
		/** False: neither read nor write UTryllChatConfig; voice starts Off. */
		SLATE_ARGUMENT(bool, PersistLayout)
		/** Test-only: select this component instead of the editor selection. */
		SLATE_ARGUMENT(UTryllAgentComponent*, AgentComponentOverride)
	SLATE_END_ARGS()

	STryllChatPanel();
	void Construct(const FArguments& InArgs);
	virtual ~STryllChatPanel() override;

	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;

	enum class EState : uint8
	{
		Idle,
		Connecting,
		Configuring,
		CheckingModels,
		CreatingAgent,
		CreatingVoiceInput,
		Ready,
		Error,
	};

	static bool IsStartable(EState InState)
	{
		return InState == EState::Idle || InState == EState::Error;
	}

	FTryllPanelSnapshot DebugSnapshotForTests() const;
	EState DebugStateForTests() const { return State; }
	bool DebugTurnInFlightForTests() const { return bTurnInFlight; }
	bool DebugHasAgentForTests() const { return ChatAgent.IsValid(); }
	/** Press Start/Stop as a click would. */
	void DebugStartStopForTests() { OnStartStopClicked(); }
	/** Type Text and press Send. */
	void DebugSendForTests(const FString& Text);

private:
	enum class EVoiceMode : uint8
	{
		PushToTalk,
		HandsFree,
	};

	// ── Layout helpers ────────────────────────────────────────────────────
	TSharedRef<SWidget> BuildConfigRow();
	TSharedRef<SWidget> BuildVoiceRow();
	TSharedRef<SWidget> BuildStartStopRow();
	TSharedRef<SWidget> BuildChatArea();
	TSharedRef<SWidget> BuildChatScroll();
	TSharedRef<SWidget> BuildInputRow();
	void RebuildChatScroll();
	void RefreshInspector();
	float MeasureChatDistance() const;
	void ScrollChatToEnd();
	float GetBubbleMaxWidth() const;
	float GetBubbleWrapWidth() const;
	TSharedRef<SWidget> WrapChatBubble(EHorizontalAlignment Align, TSharedRef<SWidget> Bubble) const;
	FReply OnAssistantBubbleClicked(FString TurnKey);

	// ── Component picker ──────────────────────────────────────────────────
	void RefreshAgentCandidates();
	void SyncSelectionFromEditor();
	TSharedRef<SWidget> BuildActorPicker();
	TSharedRef<SWidget> BuildComponentPicker();
	FText GetSelectedActorLabel() const;
	FText GetSelectedComponentLabel() const;
	void SelectActor(AActor* Actor);
	void SelectComponent(UTryllAgentComponent* Component);
	void SelectVoiceComponent(UTryllVoiceInputComponent* Component);
	TSharedRef<SWidget> BuildVoiceComponentPicker();
	FText GetSelectedVoiceComponentLabel() const;
	/** Hands this chat's agent and history to the Dialog Lab (§13). */
	FReply OnCompareInLabClicked();
	FString GetAssistantDisplayName() const;

	// ── Buttons / events ──────────────────────────────────────────────────
	FReply  OnStartStopClicked();
	FReply  OnSendClicked();
	FReply  OnStopClicked();
	void    OnInputCommitted(const FText& Text, ETextCommit::Type CommitType);
	FText   GetStartStopLabel() const;
	void    RefreshStatusChrome();
	void    HandleHostColorsRefreshed();
	void    HandleOwnerPhaseChanged(ETryllOwnerPhase Phase, const FString& Detail);

	// ── State machine ─────────────────────────────────────────────────────
	void Start();
	/** Tear down the session. Skip UI rebuild when the widget is already dying —
	 *  RebuildChatScroll BindSPs via WrapTextAt and AsShared() asserts in ~SWidget. */
	void Stop(bool bRefreshUi = true);
	void Fail(const FString& Reason);
	/** Missing agent/settings — stay Idle so Start stays Start (Unity RejectStart). */
	void RejectStart(const FString& Reason);
	void SetState(EState S, const FString& Status);
	void DetachPreviewFromSelected();

	// ── Turn helpers ──────────────────────────────────────────────────────
	bool BeginChatTurn(const FString& UserText, bool bFromVoice);
	bool FinalizeInFlightTurn(
		ETryllTurnStatus TurnStatus,
		const FString& DebugInfo,
		int32 TokensGenerated,
		const FString& ErrorText = FString());
	const FTryllChatTurnRecord* FindTurnByKey(const FString& TurnKey) const;

	// ── Persistence ───────────────────────────────────────────────────────
	void LoadChatConfig();
	void SaveChatConfig() const;

	// ── STT picker ────────────────────────────────────────────────────────
	void RebuildSttModelOptions();
	TArray<FString> RegisteredSttNames() const;
	void SetVoiceSource(ETryllChatVoiceSource Source);
	bool CanEnableVoice() const;
	TSharedRef<SWidget> BuildVoiceSourceMenu();
	FText GetVoiceSourceLabel() const;
	FText GetVoiceSourceTooltip() const;
	void RequestChatVoice(const FTryllVoiceInputConfig& Config, uint64 Gen);

	// ── Connection drive ──────────────────────────────────────────────────

	void HandleConnectionChanged(bool bConnected);
	void HandleCreateSessionComplete(bool bSuccess);
	void HandleError(const FString& ErrorMessage);
	void OnModelsReady(bool bSuccess, FString Error);

	// ── Voice ─────────────────────────────────────────────────────────────
	void    BeginUtterance(bool bAutoFinishOnSilence);
	void    EndUtterance();
	void    CancelUtterance();
	void    EnterHandsFree();
	void    ExitHandsFree();
	void    HandleTranscriptUpdate(const FString& Text, uint8 Kind);
	bool    StartCapture();
	void    StopCapture();
	bool    IsRecording() const { return bCapturing || bAwaitingUtteranceFinal; }

	void    OnMicPressed();
	void    OnMicReleased();
	TSharedRef<SWidget> BuildVoiceModeMenu();
	void    SetVoiceMode(EVoiceMode Mode);
	FText   GetVoiceModeLabel() const;

	// ── Autofind ──────────────────────────────────────────────────────────
	void AutofindSettings();

	// ── State ─────────────────────────────────────────────────────────────
	TWeakObjectPtr<UTryllRuntimeSettings> Settings;
	TWeakObjectPtr<UTryllAgentComponent>  SelectedComponent;
	TWeakObjectPtr<AActor>                SelectedActor;
	TArray<TWeakObjectPtr<AActor>>        CandidateActors;
	TArray<TWeakObjectPtr<UTryllAgentComponent>> CandidateComponents;
	TWeakObjectPtr<UTryllVoiceInputComponent> SelectedVoiceComponent;
	TArray<TWeakObjectPtr<UTryllVoiceInputComponent>> CandidateVoiceComponents;

	EState  State  = EState::Idle;
	FString Status = TEXT("Idle");

	/** Bumped on Start/Stop so in-flight CreateAgent/Voice callbacks ignore stale runs. */
	uint64 RunGeneration = 0;

	UTryllEditorSubsystem* EditorSubsystem = nullptr;
	UTryllEditorSubsystem* EditorSubsystemOverride = nullptr;
	bool bPersistLayout = true;
	bool bAcquiredConnection = false;

	FString LastErrorMessage;

	TSharedPtr<FTryllAgent> ChatAgent;
	bool                    bTurnInFlight = false;

	TSharedPtr<FTryllModelDownloadGuard> ModelGuard;

	TArray<FTryllChatTurnRecord> Turns;
	int32 TurnSequence = 0;
	int32 InFlightTurnIndex = INDEX_NONE;
	FString SelectedTurnKey;

	float InspectorWidth    = 400.0f;
	bool  bInspectorVisible = true;
	float LastBubbleMaxWidth = 0.0f;

	TSharedPtr<class STryllStatusBar> StatusBar;
	TSharedPtr<class STryllBanner>    Banner;
	FDelegateHandle OwnerPhaseHandle;

	TSharedPtr<SScrollBox>           ChatScroll;
	TSharedPtr<STryllTurnInspectorView> InspectorView;
	FTryllTailFollow                 ChatTail;
	TSharedPtr<SEditableTextBox>   InputBox;

	ETryllChatVoiceSource VoiceSource = ETryllChatVoiceSource::Off;
	FString    SttModelName;
	EVoiceMode VoiceMode    = EVoiceMode::PushToTalk;
	TArray<TSharedPtr<FString>> SttModelOptions;

	TSharedPtr<FTryllVoiceInput> ChatVoice;
	bool    bPttHeld                = false;
	bool    bHandsFreeActive        = false;
	bool    bAwaitingUtteranceFinal = false;
	bool    bUtteranceActive        = false;
	bool    bCapturing              = false;
	FString VoicePartial;
	FString PreVoiceInput;

	uint32 CaptureSampleRate = 16000;
	TUniquePtr<FTryllChatMicCapture> MicCapture;

	FDelegateHandle MapChangeHandle;
	FDelegateHandle ActorDeletedHandle;
	FDelegateHandle HostColorsHandle;
};
