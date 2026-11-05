// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllVisemeEvent.generated.h"

/**
 * Oculus 15-viseme set, as sent in TtsVisemeFrame.
 * Mirrors the FlatBuffers WireViseme enum in messages.fbs — ordinal values must stay in sync.
 */
UENUM(BlueprintType)
enum class ETryllViseme : uint8
{
	sil = 0 UMETA(DisplayName = "sil (rest)"),
	PP  = 1 UMETA(DisplayName = "PP (p, b, m)"),
	FF  = 2 UMETA(DisplayName = "FF (f, v)"),
	TH  = 3 UMETA(DisplayName = "TH (th)"),
	DD  = 4 UMETA(DisplayName = "DD (t, d)"),
	kk  = 5 UMETA(DisplayName = "kk (k, g)"),
	CH  = 6 UMETA(DisplayName = "CH (ch, j, sh)"),
	SS  = 7 UMETA(DisplayName = "SS (s, z)"),
	nn  = 8 UMETA(DisplayName = "nn (n, l)"),
	RR  = 9 UMETA(DisplayName = "RR (r)"),
	aa  = 10 UMETA(DisplayName = "aa (open vowel)"),
	E   = 11 UMETA(DisplayName = "E (e)"),
	ih  = 12 UMETA(DisplayName = "ih (i)"),
	oh  = 13 UMETA(DisplayName = "oh (o)"),
	ou  = 14 UMETA(DisplayName = "ou (u, w)"),
};

/**
 * One step of a TTS viseme track. The viseme holds until the next event's
 * offset (or the end of its sentence).
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllVisemeEvent
{
	GENERATED_BODY()

	/** Samples from the start of the turn's TTS audio, at the rate announced by the audio format. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Lip Sync")
	int64 SampleOffset = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Lip Sync")
	ETryllViseme Viseme = ETryllViseme::sil;

	/** How far the shape opens (0..1), from the loudness of that stretch of speech. Closures and rests are always 1. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Lip Sync")
	float Weight = 1.0f;
};

/** One channel of a viseme pose: a morph target / ARKit curve name and its weight at full strength. */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllVisemeChannelWeight
{
	GENERATED_BODY()

	/** ARKit name (jawOpen, mouthClose, …). Matched case-insensitively. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Lip Sync")
	FName Channel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Lip Sync", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float Weight = 0.0f;
};

/** The face a viseme makes, as ARKit channel weights. */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllVisemePose
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Lip Sync")
	ETryllViseme Viseme = ETryllViseme::sil;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Lip Sync")
	TArray<FTryllVisemeChannelWeight> Channels;
};

/** How a viseme track is played on a face. Shared by every Tryll lip-sync component. */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllVisemeSettings
{
	GENERATED_BODY()

	/** Scales every pose. 0 = no movement, 1 = the full pose table. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look", meta = (ClampMin = 0.0, ClampMax = 1.5))
	float Intensity = 0.4f;

	/** How quickly the face moves toward each shape. Lower is softer, higher is snappier. 0 disables smoothing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look", meta = (ClampMin = 0.0, ClampMax = 120.0))
	float SmoothingPerSecond = 16.0f;

	/** Faster rate used while a lip closure (p, b, m, f, v) is showing, so short closures actually land. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look", meta = (ClampMin = 0.0, ClampMax = 200.0))
	float ClosureSmoothingPerSecond = 60.0f;

	/** Cross-fade between consecutive shapes, in milliseconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look", meta = (ClampMin = 0.0, ClampMax = 200.0))
	float AttackDecayMs = 50.0f;

	/** Returns to rest if a turn ends without its closing rest (a cancelled turn). 0 disables. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look", meta = (ClampMin = 0.0))
	float MaxHoldMs = 400.0f;

	/** Shifts the face against the audio. Positive moves the face earlier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sync")
	float SyncOffsetMs = 0.0f;

	/** Holds the face back by the audio engine's output buffer, so it lines up with the sound as heard. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sync")
	bool bCompensateOutputLatency = true;

	/** Reads the track ahead by the delay the smoothing adds, so tuning smoothing does not shift sync. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sync")
	bool bCompensateSmoothingLag = true;
};
