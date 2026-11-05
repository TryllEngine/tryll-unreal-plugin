// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllError.h"
#include "TryllVoiceInput.generated.h"

class FTryllConnection;

// ── Value types ───────────────────────────────────────────────────────────────

/**
 * Classifies a transcript update. Mirrors the wire schema enum
 * Tryll::TranscriptUpdateKind — ordinal values must stay in sync.
 */
UENUM(BlueprintType)
enum class ETryllTranscriptUpdateKind : uint8
{
	/** VAD rising edge; Text is empty. */
	SpeechStart    = 0,
	/** Revisable in-progress hypothesis (online STT). */
	Partial        = 1,
	/** Engine committed a chunk; the utterance is still open. */
	SegmentFinal   = 2,
	/** Last update for this Begin/End cycle. */
	UtteranceFinal = 3,
};

/** A single transcript update delivered to OnTranscriptUpdate. */
USTRUCT(BlueprintType)
struct FTryllTranscriptUpdate
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tryll")
	FString Text;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll")
	ETryllTranscriptUpdateKind Kind = ETryllTranscriptUpdateKind::Partial;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll")
	int32 AudioMsConsumed = 0;
};

/**
 * Audio format of the PCM samples pushed via SendAudioBuffer.
 * The server resamples to the STT model's expected rate as needed.
 * Plain C++ struct — not exposed to Blueprint.
 */
struct FTryllAudioFormat
{
	uint32 SampleRate    = 16000;  // Hz
	uint16 Channels      = 1;      // mono
	uint16 BitsPerSample = 16;     // only 16-bit PCM is supported
};

/** Parameters for creating an FTryllVoiceInput handle. */
struct FTryllVoiceInputConfig
{
	FString           ModelName;                  // STT model catalog name
	FTryllAudioFormat InputFormat;                // format of samples passed to SendAudioBuffer
	float             VadThreshold    = 0.5f;     // Silero VAD speech-probability threshold
	uint32            VadMinSilenceMs = 1000;     // silence (ms) that closes a segment
	uint32            VadSpeechPadMs  = 250;      // padding (ms) around detected speech
	/** Relative path (resolved against the session storage folder) to a StringStorage
	 *  (kind=List) config whose phrases the STT decoder biases toward. Loaded on demand.
	 *  Empty (default) disables hotword biasing. */
	FString           HotwordsStoragePath;
	/** How strongly matched phrases are weighted against one another during beam
	 *  search (default 1.5). Measured on Parakeet TDT 0.6B v3: 1.8 recovered 5 of
	 *  8 target terms, 2.5 recovered 6 of 8. A lexicon line may override this for
	 *  itself by ending with ` :<number>`. */
	float             HotwordsScore   = 1.5f;
};

/** Per-utterance options passed to BeginUtterance. */
struct FTryllUtteranceOptions
{
	/** Let server VAD close the utterance on silence. */
	bool          bAutoFinishOnSilence = true;
	/** Hard timeout (ms) per utterance. */
	uint32        MaxUtteranceMs       = 60000;
};

// ── FTryllVoiceInput ──────────────────────────────────────────────────────────

/**
 * Plain C++ handle for a server-side VoiceInput (STT) session.
 * Created by UTryllSubsystem after a successful CreateVoiceInputResponse.
 * The Unreal counterpart to the C++ Tryll::VoiceInput and the Unity TryllVoiceInput.
 *
 * Ownership: typically held as TSharedPtr<FTryllVoiceInput> by UTryllVoiceInputComponent.
 * Lifetime: deterministic — the owning component releases the shared_ptr; this triggers
 * UTryllSubsystem::RequestDestroyVoiceInput().
 *
 * All callbacks are invoked on the game thread (from UTryllSubsystem::Tick).
 */
class TRYLLCLIENT_API FTryllVoiceInput final : public TSharedFromThis<FTryllVoiceInput>
{
public:
	FTryllVoiceInput(TSharedPtr<FTryllConnection> InConnection, std::uint64_t InVoiceInputId);
	~FTryllVoiceInput();

	[[nodiscard]] std::uint64_t GetVoiceInputId()  const noexcept { return VoiceInputId; }
	[[nodiscard]] bool          IsValid()          const noexcept { return Connection.IsValid() && VoiceInputId != 0; }
	[[nodiscard]] bool          IsUtteranceActive() const noexcept { return bUtteranceActive; }

	/**
	 * Open a new utterance. Rejects locally (via OnComplete) with UtteranceInProgress
	 * when one is already active, or VoiceInputNotFound when the handle is invalid.
	 */
	void BeginUtterance(
		const FTryllUtteranceOptions&       Opts,
		TFunction<void(const FTryllError&)> OnComplete = nullptr);

	/**
	 * Push a block of mono int16 PCM to the server. Fire-and-forget; dropped silently
	 * when no utterance is active.
	 */
	void SendAudioBuffer(const int16* Samples, int32 NumSamples);
	void SendAudioBuffer(const TArray<int16>& Samples) { SendAudioBuffer(Samples.GetData(), Samples.Num()); }

	/** Commit the current utterance (push-to-talk release). */
	void EndUtterance(TFunction<void(const FTryllError&)> OnComplete = nullptr);

	/** Cancel the current utterance without producing a transcript. */
	void CancelUtterance(TFunction<void(const FTryllError&)> OnComplete = nullptr);

	// ── Callback setters ──────────────────────────────────────────────────────

	/** Fired for each transcript update (partial and final). Invoked on the game thread. */
	void SetOnTranscriptUpdate(TFunction<void(const FTryllTranscriptUpdate&)> Callback);

	/** Fired on protocol-level errors not tied to a Begin/End/Cancel callback. */
	void SetOnError(TFunction<void(const FTryllError&)> Callback);

	/** Called by UTryllSubsystem when the DestroyVoiceInput Ack arrives; clears the local id. */
	void ClearAfterDestroyRequest();

	/** Complete outstanding Begin/End/Cancel callbacks on a drop or socket replacement. */
	void DrainPendingAcks(const FTryllError& TerminalError);

	// ── Dispatch (called by UTryllSubsystem on Tick) ──────────────────────────

	void HandleTranscriptUpdate(const FString& Text, ETryllTranscriptUpdateKind Kind, int32 AudioMsConsumed);
	void HandleError(const FTryllError& Error);

private:
	TSharedPtr<FTryllConnection> Connection;
	std::uint64_t                VoiceInputId = 0;
	bool                         bUtteranceActive = false;

	TFunction<void(const FTryllTranscriptUpdate&)> OnTranscriptUpdate;
	TFunction<void(const FTryllError&)>            OnError;
};
