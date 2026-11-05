// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "AudioDrivenAnimationMood.h"

namespace UE::NNE
{
	class IModelInstanceRunSync;
}

enum class ETryllSolverLoadResult : uint8
{
	Ok,
	ModelMissing,
	LoadFailed
};

class FTryllMetaHumanLipSyncSolver
{
public:

	ETryllSolverLoadResult Load(FString& OutReason);
	bool IsLoaded() const { return Model.IsValid(); }

	void Reset();

	void SetMood(EAudioDrivenAnimationMood InMood);
	void SetMoodIntensity(float InIntensity);
	void SetLookaheadMilliseconds(int32 InMilliseconds);

	void Process(TArrayView<const float> InSamples16kMono, TFunctionRef<void(const TMap<FString, float>&)> EmitFrame);

	TFunction<void()> OnInferenceFailed;

private:

	bool RunHop(TMap<FString, float>& OutRawControls);

	TSharedPtr<UE::NNE::IModelInstanceRunSync> Model;
	TArray<float> AudioBuffer;
	TArray<float> CurveValues;
	TArray<int64> Step;
	TArray<float> InputBuffer;

	FCriticalSection ParamsLock;
	EAudioDrivenAnimationMood Mood = EAudioDrivenAnimationMood::Neutral;
	float MoodIntensity = 1.0f;
	int32 LookaheadMilliseconds = 80;
};
