// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllSpeakerComponent.h"
#include "TryllClientModule.h"

#include "AudioDevice.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"

void FTryllTtsPlaybackClock::OnSamplesQueued(int64 NumSamples)
{
	FScopeLock ScopeLock(&Lock);
	QueuedSamples += NumSamples;
}

void FTryllTtsPlaybackClock::SetWave(USoundWaveProcedural* InWave)
{
	FScopeLock ScopeLock(&Lock);
	Wave = InWave;
}

int64 FTryllTtsPlaybackClock::GetQueuedSamples() const
{
	FScopeLock ScopeLock(&Lock);
	return QueuedSamples;
}

int64 FTryllTtsPlaybackClock::GetPlayedSamples() const
{
	FScopeLock ScopeLock(&Lock);
	const int64 OutstandingSamples = Wave ? Wave->GetAvailableAudioByteCount() / static_cast<int64>(sizeof(int16)) : 0;
	return FMath::Max<int64>(0, QueuedSamples - OutstandingSamples);
}

float UTryllSpeakerComponent::GetOutputLatencyMs() const
{
	const FAudioDevice* Device = AudioComponent ? AudioComponent->GetAudioDevice() : nullptr;
	if (!Device || Device->GetSampleRate() <= 0.0f)
	{
		return 0.0f;
	}
	return 1000.0f * Device->GetBufferLength() * Device->GetNumBuffers() / Device->GetSampleRate();
}

UTryllSpeakerComponent::UTryllSpeakerComponent()
{
	PrimaryComponentTick.bCanEverTick          = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PlaybackClock = MakeShared<FTryllTtsPlaybackClock>();
}

void UTryllSpeakerComponent::BeginPlay()
{
	Super::BeginPlay();

	// Lazy-create a hidden audio component on the owning actor. Non-spatialized
	// 2D playback — TTS narration is not positional.
	if (!AudioComponent)
	{
		if (AActor* Owner = GetOwner())
		{
			AudioComponent = NewObject<UAudioComponent>(Owner, TEXT("TryllSpeakerAudio"));
			AudioComponent->bAutoActivate    = false;
			AudioComponent->bAllowSpatialization = false;
			AudioComponent->RegisterComponent();
		}
	}
}

void UTryllSpeakerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AudioComponent)
	{
		AudioComponent->Stop();
		AudioComponent = nullptr;
	}
	PlaybackClock->SetWave(nullptr);
	ProceduralSound   = nullptr;
	CurrentSampleRate = 0;
	PlaybackEvents.Reset();
	SetComponentTickEnabled(false);

	Super::EndPlay(EndPlayReason);
}

void UTryllSpeakerComponent::HandleFormat(const FString& NodeName, uint32 SampleRate, uint16 Channels, uint16 BitsPerSample)
{
	PlaybackEvents.OnFormat();

	if (Channels != 1 || BitsPerSample != 16)
	{
		UE_LOG(LogTryll, Warning,
			TEXT("UTryllSpeakerComponent: unsupported TTS format (%uch %ubit). Only mono int16 PCM is supported."),
			Channels, BitsPerSample);
		return;
	}

	if (ProceduralSound && CurrentSampleRate == SampleRate)
	{
		// Same format as previous turn — reuse the procedural sound; any
		// pending audio from the prior turn naturally drains first.
		OnTtsTurnStarted.Broadcast(static_cast<int32>(SampleRate), PlaybackClock->GetQueuedSamples());
		return;
	}

	if (!AudioComponent)
	{
		UE_LOG(LogTryll, Warning, TEXT("UTryllSpeakerComponent: HandleFormat called before BeginPlay registered the audio component."));
		return;
	}

	if (AudioComponent->IsPlaying())
	{
		AudioComponent->Stop();
	}

	UE_LOG(LogTryll, Log, TEXT("UTryllSpeakerComponent: HandleFormat sr=%u ch=%u bps=%u"),
		SampleRate, Channels, BitsPerSample);

	CurrentSampleRate = SampleRate;

	ProceduralSound = NewObject<USoundWaveProcedural>(this);
	ProceduralSound->SetSampleRate(SampleRate);
	ProceduralSound->NumChannels = 1;
	ProceduralSound->Duration    = INDEFINITELY_LOOPING_DURATION;
	ProceduralSound->bLooping    = false;
	ProceduralSound->SoundGroup  = SOUNDGROUP_Voice;

	PlaybackClock->SetWave(ProceduralSound);

	AudioComponent->SetSound(ProceduralSound);
	AudioComponent->Play();

	// After the wave swap, so listeners read the clock the new turn plays on.
	OnTtsTurnStarted.Broadcast(static_cast<int32>(SampleRate), PlaybackClock->GetQueuedSamples());
}

void UTryllSpeakerComponent::HandleAudio(const FString& NodeName, TArrayView<const int16> Pcm)
{
	if (!ProceduralSound || Pcm.Num() == 0 || PlaybackEvents.IsSuppressed())
	{
		return;
	}

	ProceduralSound->QueueAudio(
		reinterpret_cast<const uint8*>(Pcm.GetData()),
		static_cast<int32>(Pcm.Num() * sizeof(int16)));
	PlaybackClock->OnSamplesQueued(Pcm.Num());

	if (PlaybackEvents.OnAudioQueued())
	{
		UE_LOG(LogTryll, Log, TEXT("UTryllSpeakerComponent: first TTS chunk (%d samples)"), Pcm.Num());
		OnTtsAudioStarted.Broadcast();
	}

	if (OnTtsPcm.IsBound())
	{
		PcmScratch.SetNumUninitialized(Pcm.Num(), EAllowShrinking::No);
		for (int32 Index = 0; Index < Pcm.Num(); ++Index)
		{
			PcmScratch[Index] = static_cast<float>(Pcm[Index]) / 32768.0f;
		}
		OnTtsPcm.Broadcast(PcmScratch, static_cast<int32>(CurrentSampleRate));
	}
}

void UTryllSpeakerComponent::HandleVisemes(const FString& NodeName, TArrayView<const FTryllVisemeEvent> Events)
{
	if (Events.Num() > 0)
	{
		OnTtsVisemes.Broadcast(NodeName, Events);
	}
}

void UTryllSpeakerComponent::HandleTurnComplete()
{
	PlaybackEvents.OnTurnComplete();
	SetComponentTickEnabled(PlaybackEvents.IsAwaitingDrain());
}

void UTryllSpeakerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const int64 QueuedBytes = ProceduralSound ? ProceduralSound->GetAvailableAudioByteCount() : 0;
	const bool bFinished = PlaybackEvents.OnQueueLevel(QueuedBytes);
	if (!PlaybackEvents.IsAwaitingDrain())
	{
		SetComponentTickEnabled(false);
	}
	if (bFinished)
	{
		OnTtsAudioFinished.Broadcast();
	}
}

void UTryllSpeakerComponent::StopPlayback()
{
	const bool bFinished = PlaybackEvents.OnStop();
	SetComponentTickEnabled(false);

	// Discarding the wave rather than calling USoundWaveProcedural::ResetAudio:
	// ResetAudio drains the wave's single-consumer queue from this thread while
	// the audio thread may be draining it too. The next HandleFormat recreates it.
	if (AudioComponent)
	{
		AudioComponent->Stop();
	}
	PlaybackClock->SetWave(nullptr);
	ProceduralSound   = nullptr;
	CurrentSampleRate = 0;

	if (bFinished)
	{
		OnTtsAudioFinished.Broadcast();
	}
}
