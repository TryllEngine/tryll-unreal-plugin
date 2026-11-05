// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllMetaHumanLipSyncComponent.h"

#include "TryllMetaHumanLipSyncAnimInstance.h"
#include "TryllMetaHumanLipSyncLiveLinkSource.h"
#include "TryllMetaHumanLipSyncSolver.h"
#include "TryllMetaHumanLipSyncPaths.h"
#include "TryllSpeakerComponent.h"
#include "TryllAgentComponent.h"
#include "TryllVoiceProfile.h"

#include "Async/Async.h"
#include "Components/ChildActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/PoseAsset.h"
#include "Features/IModularFeatures.h"
#include "ILiveLinkClient.h"

namespace
{
	TSet<FString> GActiveSubjectNames;

	FString ReserveSubjectName(const FString& Desired)
	{
		FString Name = Desired;
		for (int32 Suffix = 2; GActiveSubjectNames.Contains(Name); ++Suffix)
		{
			Name = FString::Printf(TEXT("%s_%d"), *Desired, Suffix);
		}
		GActiveSubjectNames.Add(Name);
		return Name;
	}

	FString SanitizeSubjectName(const FString& In)
	{
		FString Out;
		Out.Reserve(In.Len());
		for (TCHAR Char : In)
		{
			Out.AppendChar(FChar::IsAlnum(Char) || Char == TEXT('_') ? Char : TEXT('_'));
		}
		return Out;
	}

	USkeletalMeshComponent* FindFaceMeshOn(const AActor* Actor)
	{
		if (!Actor)
		{
			return nullptr;
		}
		TArray<USkeletalMeshComponent*> Meshes;
		Actor->GetComponents(Meshes);
		for (USkeletalMeshComponent* Mesh : Meshes)
		{
			if (Mesh->GetFName() == TEXT("Face"))
			{
				return Mesh;
			}
		}
		return nullptr;
	}
}

UTryllMetaHumanLipSyncComponent::UTryllMetaHumanLipSyncComponent()
{
	// Ticks only in Viseme mode (switched on in BeginPlay); the neural solver
	// runs on its own thread.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	ArkitMapping = TSoftObjectPtr<UPoseAsset>(FSoftObjectPath(
		TEXT("/Game/MetaHumans/Common/Face/ARKit/PA_MetaHuman_ARKit_Mapping.PA_MetaHuman_ARKit_Mapping")));

	VisemeSettings.Intensity = 1.0f;
}

void UTryllMetaHumanLipSyncComponent::SetStatus(ETryllMetaHumanLipSyncStatus NewStatus)
{
	if (Status == NewStatus)
	{
		return;
	}
	Status = NewStatus;
	OnLipSyncStatusChanged.Broadcast(Status);
}

void UTryllMetaHumanLipSyncComponent::Fail(ETryllMetaHumanLipSyncStatus FailureStatus, const TCHAR* Message)
{
	UE_LOG(LogTryllMetaHumanLipSync, Warning, TEXT("%s: %s"), *GetNameSafe(GetOwner()), Message);
	Teardown();
	SetStatus(FailureStatus);
}

void UTryllMetaHumanLipSyncComponent::Teardown()
{
	if (Speaker && PcmHandle.IsValid())
	{
		Speaker->OnTtsPcm.Remove(PcmHandle);
	}
	PcmHandle.Reset();

	if (Speaker)
	{
		Speaker->OnTtsTurnStarted.Remove(TurnStartedHandle);
		Speaker->OnTtsVisemes.Remove(VisemesHandle);
	}
	TurnStartedHandle.Reset();
	VisemesHandle.Reset();
	SetComponentTickEnabled(false);
	VisemePlayer.Reset();

	if (Source)
	{
		if (IModularFeatures::Get().IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName))
		{
			IModularFeatures::Get().GetModularFeature<ILiveLinkClient>(ILiveLinkClient::ModularFeatureName).RemoveSource(SourceGuid);
		}
		Source->RequestSourceShutdown();
		Source.Reset();
	}
	SourceGuid.Invalidate();

	if (Solver)
	{
		Solver->OnInferenceFailed = nullptr;
		Solver.Reset();
	}

	GActiveSubjectNames.Remove(EffectiveSubjectName);
	EffectiveSubjectName.Reset();
}

bool UTryllMetaHumanLipSyncComponent::ResolveSpeaker()
{
	if (Speaker)
	{
		return true;
	}

	TArray<UTryllSpeakerComponent*> Speakers;
	GetOwner()->GetComponents(Speakers);
	if (Speakers.Num() == 1)
	{
		Speaker = Speakers[0];
		return true;
	}

	if (Speakers.IsEmpty())
	{
		Fail(ETryllMetaHumanLipSyncStatus::SpeakerNotFound, TEXT("no Tryll Speaker on the owner; set Speaker."));
	}
	else
	{
		Fail(ETryllMetaHumanLipSyncStatus::SpeakerAmbiguous, TEXT("several Tryll Speakers on the owner; set Speaker."));
	}
	return false;
}

USkeletalMeshComponent* UTryllMetaHumanLipSyncComponent::FindFaceMesh() const
{
	if (TargetMetaHuman)
	{
		return FindFaceMeshOn(TargetMetaHuman);
	}

	const AActor* Owner = GetOwner();
	if (USkeletalMeshComponent* Face = FindFaceMeshOn(Owner))
	{
		return Face;
	}

	TArray<UChildActorComponent*> Children;
	Owner->GetComponents(Children);
	for (const UChildActorComponent* Child : Children)
	{
		if (USkeletalMeshComponent* Face = FindFaceMeshOn(Child->GetChildActor()))
		{
			return Face;
		}
	}
	return nullptr;
}

void UTryllMetaHumanLipSyncComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!ResolveSpeaker())
	{
		return;
	}

	USkeletalMeshComponent* Face = FindFaceMesh();
	if (!Face)
	{
		Fail(ETryllMetaHumanLipSyncStatus::FaceNotFound, TEXT("no skeletal mesh named Face on the owner, its child actors, or Target MetaHuman."));
		return;
	}

	if (!IModularFeatures::Get().IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName))
	{
		Fail(ETryllMetaHumanLipSyncStatus::LiveLinkUnavailable, TEXT("Live Link is not available; enable the Live Link plugin."));
		return;
	}

	const FString Label =
#if WITH_EDITOR
		GetOwner()->GetActorLabel();
#else
		GetOwner()->GetName();
#endif
	EffectiveSubjectName = ReserveSubjectName(SanitizeSubjectName(SubjectName.IsEmpty() ? TEXT("TryllMetaHumanLipSync_") + Label : SubjectName));

	const bool bViseme = SolverType == ETryllMetaHumanLipSyncSolver::Viseme;
	UPoseAsset* Mapping = nullptr;
	if (bViseme ? !StartViseme(Mapping) : !StartNeural())
	{
		return;   // Start* has already failed with a status and a log line
	}

	UClass* FaceAnimClass = LoadClass<UTryllMetaHumanLipSyncAnimInstance>(nullptr, TryllMetaHumanLipSync::FaceAnimBlueprintClassPath);
	if (!FaceAnimClass)
	{
		Fail(ETryllMetaHumanLipSyncStatus::AnimBlueprintMissing, TEXT("the plugin's face anim blueprint is missing; reinstall the plugin."));
		return;
	}
	Face->SetAnimInstanceClass(FaceAnimClass);
	UTryllMetaHumanLipSyncAnimInstance* FaceInstance = Cast<UTryllMetaHumanLipSyncAnimInstance>(Face->GetAnimInstance());
	if (!FaceInstance)
	{
		Fail(ETryllMetaHumanLipSyncStatus::FaceNotFound, TEXT("Face mesh did not accept the face anim blueprint."));
		return;
	}
	FaceInstance->Subject = FLiveLinkSubjectName(FName(*EffectiveSubjectName));
	FaceInstance->bVisemeSolver = bViseme;
	FaceInstance->ArkitMapping = Mapping;

	UE_LOG(LogTryllMetaHumanLipSync, Log, TEXT("%s: driving %s.Face from Live Link subject %s (%s solver)."),
		*GetNameSafe(GetOwner()), *GetNameSafe(Face->GetOwner()), *EffectiveSubjectName, bViseme ? TEXT("viseme") : TEXT("neural"));
	SetStatus(ETryllMetaHumanLipSyncStatus::Bound);

	if (bViseme)
	{
		SetComponentTickEnabled(true);
	}
}

bool UTryllMetaHumanLipSyncComponent::StartNeural()
{
	Solver = MakeShared<FTryllMetaHumanLipSyncSolver>();
	FString LoadReason;
	switch (Solver->Load(LoadReason))
	{
	case ETryllSolverLoadResult::ModelMissing:
		Fail(ETryllMetaHumanLipSyncStatus::ModelMissing, TEXT("lip sync model missing; run Set Up in Project Settings > Plugins > Tryll MetaHuman Lip Sync."));
		return false;
	case ETryllSolverLoadResult::LoadFailed:
		Fail(ETryllMetaHumanLipSyncStatus::ModelLoadFailed, *FString::Printf(TEXT("lip sync model failed to load: %s."), *LoadReason));
		return false;
	case ETryllSolverLoadResult::Ok:
		break;
	}
	Solver->SetMood(Mood);
	Solver->SetMoodIntensity(MoodIntensity);
	Solver->SetLookaheadMilliseconds(Lookahead);

	TWeakObjectPtr<UTryllMetaHumanLipSyncComponent> WeakThis(this);
	Solver->OnInferenceFailed = [WeakThis]
	{
		AsyncTask(ENamedThreads::GameThread, [WeakThis]
		{
			if (UTryllMetaHumanLipSyncComponent* This = WeakThis.Get())
			{
				This->Fail(ETryllMetaHumanLipSyncStatus::InferenceFailed, TEXT("lip sync inference failed; see the NNE log lines above."));
			}
		});
	};

	Source = MakeShared<FTryllMetaHumanLipSyncLiveLinkSource>();
	SourceGuid = IModularFeatures::Get().GetModularFeature<ILiveLinkClient>(ILiveLinkClient::ModularFeatureName).AddSource(Source);
	Source->StartSubject(FName(*EffectiveSubjectName), Solver, Speaker->GetPlaybackClock(), Lookahead + 100);
	PcmHandle = Speaker->OnTtsPcm.AddUObject(this, &UTryllMetaHumanLipSyncComponent::HandleTtsPcm);

	return true;
}

bool UTryllMetaHumanLipSyncComponent::StartViseme(UPoseAsset*& OutMapping)
{
	// The viseme track only exists when the agent's voice asks for it (protocol v27).
	// The Neural solver reads PCM and does not need it.
	const UTryllAgentComponent* AgentComp = UTryllAgentComponent::FindForSpeaker(GetOwner(), Speaker);
	if (AgentComp && AgentComp->VoiceProfile && !AgentComp->VoiceProfile->Voice.bEmitVisemes)
	{
		UE_LOG(LogTryllMetaHumanLipSync, Warning,
			TEXT("%s: the Viseme solver needs the server's viseme track, but voice profile '%s' has Emit Visemes off, so the mouth will not move. Turn on Emit Visemes in the profile, or use the Neural solver."),
			*GetNameSafe(GetOwner()), *GetNameSafe(AgentComp->VoiceProfile));
	}

	OutMapping = ArkitMapping.LoadSynchronous();
	if (!OutMapping)
	{
		Fail(ETryllMetaHumanLipSyncStatus::ArkitMappingMissing, *FString::Printf(
			TEXT("ARKit mapping pose asset not found at %s; set ARKit Mapping to your MetaHuman's PA_MetaHuman_ARKit_Mapping."),
			*ArkitMapping.ToString()));
		return false;
	}

	VisemePlayer.SetPoses(VisemePoses);
	VisemePlayer.ApplySettings(VisemeSettings);

	// The subject carries one ARKit curve per pose-table channel; the face
	// AnimBP turns them into rig controls through the mapping pose asset.
	Source = MakeShared<FTryllMetaHumanLipSyncLiveLinkSource>();
	SourceGuid = IModularFeatures::Get().GetModularFeature<ILiveLinkClient>(ILiveLinkClient::ModularFeatureName).AddSource(Source);
	Source->StartDirectSubject(FName(*EffectiveSubjectName), VisemePlayer.GetChannels());

	TurnStartedHandle = Speaker->OnTtsTurnStarted.AddUObject(this, &UTryllMetaHumanLipSyncComponent::HandleTurnStarted);
	VisemesHandle     = Speaker->OnTtsVisemes.AddUObject(this, &UTryllMetaHumanLipSyncComponent::HandleVisemes);
	return true;
}

void UTryllMetaHumanLipSyncComponent::HandleTurnStarted(int32 SampleRate, int64 TurnOriginSamples)
{
	VisemePlayer.BeginTurn(SampleRate, TurnOriginSamples);
}

void UTryllMetaHumanLipSyncComponent::HandleVisemes(const FString& NodeName, TArrayView<const FTryllVisemeEvent> Events)
{
	VisemePlayer.AddEvents(Events);
}

void UTryllMetaHumanLipSyncComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (SolverType != ETryllMetaHumanLipSyncSolver::Viseme || !Source || !Speaker)
	{
		return;
	}
	VisemePlayer.ApplySettings(VisemeSettings);   // live-editable in Play
	VisemePlayer.Settings.OutputLatencyMs = VisemeSettings.bCompensateOutputLatency ? Speaker->GetOutputLatencyMs() : 0.0f;
	VisemePlayer.Tick(Speaker->GetPlaybackClock()->GetPlayedSamples(), DeltaTime);
	Source->PushDirectFrame(VisemePlayer.GetCurves());
}

void UTryllMetaHumanLipSyncComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Teardown();
	Super::EndPlay(EndPlayReason);
}

void UTryllMetaHumanLipSyncComponent::SetMood(EAudioDrivenAnimationMood InMood, float InIntensity)
{
	Mood = InMood;
	MoodIntensity = FMath::Clamp(InIntensity, 0.0f, 1.0f);
	if (Solver)
	{
		Solver->SetMood(Mood);
		Solver->SetMoodIntensity(MoodIntensity);
	}
}

void UTryllMetaHumanLipSyncComponent::HandleTtsPcm(const TArray<float>& Samples, int32 SampleRate)
{
	Source->PushAudio(TArray<float>(Samples), SampleRate);
}
