// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllVisemePlayer.h"

namespace
{
	constexpr int32 VisemeCount = static_cast<int32>(ETryllViseme::ou) + 1;

	/** Lip-contact shapes: they must visibly close even when brief. */
	bool IsClosure(ETryllViseme V)
	{
		return V == ETryllViseme::PP || V == ETryllViseme::FF;
	}

	float Smoothstep(float T)
	{
		return T * T * (3.0f - 2.0f * T);
	}

	FTryllVisemePose Pose(ETryllViseme Viseme, std::initializer_list<TPair<const TCHAR*, float>> Weights)
	{
		FTryllVisemePose Out;
		Out.Viseme = Viseme;
		for (const TPair<const TCHAR*, float>& W : Weights)
		{
			FTryllVisemeChannelWeight& C = Out.Channels.AddDefaulted_GetRef();
			C.Channel = FName(W.Key);
			C.Weight  = W.Value;
		}
		return Out;
	}
}

FTryllVisemePlayer::FTryllVisemePlayer()
{
	SetPoses({});
}

TArray<FTryllVisemePose> FTryllVisemePlayer::DefaultPoses()
{
	using V = ETryllViseme;
	return {
		Pose(V::sil, {}),
		Pose(V::PP, { {TEXT("mouthClose"), 0.55f}, {TEXT("mouthPressLeft"), 0.45f}, {TEXT("mouthPressRight"), 0.45f},
		              {TEXT("mouthPucker"), 0.15f} }),
		Pose(V::FF, { {TEXT("jawOpen"), 0.12f}, {TEXT("mouthLowerDownLeft"), 0.55f}, {TEXT("mouthLowerDownRight"), 0.55f},
		              {TEXT("mouthFunnel"), 0.15f} }),
		Pose(V::TH, { {TEXT("jawOpen"), 0.18f}, {TEXT("mouthStretchLeft"), 0.25f}, {TEXT("mouthStretchRight"), 0.25f},
		              {TEXT("mouthUpperUpLeft"), 0.20f}, {TEXT("mouthUpperUpRight"), 0.20f} }),
		Pose(V::DD, { {TEXT("jawOpen"), 0.22f}, {TEXT("mouthClose"), 0.15f} }),
		Pose(V::kk, { {TEXT("jawOpen"), 0.32f} }),
		Pose(V::CH, { {TEXT("jawOpen"), 0.18f}, {TEXT("mouthFunnel"), 0.45f}, {TEXT("mouthPucker"), 0.20f} }),
		Pose(V::SS, { {TEXT("jawOpen"), 0.08f}, {TEXT("mouthStretchLeft"), 0.55f}, {TEXT("mouthStretchRight"), 0.55f} }),
		Pose(V::nn, { {TEXT("jawOpen"), 0.14f}, {TEXT("mouthClose"), 0.20f} }),
		Pose(V::RR, { {TEXT("jawOpen"), 0.16f}, {TEXT("mouthPucker"), 0.35f}, {TEXT("mouthFunnel"), 0.15f} }),
		Pose(V::aa, { {TEXT("jawOpen"), 0.72f} }),
		Pose(V::E,  { {TEXT("jawOpen"), 0.38f}, {TEXT("mouthStretchLeft"), 0.45f}, {TEXT("mouthStretchRight"), 0.45f} }),
		Pose(V::ih, { {TEXT("jawOpen"), 0.28f}, {TEXT("mouthStretchLeft"), 0.28f}, {TEXT("mouthStretchRight"), 0.28f} }),
		Pose(V::oh, { {TEXT("jawOpen"), 0.48f}, {TEXT("mouthFunnel"), 0.55f}, {TEXT("mouthPucker"), 0.25f} }),
		Pose(V::ou, { {TEXT("jawOpen"), 0.28f}, {TEXT("mouthPucker"), 0.72f}, {TEXT("mouthFunnel"), 0.35f} }),
	};
}

void FTryllVisemePlayer::SetPoses(TConstArrayView<FTryllVisemePose> Poses)
{
	const TArray<FTryllVisemePose> Defaults = Poses.IsEmpty() ? DefaultPoses() : TArray<FTryllVisemePose>();
	const TConstArrayView<FTryllVisemePose> Source = Poses.IsEmpty() ? TConstArrayView<FTryllVisemePose>(Defaults) : Poses;

	Channels.Reset();
	for (const FTryllVisemePose& P : Source)
	{
		for (const FTryllVisemeChannelWeight& C : P.Channels)
		{
			if (!C.Channel.IsNone())
			{
				Channels.AddUnique(C.Channel);   // FName equality ignores case
			}
		}
	}

	PoseTable.SetNum(VisemeCount);
	for (TArray<float>& Row : PoseTable)
	{
		Row.Init(0.0f, Channels.Num());
	}
	for (const FTryllVisemePose& P : Source)
	{
		const int32 V = static_cast<int32>(P.Viseme);
		if (V < 0 || V >= VisemeCount) continue;
		for (const FTryllVisemeChannelWeight& C : P.Channels)
		{
			const int32 Index = Channels.IndexOfByKey(C.Channel);
			if (Index != INDEX_NONE)
			{
				PoseTable[V][Index] = C.Weight;
			}
		}
	}

	Target.Init(0.0f, Channels.Num());
	Previous.Init(0.0f, Channels.Num());
	Current.Init(0.0f, Channels.Num());
	Smoothed.Init(0.0f, Channels.Num());
}

void FTryllVisemePlayer::ApplySettings(const FTryllVisemeSettings& In)
{
	Settings.Intensity                 = In.Intensity;
	Settings.SmoothingPerSecond        = In.SmoothingPerSecond;
	Settings.ClosureSmoothingPerSecond = In.ClosureSmoothingPerSecond;
	Settings.AttackDecayMs             = In.AttackDecayMs;
	Settings.MaxHoldMs                 = In.MaxHoldMs;
	Settings.SyncOffsetMs              = In.SyncOffsetMs;
	Settings.bCompensateSmoothingLag   = In.bCompensateSmoothingLag;
}

void FTryllVisemePlayer::BeginTurn(int32 InSampleRate, int64 TurnOriginSamples)
{
	SampleRate = InSampleRate;
	TurnOrigin = TurnOriginSamples;
	Events.Reset();
}

void FTryllVisemePlayer::AddEvents(TArrayView<const FTryllVisemeEvent> InEvents)
{
	Events.Append(InEvents.GetData(), InEvents.Num());
}

void FTryllVisemePlayer::Reset()
{
	Events.Reset();
	ActiveViseme = ETryllViseme::sil;
}

float FTryllVisemePlayer::GetSmoothingLagMs() const
{
	if (!Settings.bCompensateSmoothingLag)
	{
		return 0.0f;
	}
	// Exponential smoothing at rate k delays a slowly varying target by ~1/k s;
	// the smoothstep cross-fade delays each transition by about half its length.
	const float Smoothing = Settings.SmoothingPerSecond > 0.0f ? 1000.0f / Settings.SmoothingPerSecond : 0.0f;
	return Smoothing + 0.5f * Settings.AttackDecayMs;
}

float FTryllVisemePlayer::GetCurve(FName Channel) const
{
	const int32 Index = Channels.IndexOfByKey(Channel);
	return Index == INDEX_NONE ? 0.0f : Smoothed[Index];
}

int32 FTryllVisemePlayer::FindActiveIndex(int64 TurnSamples) const
{
	int32 Lo = 0;
	int32 Hi = Events.Num() - 1;
	int32 Found = 0;
	while (Lo <= Hi)
	{
		const int32 Mid = (Lo + Hi) / 2;
		if (Events[Mid].SampleOffset <= TurnSamples)
		{
			Found = Mid;
			Lo = Mid + 1;
		}
		else
		{
			Hi = Mid - 1;
		}
	}
	return Found;
}

void FTryllVisemePlayer::FillPose(const FTryllVisemeEvent& Event, TArray<float>& Out) const
{
	const int32 V = static_cast<int32>(Event.Viseme);
	// The server scales each shape by how loud that stretch of speech is, so
	// quieter syllables open less. Closures and rests always arrive at 1.
	float W = Event.Weight;
	if (!(W > 0.0f)) W = 1.0f;
	if (W > 1.0f)    W = 1.0f;
	for (int32 I = 0; I < Channels.Num(); ++I)
	{
		Out[I] = (V >= 0 && V < VisemeCount) ? PoseTable[V][I] * W : 0.0f;
	}
}

void FTryllVisemePlayer::EvaluateTarget(int64 TurnSamples)
{
	if (Events.IsEmpty() || SampleRate <= 0)
	{
		ActiveViseme = ETryllViseme::sil;
		for (float& T : Target) T = 0.0f;
		return;
	}

	TurnSamples = FMath::Max<int64>(0, TurnSamples);
	const int32 Index = FindActiveIndex(TurnSamples);
	const FTryllVisemeEvent& Cur = Events[Index];

	// Backstop for the end of a turn: the server ends every turn with a rest,
	// but a cancelled turn or a dropped last frame would otherwise hold the
	// final shape indefinitely.
	if (Index == Events.Num() - 1 && Settings.MaxHoldMs > 0.0f
		&& TurnSamples - Cur.SampleOffset > static_cast<int64>(Settings.MaxHoldMs * SampleRate / 1000.0))
	{
		ActiveViseme = ETryllViseme::sil;
		for (float& T : Target) T = 0.0f;
		return;
	}

	ActiveViseme = Cur.Viseme;
	FillPose(Cur, Current);

	const int64 Attack = static_cast<int64>(Settings.AttackDecayMs * SampleRate / 1000.0);
	const int64 Into = TurnSamples - Cur.SampleOffset;
	if (Index > 0 && Attack > 0 && Into < Attack)
	{
		FillPose(Events[Index - 1], Previous);
		const float U = Smoothstep(FMath::Clamp(static_cast<float>(Into) / static_cast<float>(Attack), 0.0f, 1.0f));
		for (int32 I = 0; I < Channels.Num(); ++I)
		{
			Target[I] = Previous[I] + (Current[I] - Previous[I]) * U;
		}
	}
	else
	{
		Target = Current;
	}
}

void FTryllVisemePlayer::Tick(int64 PlayedSamples, float DeltaSeconds)
{
	int64 TurnSamples = PlayedSamples - TurnOrigin;
	if (SampleRate > 0)
	{
		// Read the track at what is audible now: played samples still have the
		// device's output latency ahead of them, and the smoothing below lags.
		const float AheadMs = Settings.SyncOffsetMs + GetSmoothingLagMs() - Settings.OutputLatencyMs;
		TurnSamples += static_cast<int64>(AheadMs * SampleRate / 1000.0f);
	}
	EvaluateTarget(TurnSamples);

	// A lip closure lasts ~70 ms; at the regular rate the lips would still be on
	// their way when the next shape arrives, so p/b/m never visibly shut.
	const float Rate = IsClosure(ActiveViseme)
		? FMath::Max(Settings.SmoothingPerSecond, Settings.ClosureSmoothingPerSecond)
		: Settings.SmoothingPerSecond;
	const float K = (Rate > 0.0f && DeltaSeconds > 0.0f) ? 1.0f - FMath::Exp(-Rate * DeltaSeconds) : 1.0f;

	for (int32 I = 0; I < Channels.Num(); ++I)
	{
		const float Goal = FMath::Clamp(Target[I] * Settings.Intensity, 0.0f, 1.0f);
		Smoothed[I] += (Goal - Smoothed[I]) * K;
	}
}
