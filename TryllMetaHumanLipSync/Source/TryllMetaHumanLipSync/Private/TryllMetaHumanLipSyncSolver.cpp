// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllMetaHumanLipSyncSolver.h"

#include "TryllMetaHumanLipSyncPaths.h"

#include "GuiToRawControlsUtils.h"
#include "NNE.h"
#include "NNEModelData.h"
#include "NNERuntimeCPU.h"
#include "UObject/Package.h"

namespace
{
const TArray<FString> GuiCurveNames = {
	"CTRL_L_brow_down.ty",
	"CTRL_R_brow_down.ty",
	"CTRL_L_brow_lateral.ty",
	"CTRL_R_brow_lateral.ty",
	"CTRL_L_brow_raiseIn.ty",
	"CTRL_R_brow_raiseIn.ty",
	"CTRL_L_brow_raiseOut.ty",
	"CTRL_R_brow_raiseOut.ty",
	"CTRL_L_eye_blink.ty",
	"CTRL_R_eye_blink.ty",
	"CTRL_L_eye_squintInner.ty",
	"CTRL_R_eye_squintInner.ty",
	"CTRL_L_eye_cheekRaise.ty",
	"CTRL_R_eye_cheekRaise.ty",
	"CTRL_L_nose.ty",
	"CTRL_R_nose.ty",
	"CTRL_L_nose.tx",
	"CTRL_R_nose.tx",
	"CTRL_L_nose_nasolabialDeepen.ty",
	"CTRL_R_nose_nasolabialDeepen.ty",
	"CTRL_C_mouth.tx",
	"CTRL_L_mouth_upperLipRaise.ty",
	"CTRL_R_mouth_upperLipRaise.ty",
	"CTRL_L_mouth_lowerLipDepress.ty",
	"CTRL_R_mouth_lowerLipDepress.ty",
	"CTRL_L_mouth_cornerPull.ty",
	"CTRL_R_mouth_cornerPull.ty",
	"CTRL_L_mouth_stretch.ty",
	"CTRL_R_mouth_stretch.ty",
	"CTRL_L_mouth_dimple.ty",
	"CTRL_R_mouth_dimple.ty",
	"CTRL_L_mouth_cornerDepress.ty",
	"CTRL_R_mouth_cornerDepress.ty",
	"CTRL_L_mouth_purseU.ty",
	"CTRL_R_mouth_purseU.ty",
	"CTRL_L_mouth_purseD.ty",
	"CTRL_R_mouth_purseD.ty",
	"CTRL_L_mouth_towardsU.ty",
	"CTRL_R_mouth_towardsU.ty",
	"CTRL_L_mouth_towardsD.ty",
	"CTRL_R_mouth_towardsD.ty",
	"CTRL_L_mouth_funnelU.ty",
	"CTRL_R_mouth_funnelU.ty",
	"CTRL_L_mouth_funnelD.ty",
	"CTRL_R_mouth_funnelD.ty",
	"CTRL_L_mouth_lipsTogetherU.ty",
	"CTRL_R_mouth_lipsTogetherU.ty",
	"CTRL_L_mouth_lipsTogetherD.ty",
	"CTRL_R_mouth_lipsTogetherD.ty",
	"CTRL_L_mouth_lipBiteU.ty",
	"CTRL_R_mouth_lipBiteU.ty",
	"CTRL_L_mouth_lipBiteD.ty",
	"CTRL_R_mouth_lipBiteD.ty",
	"CTRL_L_mouth_sharpCornerPull.ty",
	"CTRL_R_mouth_sharpCornerPull.ty",
	"CTRL_L_mouth_pushPullU.ty",
	"CTRL_R_mouth_pushPullU.ty",
	"CTRL_L_mouth_pushPullD.ty",
	"CTRL_R_mouth_pushPullD.ty",
	"CTRL_L_mouth_cornerSharpnessU.ty",
	"CTRL_R_mouth_cornerSharpnessU.ty",
	"CTRL_L_mouth_cornerSharpnessD.ty",
	"CTRL_R_mouth_cornerSharpnessD.ty",
	"CTRL_L_mouth_lipsRollU.ty",
	"CTRL_R_mouth_lipsRollU.ty",
	"CTRL_L_mouth_lipsRollD.ty",
	"CTRL_R_mouth_lipsRollD.ty",
	"CTRL_C_jaw.ty",
	"CTRL_C_jaw.tx",
	"CTRL_C_jaw_fwdBack.ty",
	"CTRL_L_jaw_ChinRaiseD.ty",
	"CTRL_R_jaw_ChinRaiseD.ty",
	"CTRL_C_tongue_move.ty",
	"CTRL_C_tongue_move.tx",
	"CTRL_C_tongue_inOut.ty",
	"CTRL_C_tongue_tipMove.ty",
	"CTRL_C_tongue_tipMove.tx",
	"CTRL_C_tongue_wideNarrow.ty",
	"CTRL_C_tongue_press.ty",
	"CTRL_C_tongue_roll.ty",
	"CTRL_C_tongue_thickThin.ty"
};

constexpr int32 NumCurves = 81;
constexpr int32 NumMoodSlots = 21;
constexpr int32 WindowSamples = 16000;
constexpr int32 HopSamples = 320;
}

ETryllSolverLoadResult FTryllMetaHumanLipSyncSolver::Load(FString& OutReason)
{
	using namespace UE::NNE;

	check(GuiCurveNames.Num() == NumCurves);

	UNNEModelData* ModelData = LoadObject<UNNEModelData>(GetTransientPackage(), TryllMetaHumanLipSync::ModelObjectPath);
	if (!ModelData)
	{
		OutReason = FString::Printf(TEXT("the project speech-to-animation model %s was not found"), TryllMetaHumanLipSync::ModelObjectPath);
		return ETryllSolverLoadResult::ModelMissing;
	}

	TWeakInterfacePtr<INNERuntimeCPU> Runtime = GetRuntime<INNERuntimeCPU>("NNERuntimeORTCpu");
	if (!Runtime.IsValid())
	{
		OutReason = TEXT("NNERuntimeORTCpu is not available");
		return ETryllSolverLoadResult::LoadFailed;
	}

	TSharedPtr<IModelCPU> CPUModel = Runtime->CreateModelCPU(ModelData);
	if (!CPUModel)
	{
		OutReason = TEXT("failed to create the CPU model");
		return ETryllSolverLoadResult::LoadFailed;
	}

	Model = CPUModel->CreateModelInstanceCPU();

	if (!Model)
	{
		OutReason = TEXT("failed to create the model instance");
		return ETryllSolverLoadResult::LoadFailed;
	}

	const TArray<FTensorShape> InputShapes = {
		FTensorShape::Make({ WindowSamples }),
		FTensorShape::Make({ NumCurves }),
		FTensorShape::Make({ NumMoodSlots }),
		FTensorShape::Make({ 1 }),
		FTensorShape::Make({ 1 })
	};
	if (Model->SetInputTensorShapes(InputShapes) != EResultStatus::Ok)
	{
		Model.Reset();
		OutReason = TEXT("failed to set the model input shapes");
		return ETryllSolverLoadResult::LoadFailed;
	}

	Reset();
	return ETryllSolverLoadResult::Ok;
}

void FTryllMetaHumanLipSyncSolver::Reset()
{
	AudioBuffer.SetNumZeroed(WindowSamples);
	CurveValues.SetNumZeroed(NumCurves);
	Step.SetNumZeroed(1);
	InputBuffer.Reset();
}

void FTryllMetaHumanLipSyncSolver::SetMood(EAudioDrivenAnimationMood InMood)
{
	FScopeLock Lock(&ParamsLock);
	Mood = InMood;
}

void FTryllMetaHumanLipSyncSolver::SetMoodIntensity(float InIntensity)
{
	FScopeLock Lock(&ParamsLock);
	MoodIntensity = InIntensity;
}

void FTryllMetaHumanLipSyncSolver::SetLookaheadMilliseconds(int32 InMilliseconds)
{
	FScopeLock Lock(&ParamsLock);
	LookaheadMilliseconds = InMilliseconds;
}

void FTryllMetaHumanLipSyncSolver::Process(TArrayView<const float> InSamples16kMono, TFunctionRef<void(const TMap<FString, float>&)> EmitFrame)
{
	if (!Model.IsValid())
	{
		return;
	}

	InputBuffer.Append(InSamples16kMono.GetData(), InSamples16kMono.Num());

	while (InputBuffer.Num() >= HopSamples)
	{
		FMemory::Memmove(AudioBuffer.GetData(), &AudioBuffer.GetData()[HopSamples], (WindowSamples - HopSamples) * sizeof(float));
		FMemory::Memcpy(&AudioBuffer.GetData()[WindowSamples - HopSamples], InputBuffer.GetData(), HopSamples * sizeof(float));
		InputBuffer.RemoveAt(0, HopSamples, EAllowShrinking::No);

		TMap<FString, float> RawControls;
		if (!RunHop(RawControls))
		{
			return;
		}

		EmitFrame(RawControls);
	}
}

bool FTryllMetaHumanLipSyncSolver::RunHop(TMap<FString, float>& OutRawControls)
{
	using namespace UE::NNE;

	TArray<float> MoodArray;
	MoodArray.SetNumZeroed(NumMoodSlots);
	TArray<int64> LookaheadArray;
	LookaheadArray.SetNumZeroed(1);

	{
		FScopeLock Lock(&ParamsLock);
		const uint8 MoodIndex = static_cast<uint8>(Mood);
		if (MoodIndex < NumMoodSlots)
		{
			MoodArray[MoodIndex] = MoodIntensity;
		}
		LookaheadArray[0] = LookaheadMilliseconds / 20;
	}

	const TArray<FTensorBindingCPU> Inputs = {
		{ (void*)AudioBuffer.GetData(), AudioBuffer.Num() * sizeof(float) },
		{ (void*)CurveValues.GetData(), CurveValues.Num() * sizeof(float) },
		{ (void*)MoodArray.GetData(), MoodArray.Num() * sizeof(float) },
		{ (void*)LookaheadArray.GetData(), LookaheadArray.Num() * sizeof(int64) },
		{ (void*)Step.GetData(), Step.Num() * sizeof(int64) }
	};
	const TArray<FTensorBindingCPU> Outputs = {
		{ (void*)CurveValues.GetData(), CurveValues.Num() * sizeof(float) },
		{ (void*)Step.GetData(), Step.Num() * sizeof(int64) }
	};

	if (Model->RunSync(Inputs, Outputs) != EResultStatus::Ok)
	{
		Model.Reset();
		if (OnInferenceFailed)
		{
			OnInferenceFailed();
		}
		return false;
	}

	TMap<FString, float> GuiControls;
	GuiControls.Reserve(NumCurves);
	for (int32 Index = 0; Index < NumCurves; ++Index)
	{
		GuiControls.Add(GuiCurveNames[Index], CurveValues[Index]);
	}

	OutRawControls = GuiToRawControlsUtils::ConvertGuiToRawControls(GuiControls);
	return true;
}
