// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TryllTtsPlaybackEvents.h"
#include "TryllVisemeEvent.h"
#include "TryllSpeakerComponent.generated.h"

class UAudioComponent;
class USoundWaveProcedural;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTryllTtsAudioStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTryllTtsAudioFinished);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTryllTtsPcm, const TArray<float>& /*Samples*/, int32 /*SampleRate*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTryllTtsTurnStarted, int32 /*SampleRate*/, int64 /*TurnOriginSamples*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTryllTtsVisemes, const FString& /*NodeName*/, TArrayView<const FTryllVisemeEvent> /*Events*/);

/**
 * Tracks how many TTS samples the audio device has actually consumed, so
 * consumers pacing work against playback (e.g. lip sync) can slave to it.
 * Safe to query from any thread; the speaker updates it on the game thread.
 * Samples queued to a procedural sound that was discarded before draining
 * (format change, EndPlay) count as played.
 */
class TRYLLCLIENT_API FTryllTtsPlaybackClock
{
public:
	void OnSamplesQueued(int64 NumSamples);
	void SetWave(USoundWaveProcedural* InWave);
	int64 GetPlayedSamples() const;
	/** Every sample ever queued, played or not. The next queued sample lands at this position. */
	int64 GetQueuedSamples() const;

private:
	mutable FCriticalSection Lock;
	USoundWaveProcedural* Wave = nullptr;
	int64 QueuedSamples = 0;
};

/**
 * Plays streaming TTS audio from a server-side agent through a
 * USoundWaveProcedural fed by a hidden UAudioComponent.
 *
 * Add it to the same actor as a UTryllAgentComponent, or assign it to the
 * agent's Speaker slot; the agent forwards TTS audio frames here. If the
 * workflow speaks and the actor has no speaker, the agent creates one.
 *
 * Threading
 * ---------
 * HandleFormat and HandleAudio fire on the game thread (drained by
 * UTryllSubsystem::Tick). USoundWaveProcedural::QueueAudio is internally
 * thread-safe — it copies into its own consumer queue, which the audio
 * mixer thread drains.
 *
 * End-of-stream
 * -------------
 * TtsAudioFrame carries no is_final flag — the turn ends with TurnComplete,
 * which the agent forwards to HandleTurnComplete. The procedural sound never
 * finishes on its own (it plays silence when its queue is empty), so after
 * HandleTurnComplete the component ticks until the queue has drained, then
 * fires OnTtsAudioFinished.
 */
UCLASS(ClassGroup=(Tryll), meta=(BlueprintSpawnableComponent), DisplayName="Tryll Speaker")
class TRYLLCLIENT_API UTryllSpeakerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTryllSpeakerComponent();

	/**
	 * Fired once when the first PCM chunk of a turn is queued for playback.
	 * Always followed by exactly one OnTtsAudioFinished before it fires again.
	 */
	UPROPERTY(BlueprintAssignable, Category="Tryll|TTS")
	FOnTryllTtsAudioStarted OnTtsAudioStarted;

	/**
	 * Fired when the turn has completed and its queued audio has been handed to
	 * the audio engine — the engine's own buffers (a few tens of ms) are still
	 * to play out. Also fired immediately by StopPlayback while speaking.
	 */
	UPROPERTY(BlueprintAssignable, Category="Tryll|TTS")
	FOnTryllTtsAudioFinished OnTtsAudioFinished;

	/**
	 * Cancels the current playback: drops all queued audio and ignores further
	 * audio until the next turn announces its audio format. Fires
	 * OnTtsAudioFinished if speech was playing. Does not cancel the agent's
	 * turn — call Cancel on the agent for that. Safe to call repeatedly and
	 * when nothing is playing.
	 */
	UFUNCTION(BlueprintCallable, Category="Tryll|TTS")
	void StopPlayback();

	/**
	 * Called by UTryllAgentComponent when the server announces the audio
	 * format for the current turn. (Re)creates the procedural sound if the
	 * sample rate differs from the previous turn's.
	 * NodeName (the producing node's name — multi-sender attribution) is
	 * accepted for signature compatibility with FTryllAgent::OnTtsAudioFormat
	 * but otherwise unused: this component mixes all producers into one
	 * procedural sound.
	 */
	void HandleFormat(const FString& NodeName, uint32 SampleRate, uint16 Channels, uint16 BitsPerSample);

	/**
	 * Called by UTryllAgentComponent for each PCM chunk in the current turn.
	 * The view references caller-owned memory and is consumed synchronously.
	 * NodeName is accepted for signature compatibility (see HandleFormat) but
	 * unused.
	 */
	void HandleAudio(const FString& NodeName, TArrayView<const int16> Pcm);

	/**
	 * Called by UTryllAgentComponent for each viseme frame (one per synthesized
	 * sentence, when the agent's voice profile has Emit Visemes on). Re-broadcast on
	 * OnTtsVisemes so lip sync needs only this speaker: it already owns the
	 * audio and the playback clock the track is timed against.
	 */
	void HandleVisemes(const FString& NodeName, TArrayView<const FTryllVisemeEvent> Events);

	/**
	 * Called by UTryllAgentComponent when no more audio is coming for the current
	 * turn: on TurnComplete, and when the agent is destroyed or the connection drops.
	 */
	void HandleTurnComplete();

	/**
	 * Fired on the game thread for each PCM chunk after it is queued for
	 * playback, as normalized float samples in [-1, 1]. The array is a reused
	 * scratch buffer — copy it if you need it beyond the callback.
	 */
	FOnTryllTtsPcm OnTtsPcm;

	/**
	 * Fired on the game thread when a turn's audio format is announced, before
	 * any of its audio is queued. TurnOriginSamples is the playback-clock
	 * position (GetPlaybackClock()->GetPlayedSamples() units) at which the
	 * turn's first sample will play; viseme offsets are relative to it.
	 */
	FOnTryllTtsTurnStarted OnTtsTurnStarted;

	/** Fired on the game thread for each viseme frame. The view is valid only during the callback. */
	FOnTryllTtsVisemes OnTtsVisemes;

	UFUNCTION(BlueprintPure, Category="Tryll|TTS")
	int32 GetSampleRate() const { return static_cast<int32>(CurrentSampleRate); }

	TSharedRef<FTryllTtsPlaybackClock> GetPlaybackClock() const { return PlaybackClock.ToSharedRef(); }

	/** Audio engine output buffering after the playback clock's played count, in milliseconds. 0 without an audio device. */
	UFUNCTION(BlueprintPure, Category="Tryll|TTS")
	float GetOutputLatencyMs() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> AudioComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USoundWaveProcedural> ProceduralSound = nullptr;

	uint32 CurrentSampleRate = 0;
	FTryllTtsPlaybackEvents PlaybackEvents;

	TArray<float> PcmScratch;
	TSharedPtr<FTryllTtsPlaybackClock> PlaybackClock;
};
