// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllVoiceInputComponent.h"
#include "TryllRuntimeSettings.h"
#include "TryllSubsystem.h"
#include "TryllAgentComponent.h"
#include "TryllVoiceInput.h"
#include "TryllClientModule.h"

#include "AudioCaptureCore.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

// ── Microphone capture (game-thread drained, audio-thread filled) ─────────────

struct FTryllMicCapture
{
	Audio::FAudioCapture Capture;
	FCriticalSection     BufferLock;
	TArray<int16>        PendingPcm;
	int32                SampleRate = 16000;
};

namespace
{
	/**
	 * Probe the default capture device (the one OpenAudioCaptureStream opens). False when
	 * there is none; otherwise its preferred sample rate, 0 when the backend does not say.
	 */
	bool ProbeDefaultCaptureDevice(int32& OutPreferredRate)
	{
		Audio::FAudioCapture Probe;
		Audio::FCaptureDeviceInfo Info;
		if (!Probe.GetCaptureDeviceInfo(Info))
		{
			return false;
		}
		OutPreferredRate = Info.PreferredSampleRate;
		return true;
	}

	const TCHAR* const CaptureFailedProblem =
		TEXT("The default microphone could not be opened. It may be in use by another app or ")
		TEXT("blocked by Windows microphone privacy settings. Voice input is unavailable until it opens.");
}

// ── Construction / lifecycle ──────────────────────────────────────────────────

UTryllVoiceInputComponent::UTryllVoiceInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	MicProbe = &ProbeDefaultCaptureDevice;
}

void UTryllVoiceInputComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			TryllSubsystem = GI->GetSubsystem<UTryllSubsystem>();
		}
	}

	if (!TryllSubsystem)
	{
		UE_LOG(LogTryll, Warning, TEXT("TryllVoiceInputComponent: UTryllSubsystem unavailable"));
		return;
	}

	// Auto-discover a sibling agent when none was assigned. The details panel can't
	// pick sibling components (a component-reference property shows an asset picker
	// that always reads "None"), so default to the agent on the same actor —
	// matching how UTryllAgentComponent auto-discovers its Speaker.
	if (!TargetAgent)
	{
		if (AActor* Owner = GetOwner())
		{
			TargetAgent = Owner->FindComponentByClass<UTryllAgentComponent>();
		}
	}

	TryllSubsystem->OnConnectionChanged.AddDynamic(this, &UTryllVoiceInputComponent::HandleConnectionChanged);
	TryllSubsystem->OnCreateSessionComplete.AddDynamic(this, &UTryllVoiceInputComponent::HandleCreateSessionComplete);

	TryAutoCreate();
}

void UTryllVoiceInputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopCapture();
	DestroyVoiceInput();

	if (TryllSubsystem)
	{
		TryllSubsystem->OnConnectionChanged.RemoveDynamic(this, &UTryllVoiceInputComponent::HandleConnectionChanged);
		TryllSubsystem->OnCreateSessionComplete.RemoveDynamic(this, &UTryllVoiceInputComponent::HandleCreateSessionComplete);
		TryllSubsystem = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void UTryllVoiceInputComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!MicCapture || !Voice.IsValid())
	{
		return;
	}

	TArray<int16> Chunk;
	{
		FScopeLock Lock(&MicCapture->BufferLock);
		if (MicCapture->PendingPcm.Num() == 0)
		{
			return;
		}
		Chunk = MoveTemp(MicCapture->PendingPcm);
		MicCapture->PendingPcm.Reset();
	}
	Voice->SendAudioBuffer(Chunk);
}

// ── Queries ───────────────────────────────────────────────────────────────────

bool UTryllVoiceInputComponent::HasVoiceInput() const
{
	return Voice.IsValid() && Voice->IsValid();
}

bool UTryllVoiceInputComponent::IsUtteranceActive() const
{
	return Voice.IsValid() && Voice->IsUtteranceActive();
}

bool UTryllVoiceInputComponent::IsMicrophoneUnavailable() const
{
	return MicrophoneStatus == ETryllMicrophoneStatus::NoDevice
		|| MicrophoneStatus == ETryllMicrophoneStatus::CaptureFailed;
}

// ── VoiceInput lifecycle ──────────────────────────────────────────────────────

FTryllVoiceInputConfig UTryllVoiceInputComponent::BuildVoiceInputConfig(uint32 SampleRate) const
{
	FTryllVoiceInputConfig Config;
	Config.ModelName                 = ModelName.ToString();
	Config.InputFormat.SampleRate    = SampleRate;
	Config.InputFormat.Channels      = 1;   // capture is downmixed to mono
	Config.InputFormat.BitsPerSample = 16;
	Config.VadThreshold              = VadThreshold;
	Config.VadMinSilenceMs           = static_cast<uint32>(FMath::Max(0, VadMinSilenceMs));
	Config.VadSpeechPadMs            = static_cast<uint32>(FMath::Max(0, VadSpeechPadMs));
	Config.HotwordsStoragePath       = UTryllRuntimeSettings::NormalizeStorageRelativePath(HotwordsStoragePath.Path);
	Config.HotwordsScore             = HotwordsScore;
	return Config;
}

void UTryllVoiceInputComponent::CreateVoiceInput()
{
	CreateVoiceInputImpl(/*bReportMicrophoneError=*/true);
}

void UTryllVoiceInputComponent::CreateVoiceInputImpl(bool bReportMicrophoneError)
{
	if (HasVoiceInput() || bCreatingVoiceInput)
	{
		return;
	}
	if (!TryllSubsystem)
	{
		UE_LOG(LogTryll, Warning, TEXT("TryllVoiceInputComponent::CreateVoiceInput: no subsystem"));
		return;
	}
	if (!TryllSubsystem->IsSessionReady())
	{
		// Session not configured yet — postpone until OnCreateSessionComplete.
		bCreateQueued = true;
		return;
	}
	bCreateQueued = false;

	// Without a device there is no rate to declare, and the server would load an STT
	// model nothing can feed.
	bCreateSkippedNoMic = !CheckMicrophone();
	if (bCreateSkippedNoMic)
	{
		if (bReportMicrophoneError)
		{
			BroadcastError(MicrophoneError());
		}
		return;
	}

	bCreatingVoiceInput = true;

	const int32 SampleRate = ProbedSampleRate;
	const FTryllVoiceInputConfig Config = BuildVoiceInputConfig(static_cast<uint32>(SampleRate));

	TWeakObjectPtr<UTryllVoiceInputComponent> WeakThis(this);
	TryllSubsystem->RequestCreateVoiceInput(
		Config,
		[WeakThis, SampleRate](TSharedPtr<FTryllVoiceInput> NewVoice, FTryllError Error)
		{
			UTryllVoiceInputComponent* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			Self->bCreatingVoiceInput = false;

			if (!Error.IsOk() || !NewVoice.IsValid())
			{
				UE_LOG(LogTryll, Warning, TEXT("TryllVoiceInputComponent: CreateVoiceInput failed: %s"), *Error.Message);
				Self->BroadcastError(Error);
				return;
			}

			Self->DeclaredSampleRate = SampleRate;
			Self->Voice = NewVoice;
			Self->Voice->SetOnTranscriptUpdate(
				[Self](const FTryllTranscriptUpdate& Update) { Self->HandleTranscriptUpdate(Update); });
			Self->Voice->SetOnError(
				[Self](const FTryllError& Err) { Self->HandleVoiceError(Err); });
			Self->OnVoiceInputCreated.Broadcast();
		});
}

void UTryllVoiceInputComponent::DestroyVoiceInput()
{
	// Capture belongs to the handle's utterance; without the handle nothing drains it.
	StopCapture();
	if (!Voice.IsValid())
	{
		return;
	}
	if (Voice->IsUtteranceActive())
	{
		Voice->CancelUtterance();  // fire-and-forget — teardown
	}
	Voice->SetOnTranscriptUpdate(nullptr);
	Voice->SetOnError(nullptr);
	if (TryllSubsystem)
	{
		TryllSubsystem->RequestDestroyVoiceInput(Voice);
	}
	Voice.Reset();
}

// ── Utterance control ─────────────────────────────────────────────────────────

void UTryllVoiceInputComponent::BeginUtterance(UTryllAgentComponent* AgentOverride)
{
	if (IsUtteranceActive())
	{
		// Refuse before touching capture: the handle's own UtteranceInProgress would
		// reach the Begin error callback, which stops the live recording.
		BroadcastError(FTryllError(ETryllErrorCode::UtteranceInProgress, TEXT("Utterance already in progress")));
		return;
	}

	if (!CheckMicrophone())
	{
		BroadcastError(MicrophoneError());
		return;
	}

	if (!HasVoiceInput() && bCreateSkippedNoMic)
	{
		// A microphone appeared since creation was skipped.
		CreateVoiceInput();
		BroadcastError(FTryllError(ETryllErrorCode::VoiceInputStarting,
			TEXT("Microphone found; voice input is starting. Try again shortly.")));
		return;
	}

	if (!HasVoiceInput())
	{
		UE_LOG(LogTryll, Warning, TEXT("TryllVoiceInputComponent::BeginUtterance: no VoiceInput handle"));
		BroadcastError(FTryllError(ETryllErrorCode::VoiceInputNotFound, TEXT("VoiceInput not ready")));
		return;
	}

	if (ProbedSampleRate != DeclaredSampleRate)
	{
		// The default device changed since the voice input declared its rate: audio
		// sent now would be resampled from the wrong rate and transcribe as noise.
		// The microphone itself is fine, so the status stays Available.
		const FString Message = FString::Printf(
			TEXT("The microphone changed (%d Hz; voice input declared %d Hz). Voice input is restarting; try again shortly."),
			ProbedSampleRate, DeclaredSampleRate);
		UE_LOG(LogTryll, Log, TEXT("%s: %s"), *OwnerLabel(), *Message);
		DestroyVoiceInput();
		CreateVoiceInput();
		BroadcastError(FTryllError(ETryllErrorCode::VoiceInputStarting, Message));
		return;
	}

	const UTryllAgentComponent* Agent = AgentOverride ? AgentOverride : TargetAgent;
	UE_LOG(LogTryll, Log, TEXT("TryllVoiceInputComponent::BeginUtterance called (target=%s)"),
		Agent ? *Agent->GetName() : TEXT("none"));

	UtteranceAgentOverride = AgentOverride;

	FTryllUtteranceOptions Opts;
	Opts.bAutoFinishOnSilence = bAutoFinishOnSilence;
	Opts.MaxUtteranceMs       = static_cast<uint32>(FMath::Max(0, MaxUtteranceMs));

	TWeakObjectPtr<UTryllVoiceInputComponent> WeakThis(this);
	Voice->BeginUtterance(
		Opts,
		[WeakThis](const FTryllError& Error)
		{
			UTryllVoiceInputComponent* Self = WeakThis.Get();
			if (!Self || Error.IsOk())
			{
				return;
			}
			Self->StopCapture();
			Self->BroadcastError(Error);
		});

	// Capture starts now rather than on the ack: Begin marks the utterance active
	// eagerly and the command queue is ordered, so early audio is kept. Starting on
	// the ack clipped the first words, and an End that beat the ack left the late
	// ack to open a microphone no utterance would ever close.
	if (!StartCapture())
	{
		CancelUtterance();
		BroadcastError(MicrophoneError());
	}
}

void UTryllVoiceInputComponent::EndUtterance()
{
	StopCapture();
	if (Voice.IsValid())
	{
		TWeakObjectPtr<UTryllVoiceInputComponent> WeakThis(this);
		Voice->EndUtterance(
			[WeakThis](const FTryllError& Error)
			{
				UTryllVoiceInputComponent* Self = WeakThis.Get();
				if (Self && !Error.IsOk())
				{
					Self->BroadcastError(Error);
				}
			});
	}
}

void UTryllVoiceInputComponent::CancelUtterance()
{
	StopCapture();
	if (Voice.IsValid())
	{
		TWeakObjectPtr<UTryllVoiceInputComponent> WeakThis(this);
		Voice->CancelUtterance(
			[WeakThis](const FTryllError& Error)
			{
				UTryllVoiceInputComponent* Self = WeakThis.Get();
				if (Self && !Error.IsOk())
				{
					Self->BroadcastError(Error);
				}
			});
	}
}

// ── Microphone capture ────────────────────────────────────────────────────────

bool UTryllVoiceInputComponent::StartCapture()
{
	if (MicCapture)
	{
		return true;  // already capturing
	}
	if (!IsUtteranceActive())
	{
		return true;  // Begin already failed and reported; nothing to capture for
	}

	MicCapture = new FTryllMicCapture();
	// BeginUtterance has checked that the device still matches this rate.
	MicCapture->SampleRate = DeclaredSampleRate;

	FTryllMicCapture* Cap = MicCapture;
	Audio::FOnAudioCaptureFunction OnCapture =
		[Cap](const void* InAudio, int32 NumFrames, int32 NumChannels, int32 /*InSampleRate*/,
		      double /*StreamTime*/, bool /*bOverflow*/)
		{
			if (!InAudio || NumFrames <= 0 || NumChannels <= 0)
			{
				return;
			}
			const float* Samples = static_cast<const float*>(InAudio);

			TArray<int16> Mono;
			Mono.SetNumUninitialized(NumFrames);
			for (int32 Frame = 0; Frame < NumFrames; ++Frame)
			{
				float Sum = 0.f;
				for (int32 Ch = 0; Ch < NumChannels; ++Ch)
				{
					Sum += Samples[Frame * NumChannels + Ch];
				}
				const float Mixed = FMath::Clamp(Sum / NumChannels, -1.f, 1.f);
				Mono[Frame] = static_cast<int16>(FMath::RoundToInt(Mixed * 32767.f));
			}

			FScopeLock Lock(&Cap->BufferLock);
			Cap->PendingPcm.Append(Mono);
		};

	Audio::FAudioCaptureDeviceParams Params;  // default device
	if (!MicCapture->Capture.OpenAudioCaptureStream(Params, MoveTemp(OnCapture), 1024))
	{
		delete MicCapture;
		MicCapture = nullptr;
		SetMicrophoneStatus(ETryllMicrophoneStatus::CaptureFailed, CaptureFailedProblem);
		return false;
	}

	if (!MicCapture->Capture.StartStream())
	{
		MicCapture->Capture.CloseStream();
		delete MicCapture;
		MicCapture = nullptr;
		SetMicrophoneStatus(ETryllMicrophoneStatus::CaptureFailed, CaptureFailedProblem);
		return false;
	}

	if (MicrophoneStatus != ETryllMicrophoneStatus::Available)
	{
		SetMicrophoneStatus(ETryllMicrophoneStatus::Available, FString());
	}
	UE_LOG(LogTryll, Log, TEXT("TryllVoiceInputComponent: mic capture started (%d Hz)"), MicCapture->SampleRate);
	return true;
}

void UTryllVoiceInputComponent::StopCapture()
{
	if (!MicCapture)
	{
		return;
	}
	// StopStream blocks until the capture callback ceases — safe to delete afterwards.
	MicCapture->Capture.StopStream();
	MicCapture->Capture.CloseStream();
	delete MicCapture;
	MicCapture = nullptr;
}

// ── Event handlers ────────────────────────────────────────────────────────────

void UTryllVoiceInputComponent::HandleConnectionChanged(bool bConnected)
{
	if (!bConnected)
	{
		bCreateQueued = false;
		if (Voice.IsValid())
		{
			// Server gone — drop local state without sending DestroyVoiceInput.
			StopCapture();
			Voice->SetOnTranscriptUpdate(nullptr);
			Voice->SetOnError(nullptr);
			Voice.Reset();
		}
	}
	// Creation is driven by HandleCreateSessionComplete: the session must be
	// configured before CreateVoiceInput can succeed.
}

void UTryllVoiceInputComponent::HandleCreateSessionComplete(bool bSuccess)
{
	if (!bSuccess || HasVoiceInput())
	{
		return;
	}

	// An explicit CreateVoiceInput() queued before the session was ready takes
	// precedence and ignores bCreateOnConnect (manual Blueprint use).
	if (bCreateQueued)
	{
		CreateVoiceInput();
		return;
	}

	TryAutoCreate();
}

void UTryllVoiceInputComponent::TryAutoCreate()
{
	if (!bCreateOnConnect || HasVoiceInput() || bCreatingVoiceInput)
	{
		return;
	}

	// Automatic: BeginPlay can run before other components bind OnError, so a
	// missing microphone only sets the status and logs here.
	CreateVoiceInputImpl(/*bReportMicrophoneError=*/false);
}

void UTryllVoiceInputComponent::HandleTranscriptUpdate(const FTryllTranscriptUpdate& Update)
{
	OnTranscriptUpdate.Broadcast(Update);
	// Every UtteranceFinal closes the turn; stop capture so the next turn
	// starts from a fresh BeginUtterance. A non-empty final is sent through
	// the agent component unless that agent already has a turn in flight.
	if (Update.Kind != ETryllTranscriptUpdateKind::UtteranceFinal)
	{
		return;
	}

	StopCapture();
	UTryllAgentComponent* Override = UtteranceAgentOverride.Get();
	UtteranceAgentOverride.Reset();
	if (Update.Text.IsEmpty())
	{
		return;
	}

	UTryllAgentComponent* Target = Override ? Override : TargetAgent;
	if (!Target)
	{
		return;
	}
	if (Target->IsTurnInFlight())
	{
		UE_LOG(LogTryll, Warning,
			TEXT("TryllVoiceInputComponent: dropping transcript; agent turn already in flight"));
		return;
	}
	Target->SendMessage(Update.Text);
}

void UTryllVoiceInputComponent::HandleVoiceError(const FTryllError& Error)
{
	BroadcastError(Error);
}

void UTryllVoiceInputComponent::BroadcastError(const FTryllError& Error)
{
	OnErrorDetailed.Broadcast(Error);
	OnError.Broadcast(Error.Message);
}

// ── Microphone check ──────────────────────────────────────────────────────────

bool UTryllVoiceInputComponent::CheckMicrophone()
{
	int32 PreferredRate = 0;
	if (!MicProbe || !MicProbe(PreferredRate))
	{
		SetMicrophoneStatus(ETryllMicrophoneStatus::NoDevice,
			TEXT("No microphone found. Voice input is disabled; text chat is unaffected."));
		return false;
	}
	ProbedSampleRate = PreferredRate > 0 ? PreferredRate : 48000;
	if (MicrophoneStatus != ETryllMicrophoneStatus::CaptureFailed)
	{
		SetMicrophoneStatus(ETryllMicrophoneStatus::Available, FString());
	}
	return true;
}

void UTryllVoiceInputComponent::SetMicrophoneStatus(ETryllMicrophoneStatus Status, const FString& Problem)
{
	MicrophoneStatus        = Status;
	MicrophoneStatusMessage = Problem;

	if (!Problem.IsEmpty())
	{
		if (Problem == LoggedMicProblem)
		{
			return;
		}
		LoggedMicProblem = Problem;
		UE_LOG(LogTryll, Warning, TEXT("%s: %s"), *OwnerLabel(), *Problem);
	}
	else if (!LoggedMicProblem.IsEmpty())
	{
		LoggedMicProblem.Reset();
		UE_LOG(LogTryll, Log, TEXT("%s: microphone available again."), *OwnerLabel());
	}
}

FTryllError UTryllVoiceInputComponent::MicrophoneError() const
{
	return FTryllError(ETryllErrorCode::MicrophoneUnavailable, MicrophoneStatusMessage);
}

FString UTryllVoiceInputComponent::OwnerLabel() const
{
	const AActor* Owner = GetOwner();
	return FString::Printf(TEXT("TryllVoiceInputComponent on '%s'"),
		Owner ? *Owner->GetActorNameOrLabel() : *GetName());
}
