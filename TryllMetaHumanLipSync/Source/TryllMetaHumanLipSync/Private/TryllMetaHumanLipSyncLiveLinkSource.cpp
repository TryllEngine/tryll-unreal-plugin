// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllMetaHumanLipSyncLiveLinkSource.h"

#include "TryllMetaHumanLipSyncSolver.h"
#include "TryllMetaHumanLipSyncPaths.h"
#include "TryllSpeakerComponent.h"

#include "HAL/Event.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "HAL/RunnableThread.h"
#include "ILiveLinkClient.h"
#include "LiveLinkTypes.h"
#include "Misc/QualifiedFrameTime.h"
#include "Roles/LiveLinkBasicRole.h"

#define LOCTEXT_NAMESPACE "TryllMetaHumanLipSyncLiveLinkSource"

namespace
{
	constexpr int32 TryllBlocksPerSecond = 50;
	constexpr int32 TryllSolverSampleRate = 16000;
	constexpr double TryllTurnGapSeconds = 1.0;
	constexpr int32 TryllMaxBufferedSeconds = 120;
	const FFrameRate TryllSceneFrameRate(60, 1);

	const TArray<FName> ExtraPropertyNames = {
		"HeadControlSwitch", "HeadRoll", "HeadPitch", "HeadYaw",
		"HeadTranslationX", "HeadTranslationY", "HeadTranslationZ",
		"MHFDSVersion", "DisableFaceOverride"
	};
	const TArray<float> ExtraPropertyValues = { 0, 0, 0, 0, 0, 0, 0, 1, 1 };

	float SampleAtIndex(const float* Input, int32 InputCount, int32 Index, bool bHasPrevSample, float PrevSample)
	{
		if (Index < 0)
		{
			return bHasPrevSample ? PrevSample : (InputCount > 0 ? Input[0] : 0.0f);
		}
		if (Index >= InputCount)
		{
			return InputCount > 0 ? Input[InputCount - 1] : (bHasPrevSample ? PrevSample : 0.0f);
		}
		return Input[Index];
	}

	void ResampleBlock(const float* Input, int32 InputCount, int32 InputRate, int32 OutputRate, TArray<float>& Out, double& Phase, float& PrevSample, bool& bHasPrevSample)
	{
		Out.Reset();
		if (InputCount <= 0)
		{
			return;
		}

		if (InputRate == OutputRate)
		{
			Out.Append(Input, InputCount);
			PrevSample = Input[InputCount - 1];
			bHasPrevSample = true;
			Phase = 0.0;
			return;
		}

		const double Ratio = static_cast<double>(InputRate) / OutputRate;
		while (Phase < InputCount)
		{
			const int32 Index0 = FMath::FloorToInt(Phase);
			const float Frac = static_cast<float>(Phase - Index0);
			const float S0 = SampleAtIndex(Input, InputCount, Index0, bHasPrevSample, PrevSample);
			const float S1 = SampleAtIndex(Input, InputCount, Index0 + 1, bHasPrevSample, PrevSample);
			Out.Add(FMath::Lerp(S0, S1, Frac));
			Phase += Ratio;
		}
		Phase -= InputCount;

		PrevSample = Input[InputCount - 1];
		bHasPrevSample = true;
	}
}

FTryllMetaHumanLipSyncLiveLinkSource::~FTryllMetaHumanLipSyncLiveLinkSource()
{
	Stop();
	if (Thread)
	{
		Thread->WaitForCompletion();
		Thread.Reset();
	}
	if (WakeEvent)
	{
		FPlatformProcess::ReturnSynchEventToPool(WakeEvent);
		WakeEvent = nullptr;
	}
}

void FTryllMetaHumanLipSyncLiveLinkSource::ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid)
{
	LiveLinkClient = InClient;
	SourceGuid = InSourceGuid;
}

bool FTryllMetaHumanLipSyncLiveLinkSource::IsSourceStillValid() const
{
	return LiveLinkClient != nullptr && !bStopping;
}

bool FTryllMetaHumanLipSyncLiveLinkSource::RequestSourceShutdown()
{
	Stop();
	if (Thread)
	{
		Thread->WaitForCompletion();
		Thread.Reset();
	}
	LiveLinkClient = nullptr;
	return true;
}

FText FTryllMetaHumanLipSyncLiveLinkSource::GetSourceType() const
{
	return LOCTEXT("SourceType", "Tryll MetaHuman Lip Sync");
}

FText FTryllMetaHumanLipSyncLiveLinkSource::GetSourceMachineName() const
{
	return FText::FromString(FPlatformProcess::ComputerName());
}

FText FTryllMetaHumanLipSyncLiveLinkSource::GetSourceStatus() const
{
	return bStopping ? LOCTEXT("StatusStopped", "Stopped") : LOCTEXT("StatusActive", "Active");
}

void FTryllMetaHumanLipSyncLiveLinkSource::StartSubject(const FName& InSubjectName, TSharedPtr<FTryllMetaHumanLipSyncSolver> InSolver, TSharedPtr<FTryllTtsPlaybackClock> InClock, int32 InLeadMilliseconds)
{
	check(LiveLinkClient);
	check(!Thread);

	SubjectName = InSubjectName;
	Solver = InSolver;
	PlaybackClock = InClock;
	LeadMilliseconds = InLeadMilliseconds;

	WakeEvent = FPlatformProcess::GetSynchEventFromPool(false);

	Thread.Reset(FRunnableThread::Create(this, TEXT("TryllMetaHumanLipSync"), 0, TPri_Normal));
}

void FTryllMetaHumanLipSyncLiveLinkSource::StartDirectSubject(const FName& InSubjectName, const TArray<FName>& InPropertyNames)
{
	check(LiveLinkClient);
	check(!Thread);

	SubjectName = InSubjectName;

	FLiveLinkStaticDataStruct StaticDataStruct = FLiveLinkStaticDataStruct(FLiveLinkBaseStaticData::StaticStruct());
	FLiveLinkBaseStaticData& StaticData = *StaticDataStruct.Cast<FLiveLinkBaseStaticData>();
	StaticData.PropertyNames = InPropertyNames;
	LiveLinkClient->PushSubjectStaticData_AnyThread({ SourceGuid, SubjectName }, ULiveLinkBasicRole::StaticClass(), MoveTemp(StaticDataStruct));
	bStaticDataSent = true;
}

void FTryllMetaHumanLipSyncLiveLinkSource::PushDirectFrame(TConstArrayView<float> InValues)
{
	if (!LiveLinkClient || bStopping || !bStaticDataSent)
	{
		return;
	}

	FLiveLinkFrameDataStruct FrameDataStruct = FLiveLinkFrameDataStruct(FLiveLinkBaseFrameData::StaticStruct());
	FLiveLinkBaseFrameData& FrameData = *FrameDataStruct.Cast<FLiveLinkBaseFrameData>();
	FrameData.PropertyValues.Append(InValues.GetData(), InValues.Num());
	FrameData.MetaData.SceneTime = FQualifiedFrameTime(FFrameTime::FromDecimal(FPlatformTime::Seconds() * TryllSceneFrameRate.AsDecimal()), TryllSceneFrameRate);

	LiveLinkClient->PushSubjectFrameData_AnyThread({ SourceGuid, SubjectName }, MoveTemp(FrameDataStruct));
}

void FTryllMetaHumanLipSyncLiveLinkSource::PushAudio(TArray<float>&& InSamples, int32 InSampleRate)
{
	if (InSamples.IsEmpty() || InSampleRate <= 0)
	{
		return;
	}

	{
		FScopeLock Lock(&QueueLock);
		if (bStopping)
		{
			return;
		}
		FChunk& Chunk = Queue.AddDefaulted_GetRef();
		Chunk.Samples = MoveTemp(InSamples);
		Chunk.SampleRate = InSampleRate;
	}

	if (WakeEvent)
	{
		WakeEvent->Trigger();
	}
}

void FTryllMetaHumanLipSyncLiveLinkSource::Stop()
{
	bStopping = true;
	{
		FScopeLock Lock(&QueueLock);
		Queue.Reset();
	}
	if (WakeEvent)
	{
		WakeEvent->Trigger();
	}
}

uint32 FTryllMetaHumanLipSyncLiveLinkSource::Run()
{
	TArray<float> Staging;
	TArray<float> Resampled;
	int32 SampleRate = 0;
	int64 ReleasedSamples = 0;
	int64 ClockBaselineSamples = 0;
	double ResamplePhase = 0.0;
	float PrevInputSample = 0.0f;
	bool bHasPrevInputSample = false;
	double WallAnchor = -1.0;
	double LastReleaseTime = FPlatformTime::Seconds();
	bool bDiscontinuity = false;
	bool bLoggedOverflow = false;

	while (!bStopping)
	{
		{
			FScopeLock Lock(&QueueLock);
			for (FChunk& Chunk : Queue)
			{
				if (SampleRate == 0 || Chunk.SampleRate != SampleRate)
				{
					Staging.Reset();
					ReleasedSamples = 0;
					ClockBaselineSamples = PlaybackClock ? PlaybackClock->GetPlayedSamples() : 0;
					WallAnchor = -1.0;
					bDiscontinuity = true;
				}
				SampleRate = Chunk.SampleRate;
				Staging.Append(MoveTemp(Chunk.Samples));
			}
			Queue.Reset();
		}

		if (SampleRate == 0 || !Solver || !Solver->IsLoaded())
		{
			WakeEvent->Wait(MAX_uint32);
			continue;
		}

		const int32 BlockSamples = FMath::Max(1, SampleRate / TryllBlocksPerSecond);
		const int64 LeadSamples = static_cast<int64>(LeadMilliseconds) * SampleRate / 1000;

		const int32 MaxBufferedSamples = TryllMaxBufferedSeconds * SampleRate;
		if (Staging.Num() > MaxBufferedSamples)
		{
			const int32 DropCount = Staging.Num() - MaxBufferedSamples;
			Staging.RemoveAt(0, DropCount, EAllowShrinking::No);
			ReleasedSamples += DropCount;
			bDiscontinuity = true;
			if (!bLoggedOverflow)
			{
				bLoggedOverflow = true;
				UE_LOG(LogTryllMetaHumanLipSync, Warning, TEXT("Audio buffer exceeded %d seconds; dropping oldest samples."), TryllMaxBufferedSeconds);
			}
		}

		int64 TargetSamples;
		if (PlaybackClock)
		{
			TargetSamples = (PlaybackClock->GetPlayedSamples() - ClockBaselineSamples) + LeadSamples;
		}
		else
		{
			if (WallAnchor < 0.0)
			{
				WallAnchor = FPlatformTime::Seconds();
			}
			TargetSamples = static_cast<int64>((FPlatformTime::Seconds() - WallAnchor) * SampleRate) + LeadSamples;
		}

		if (TargetSamples - ReleasedSamples > SampleRate)
		{
			const int32 SkipCount = static_cast<int32>(FMath::Min<int64>(Staging.Num(), TargetSamples - ReleasedSamples - LeadSamples));
			if (SkipCount > 0)
			{
				Staging.RemoveAt(0, SkipCount, EAllowShrinking::No);
				bDiscontinuity = true;
			}
			ReleasedSamples = TargetSamples - LeadSamples;
		}

		if (Staging.Num() >= BlockSamples && ReleasedSamples < TargetSamples)
		{
			if (bDiscontinuity)
			{
				Solver->Reset();
				ResamplePhase = 0.0;
				bHasPrevInputSample = false;
				bDiscontinuity = false;
			}
			bLoggedOverflow = false;

			ResampleBlock(Staging.GetData(), BlockSamples, SampleRate, TryllSolverSampleRate, Resampled, ResamplePhase, PrevInputSample, bHasPrevInputSample);

			Staging.RemoveAt(0, BlockSamples, EAllowShrinking::No);
			ReleasedSamples += BlockSamples;
			LastReleaseTime = FPlatformTime::Seconds();

			Solver->Process(Resampled, [this](const TMap<FString, float>& RawControls)
			{
				if (!bStaticDataSent)
				{
					bStaticDataSent = true;
					PushStaticData(RawControls);
				}
				PushFrameData(RawControls);
			});
			continue;
		}

		if (!bDiscontinuity && FPlatformTime::Seconds() - LastReleaseTime > TryllTurnGapSeconds)
		{
			bDiscontinuity = true;
		}

		if (Staging.IsEmpty())
		{
			WakeEvent->Wait(MAX_uint32);
		}
		else
		{
			const int64 SamplesUntilDue = (ReleasedSamples + BlockSamples) - TargetSamples;
			const double SecondsUntilDue = SamplesUntilDue > 0 ? static_cast<double>(SamplesUntilDue) / SampleRate : 0.0;
			const uint32 WaitMs = static_cast<uint32>(FMath::Clamp(SecondsUntilDue * 1000.0, 1.0, 50.0));
			WakeEvent->Wait(WaitMs);
		}
	}

	return 0;
}

void FTryllMetaHumanLipSyncLiveLinkSource::PushStaticData(const TMap<FString, float>& InRawControls)
{
	FLiveLinkStaticDataStruct StaticDataStruct = FLiveLinkStaticDataStruct(FLiveLinkBaseStaticData::StaticStruct());
	FLiveLinkBaseStaticData& StaticData = *StaticDataStruct.Cast<FLiveLinkBaseStaticData>();

	for (const TPair<FString, float>& RawControl : InRawControls)
	{
		StaticData.PropertyNames.Add(FName(RawControl.Key));
	}
	StaticData.PropertyNames.Append(ExtraPropertyNames);

	LiveLinkClient->PushSubjectStaticData_AnyThread({ SourceGuid, SubjectName }, ULiveLinkBasicRole::StaticClass(), MoveTemp(StaticDataStruct));

	UE_LOG(LogTryllMetaHumanLipSync, Log, TEXT("Publishing subject \"%s\" with %d face controls."), *SubjectName.ToString(), InRawControls.Num());
}

void FTryllMetaHumanLipSyncLiveLinkSource::PushFrameData(const TMap<FString, float>& InRawControls)
{
	FLiveLinkFrameDataStruct FrameDataStruct = FLiveLinkFrameDataStruct(FLiveLinkBaseFrameData::StaticStruct());
	FLiveLinkBaseFrameData& FrameData = *FrameDataStruct.Cast<FLiveLinkBaseFrameData>();

	FrameData.PropertyValues.Reserve(InRawControls.Num() + ExtraPropertyValues.Num());
	for (const TPair<FString, float>& RawControl : InRawControls)
	{
		FrameData.PropertyValues.Add(RawControl.Value);
	}
	FrameData.PropertyValues.Append(ExtraPropertyValues);

	FrameData.MetaData.SceneTime = FQualifiedFrameTime(FFrameTime::FromDecimal(FPlatformTime::Seconds() * TryllSceneFrameRate.AsDecimal()), TryllSceneFrameRate);

	LiveLinkClient->PushSubjectFrameData_AnyThread({ SourceGuid, SubjectName }, MoveTemp(FrameDataStruct));
}

#undef LOCTEXT_NAMESPACE
