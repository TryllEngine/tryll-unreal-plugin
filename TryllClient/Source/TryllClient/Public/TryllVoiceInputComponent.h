// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TryllError.h"
#include "TryllModelName.h"
#include "TryllStoragePath.h"
#include "TryllVoiceInput.h"
#include "TryllVoiceInputComponent.generated.h"

class UTryllSubsystem;
class UTryllAgentComponent;
class FTryllVoiceInput;
class FTryllVoiceInputComponentForwardSpec;
class FTryllVoiceInputMicrophoneSpec;
struct FTryllMicCapture;

/** Whether a UTryllVoiceInputComponent has a microphone it can capture from. */
UENUM(BlueprintType)
enum class ETryllMicrophoneStatus : uint8
{
	/** Not checked yet: no CreateVoiceInput or BeginUtterance has run. */
	Unknown,
	/** The default capture device is present. */
	Available,
	/** The system reports no default capture device. */
	NoDevice,
	/** The device is present but its capture stream could not be opened or started. */
	CaptureFailed,
};

// ── Delegates ─────────────────────────────────────────────────────────────────

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllTranscriptUpdate, const FTryllTranscriptUpdate&, Update);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllVoiceError,       const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllVoiceErrorDetailed, const FTryllError&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTryllVoiceInputCreated);

/**
 * Actor component: owns one FTryllVoiceInput (STT) handle and drives it from the
 * system microphone. The Unreal counterpart to the Unity TryllVoiceInputComponent.
 *
 * Capture model: on-demand. The microphone opens on BeginUtterance() and closes
 * when the final transcript arrives (or on End / Cancel). The component ticks to
 * drain captured PCM and forward it to the server.
 *
 * Audio: the device-native sample rate is declared to the server, which resamples
 * to the STT model's expected rate. Multi-channel input is downmixed to mono. If
 * the default device's rate changes under a live voice input, the next
 * BeginUtterance recreates the voice input with the new rate (VoiceInputStarting).
 *
 * Microphone check: CreateVoiceInput() and every BeginUtterance() first check
 * that a default capture device exists. Without one the voice input is not created
 * (no STT model is loaded), GetMicrophoneStatus() says why, one warning is logged,
 * and each call raises OnErrorDetailed with ETryllErrorCode::MicrophoneUnavailable
 * (except automatic creation, which may run before listeners bind: read
 * GetMicrophoneStatus() at start-up). A later BeginUtterance() that finds a
 * microphone creates the voice input and raises ETryllErrorCode::VoiceInputStarting
 * for that call; so does one that finds the device's sample rate changed.
 *
 * All delegates fire on the game thread.
 */
UCLASS(ClassGroup = (Tryll), meta = (BlueprintSpawnableComponent))
class TRYLLCLIENT_API UTryllVoiceInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTryllVoiceInputComponent();

	// ── Properties ────────────────────────────────────────────────────────────

	/** STT model — picked from the project's registered models (Model Manager). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice", meta = (TryllModelType = "stt"))
	FTryllModelName ModelName = TEXT("Whisper Tiny EN (int8)");

	/**
	 * Agent that receives the final transcript. Leave unset to auto-use the
	 * UTryllAgentComponent on the same actor (the common case — the details panel
	 * can't pick sibling components, so this resolves in BeginPlay). Put the Voice
	 * component on an actor with no agent for transcribe-only mode.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice")
	UTryllAgentComponent* TargetAgent = nullptr;

	/** Let server VAD close the utterance on silence (stop-on-silence mode). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice")
	bool bAutoFinishOnSilence = true;

	/** Hard timeout (ms) per utterance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice")
	int32 MaxUtteranceMs = 60000;

	/** Silero VAD speech-probability threshold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VadThreshold = 0.5f;

	/** Silence duration (ms) that closes a speech segment. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice")
	int32 VadMinSilenceMs = 1000;

	/** Padding (ms) added around detected speech. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice")
	int32 VadSpeechPadMs = 250;

	/**
	 * Plain string storage containing hotword / wake-word phrases (one per line).
	 * Relative to TryllRuntimeSettings::StorageDataFolder, e.g. "hotwords.txt".
	 * Leave empty to disable hotword boosting.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice",
		meta = (TryllStorageKind = "plain"))
	FTryllStoragePath HotwordsStoragePath;

	/** Logit-bias score applied to hotword tokens during decoding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice",
		meta = (ClampMin = "0.0"))
	float HotwordsScore = 1.5f;

	/**
	 * Create the VoiceInput handle automatically once the session is ready (connected +
	 * configured); queues until OnCreateSessionComplete if it spawns earlier.
	 * (Name kept for compatibility; it fires on session-ready, not raw connect.)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice")
	bool bCreateOnConnect = true;

	// ── Actions ───────────────────────────────────────────────────────────────

	/**
	 * Create the server-side VoiceInput handle. Ignored when one already exists.
	 * Without a microphone it broadcasts MicrophoneUnavailable instead.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Voice")
	void CreateVoiceInput();

	/** Destroy the handle and release the server-side session. */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Voice")
	void DestroyVoiceInput();

	/**
	 * Open a new utterance and start microphone capture. A non-empty final
	 * transcript is sent through AgentOverride (or TargetAgent) unless that
	 * agent already has a turn in flight, in which case the transcript is
	 * dropped. Transcribe-only when both are null.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Voice")
	void BeginUtterance(UTryllAgentComponent* AgentOverride = nullptr);

	/** Commit the current utterance (push-to-talk release). */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Voice")
	void EndUtterance();

	/** Cancel the current utterance without producing a transcript. */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Voice")
	void CancelUtterance();

	// ── Queries ───────────────────────────────────────────────────────────────

	/** True once the server-side VoiceInput handle exists. */
	UFUNCTION(BlueprintPure, Category = "Tryll|Voice")
	bool HasVoiceInput() const;

	/**
	 * CreateVoiceInput config from this component's inspector fields.
	 * SampleRate is the capture device rate declared to the server.
	 * Chat uses this so an "Actor component" source cannot drift from runtime.
	 */
	FTryllVoiceInputConfig BuildVoiceInputConfig(uint32 SampleRate) const;

	/** True while an utterance is open. */
	UFUNCTION(BlueprintPure, Category = "Tryll|Voice")
	bool IsUtteranceActive() const;

	/** Result of the last microphone check. */
	UFUNCTION(BlueprintPure, Category = "Tryll|Voice")
	ETryllMicrophoneStatus GetMicrophoneStatus() const { return MicrophoneStatus; }

	/** Why the microphone is unavailable, readable as-is. Empty when it is not. */
	UFUNCTION(BlueprintPure, Category = "Tryll|Voice")
	FString GetMicrophoneStatusMessage() const { return MicrophoneStatusMessage; }

	/** True when the last check found no usable microphone (NoDevice or CaptureFailed). */
	UFUNCTION(BlueprintPure, Category = "Tryll|Voice")
	bool IsMicrophoneUnavailable() const;

	// ── Delegates ─────────────────────────────────────────────────────────────

	/** Fired for each transcript update (partial and final). */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllTranscriptUpdate OnTranscriptUpdate;

	/** Fired on every error, with its message only. OnErrorDetailed carries the code. */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllVoiceError OnError;

	/**
	 * Fired on every error, together with OnError, with the full FTryllError so a
	 * handler can branch on Code (e.g. ETryllErrorCode::MicrophoneUnavailable).
	 */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllVoiceErrorDetailed OnErrorDetailed;

	/** Fired when the VoiceInput handle has been created. */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllVoiceInputCreated OnVoiceInputCreated;

	/** Live handle. Null until CreateVoiceInput completes. */
	TSharedPtr<FTryllVoiceInput> GetVoiceInput() const { return Voice; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	friend class FTryllVoiceInputComponentForwardSpec;
	friend class FTryllVoiceInputMicrophoneSpec;

	UFUNCTION()
	void HandleConnectionChanged(bool bConnected);

	UFUNCTION()
	void HandleCreateSessionComplete(bool bSuccess);

	/// Auto-create the VoiceInput once the session is ready. No-op otherwise;
	/// re-driven by HandleCreateSessionComplete.
	void TryAutoCreate();

	/// bReportMicrophoneError is false for automatic creation, which can run before
	/// listeners bind: a missing microphone then only sets the status and logs.
	void CreateVoiceInputImpl(bool bReportMicrophoneError);

	void HandleTranscriptUpdate(const FTryllTranscriptUpdate& Update);
	void HandleVoiceError(const FTryllError& Error);

	/// Fires OnErrorDetailed and OnError.
	void BroadcastError(const FTryllError& Error);

	/// Probes the default capture device and updates MicrophoneStatus. True when
	/// capture can be attempted. CaptureFailed is kept while the device is still
	/// present: only a stream that opens clears it.
	bool CheckMicrophone();

	/// Problem is empty when the microphone is usable. Logs once per distinct
	/// problem, and once when it clears.
	void SetMicrophoneStatus(ETryllMicrophoneStatus Status, const FString& Problem);

	FTryllError MicrophoneError() const;
	FString OwnerLabel() const;

	bool StartCapture();
	void StopCapture();

	UTryllSubsystem*             TryllSubsystem = nullptr;
	TSharedPtr<FTryllVoiceInput> Voice;
	bool                         bCreatingVoiceInput = false;
	/// CreateVoiceInput was requested before the session was ready; create on configure-complete.
	bool                         bCreateQueued = false;
	/// AgentOverride from the open utterance. Null means use TargetAgent.
	TWeakObjectPtr<UTryllAgentComponent> UtteranceAgentOverride;

	// Microphone capture state (defined in the .cpp).
	FTryllMicCapture* MicCapture = nullptr;

	ETryllMicrophoneStatus MicrophoneStatus = ETryllMicrophoneStatus::Unknown;
	FString                MicrophoneStatusMessage;
	/// Problem already logged, so a repeated failure stays quiet. Empty when none.
	FString                LoggedMicProblem;
	/// CreateVoiceInput gave up for lack of a microphone; BeginUtterance retries it.
	bool                   bCreateSkippedNoMic = false;
	/// Default device rate from the latest CheckMicrophone.
	int32                  ProbedSampleRate = 0;
	/// Rate the live voice input declared to the server.
	int32                  DeclaredSampleRate = 0;
	/// Default-device probe: false when there is no device, else its preferred rate
	/// (0 when unknown). Tests replace it.
	TFunction<bool(int32& OutPreferredRate)> MicProbe;
};
