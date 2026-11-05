// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllVisemeEvent.h"

/**
 * Plays a TTS viseme track as ARKit curve values, in step with audio playback.
 *
 * Engine-object free so any consumer can drive it: UTryllLipSyncComponent
 * writes the curves to morph targets, and a MetaHuman integration can push the
 * same curves through Epic's ARKit mapping.
 *
 * Per turn: BeginTurn (from UTryllSpeakerComponent::OnTtsTurnStarted), then
 * AddEvents for each viseme frame (OnTtsVisemes). Every frame: Tick with the
 * playback clock's played samples, then read GetCurves().
 *
 * What it does, as in the Unity client's TryllLipSync:
 * - the active viseme's pose, scaled by the event weight (loudness);
 * - a smoothstep cross-fade over AttackDecayMs into each new viseme;
 * - Intensity scales everything;
 * - exponential smoothing at SmoothingPerSecond, ClosureSmoothingPerSecond while
 *   p/b/m or f/v is showing (a ~70 ms closure never lands at the slow rate);
 * - the track is read ahead by the delay that smoothing adds, so tuning it does
 *   not move sync;
 * - rest once the last shape of a turn has been held MaxHoldMs.
 */
class TRYLLCLIENT_API FTryllVisemePlayer
{
public:
	struct FSettings
	{
		float Intensity                  = 0.4f;
		float SmoothingPerSecond         = 16.0f;
		float ClosureSmoothingPerSecond  = 60.0f;
		float AttackDecayMs              = 50.0f;
		float MaxHoldMs                  = 400.0f;
		/** Positive shows the face earlier. */
		float SyncOffsetMs               = 0.0f;
		/** Audio device latency after the playback clock's sample count. */
		float OutputLatencyMs            = 0.0f;
		bool  bCompensateSmoothingLag    = true;
	};

	FTryllVisemePlayer();

	FSettings Settings;

	/** Copies the editable settings (a component's Details panel) into Settings. */
	void ApplySettings(const FTryllVisemeSettings& In);

	/** Replaces the pose table. Empty uses DefaultPoses(). */
	void SetPoses(TConstArrayView<FTryllVisemePose> Poses);

	/** Hand-authored from published Oculus / VRChat viseme references (same table as the Unity client). */
	static TArray<FTryllVisemePose> DefaultPoses();

	/** Every channel any pose drives; a consumer binds exactly these. */
	const TArray<FName>& GetChannels() const { return Channels; }

	void BeginTurn(int32 SampleRate, int64 TurnOriginSamples);
	void AddEvents(TArrayView<const FTryllVisemeEvent> Events);

	/** Advances to the audible position. PlayedSamples is FTryllTtsPlaybackClock::GetPlayedSamples(). */
	void Tick(int64 PlayedSamples, float DeltaSeconds);

	/** Smoothed value per channel after the last Tick, 0..1, aligned with GetChannels(). */
	TConstArrayView<float> GetCurves() const { return Smoothed; }

	/** Current value of one channel (0 if it isn't driven). */
	float GetCurve(FName Channel) const;

	ETryllViseme GetActiveViseme() const { return ActiveViseme; }

	/** Look-ahead applied for the smoothing delay, in milliseconds. */
	float GetSmoothingLagMs() const;

	/** Drops the track and eases back to rest. */
	void Reset();

private:
	void EvaluateTarget(int64 TurnSamples);
	void FillPose(const FTryllVisemeEvent& Event, TArray<float>& Out) const;
	int32 FindActiveIndex(int64 TurnSamples) const;

	TArray<FName> Channels;
	/** [viseme][channel] weight at full strength. */
	TArray<TArray<float>> PoseTable;

	TArray<FTryllVisemeEvent> Events;
	int32 SampleRate = 0;
	int64 TurnOrigin = 0;

	TArray<float> Target;
	TArray<float> Previous;
	TArray<float> Current;
	TArray<float> Smoothed;
	ETryllViseme ActiveViseme = ETryllViseme::sil;
};
