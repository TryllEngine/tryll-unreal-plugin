// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "ILiveLinkSource.h"

class FTryllMetaHumanLipSyncSolver;
class FTryllTtsPlaybackClock;
class FRunnableThread;
class FEvent;

class TRYLLMETAHUMANLIPSYNC_API FTryllMetaHumanLipSyncLiveLinkSource : public ILiveLinkSource, public FRunnable
{
public:

	virtual ~FTryllMetaHumanLipSyncLiveLinkSource() override;

	virtual void ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid) override;
	virtual bool IsSourceStillValid() const override;
	virtual bool RequestSourceShutdown() override;
	virtual FText GetSourceType() const override;
	virtual FText GetSourceMachineName() const override;
	virtual FText GetSourceStatus() const override;
	virtual void InitializeSettings(ULiveLinkSourceSettings* InSettings) override {}

	void StartSubject(const FName& InSubjectName, TSharedPtr<FTryllMetaHumanLipSyncSolver> InSolver, TSharedPtr<FTryllTtsPlaybackClock> InClock, int32 InLeadMilliseconds);

	void PushAudio(TArray<float>&& InSamples, int32 InSampleRate);

	/**
	 * Direct mode, for curves computed on the game thread (the viseme solver):
	 * declares the subject's property names once, then PushDirectFrame sends one
	 * value per name each tick. No worker thread or model is involved.
	 */
	void StartDirectSubject(const FName& InSubjectName, const TArray<FName>& InPropertyNames);
	void PushDirectFrame(TConstArrayView<float> InValues);

	virtual uint32 Run() override;
	virtual void Stop() override;

private:

	void PushStaticData(const TMap<FString, float>& InRawControls);
	void PushFrameData(const TMap<FString, float>& InRawControls);

	struct FChunk
	{
		TArray<float> Samples;
		int32 SampleRate = 0;
	};

	ILiveLinkClient* LiveLinkClient = nullptr;
	FGuid SourceGuid;
	FName SubjectName;
	TSharedPtr<FTryllMetaHumanLipSyncSolver> Solver;
	TSharedPtr<FTryllTtsPlaybackClock> PlaybackClock;
	int32 LeadMilliseconds = 180;

	FCriticalSection QueueLock;
	TArray<FChunk> Queue;

	TUniquePtr<FRunnableThread> Thread;
	FEvent* WakeEvent = nullptr;
	FThreadSafeBool bStopping = false;
	FThreadSafeBool bStaticDataSent = false;
};
