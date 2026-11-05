// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllLipSyncComponent.h"
#include "TryllAgentComponent.h"
#include "TryllVoiceProfile.h"
#include "TryllClientModule.h"
#include "TryllSpeakerComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

UTryllLipSyncComponent::UTryllLipSyncComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UTryllLipSyncComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!Speaker)
	{
		if (AActor* Owner = GetOwner())
		{
			Speaker = Owner->FindComponentByClass<UTryllSpeakerComponent>();
		}
	}
	if (!Speaker)
	{
		UE_LOG(LogTryll, Warning,
			TEXT("UTryllLipSyncComponent on %s: no Tryll Speaker assigned or found on the owner — nothing to lip-sync to."),
			*GetNameSafe(GetOwner()));
	}
	else
	{
		TurnStartedHandle = Speaker->OnTtsTurnStarted.AddUObject(this, &UTryllLipSyncComponent::HandleTurnStarted);
		VisemesHandle     = Speaker->OnTtsVisemes.AddUObject(this, &UTryllLipSyncComponent::HandleVisemes);

		// The viseme track only exists when the agent's voice asks for it (protocol v27).
		const UTryllAgentComponent* AgentComp = UTryllAgentComponent::FindForSpeaker(GetOwner(), Speaker);
		if (AgentComp && AgentComp->VoiceProfile && !AgentComp->VoiceProfile->Voice.bEmitVisemes)
		{
			UE_LOG(LogTryll, Warning,
				TEXT("UTryllLipSyncComponent on %s: voice profile '%s' has Emit Visemes off, so the server sends no viseme track and the mouth will not move. Turn on Emit Visemes in the profile."),
				*GetNameSafe(GetOwner()), *GetNameSafe(AgentComp->VoiceProfile));
		}
	}

	Player.SetPoses(Poses);
	Player.ApplySettings(Settings);
	BindFace(FaceMesh ? FaceMesh.Get() : FindFaceMesh());
}

void UTryllLipSyncComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Speaker)
	{
		Speaker->OnTtsTurnStarted.Remove(TurnStartedHandle);
		Speaker->OnTtsVisemes.Remove(VisemesHandle);
	}
	TurnStartedHandle.Reset();
	VisemesHandle.Reset();
	ReleaseFace();
	Player.Reset();

	Super::EndPlay(EndPlayReason);
}

USkeletalMeshComponent* UTryllLipSyncComponent::FindFaceMesh() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	TInlineComponentArray<USkeletalMeshComponent*> Meshes;
	Owner->GetComponents(Meshes);

	// The first mesh that actually carries a pose-table morph target; a
	// component named "Face" wins if several do (split head/body rigs).
	USkeletalMeshComponent* Best = nullptr;
	for (USkeletalMeshComponent* Mesh : Meshes)
	{
		const USkeletalMesh* Asset = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr;
		if (!Asset) continue;

		const bool bHasChannel = Player.GetChannels().ContainsByPredicate(
			[Asset](FName Channel) { return Asset->FindMorphTarget(Channel) != nullptr; });
		if (!bHasChannel) continue;

		if (Mesh->GetName().Contains(TEXT("Face")))
		{
			return Mesh;
		}
		if (!Best)
		{
			Best = Mesh;
		}
	}
	return Best;
}

void UTryllLipSyncComponent::BindFace(USkeletalMeshComponent* InFaceMesh)
{
	ReleaseFace();
	FaceMesh = InFaceMesh;
	BoundChannels.Reset();

	const USkeletalMesh* Asset = FaceMesh ? FaceMesh->GetSkeletalMeshAsset() : nullptr;
	if (!Asset)
	{
		UE_LOG(LogTryll, Warning,
			TEXT("UTryllLipSyncComponent on %s: no face mesh with ARKit morph targets found. Set Face Mesh, or call BindFace."),
			*GetNameSafe(GetOwner()));
		return;
	}

	const TArray<FName>& Channels = Player.GetChannels();
	for (int32 Index = 0; Index < Channels.Num(); ++Index)
	{
		if (Asset->FindMorphTarget(Channels[Index]))   // FName match ignores case
		{
			BoundChannels.Add(Index);
		}
	}

	// Our morph targets must be set before the mesh evaluates them this frame.
	FaceMesh->AddTickPrerequisiteComponent(this);

	UE_LOG(LogTryll, Log, TEXT("UTryllLipSyncComponent on %s: bound %d of %d channels on %s."),
		*GetNameSafe(GetOwner()), BoundChannels.Num(), Channels.Num(), *FaceMesh->GetName());
}

void UTryllLipSyncComponent::ReleaseFace()
{
	if (!FaceMesh)
	{
		return;
	}
	// A zero weight removes our override, handing the channel back to the
	// character's own animation.
	const TArray<FName>& Channels = Player.GetChannels();
	for (const int32 Index : BoundChannels)
	{
		FaceMesh->SetMorphTarget(Channels[Index], 0.0f);
	}
	FaceMesh->RemoveTickPrerequisiteComponent(this);
	BoundChannels.Reset();
}

void UTryllLipSyncComponent::HandleTurnStarted(int32 SampleRate, int64 TurnOriginSamples)
{
	Player.BeginTurn(SampleRate, TurnOriginSamples);
}

void UTryllLipSyncComponent::HandleVisemes(const FString& NodeName, TArrayView<const FTryllVisemeEvent> Events)
{
	Player.AddEvents(Events);
}

void UTryllLipSyncComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Speaker || !FaceMesh || BoundChannels.IsEmpty())
	{
		return;
	}

	// Settings are live-editable (Details panel, Blueprint), so re-read them.
	Player.ApplySettings(Settings);
	Player.Settings.OutputLatencyMs = Settings.bCompensateOutputLatency ? Speaker->GetOutputLatencyMs() : 0.0f;
	Player.Tick(Speaker->GetPlaybackClock()->GetPlayedSamples(), DeltaTime);

	const TArray<FName>& Channels = Player.GetChannels();
	const TConstArrayView<float> Curves = Player.GetCurves();
	for (const int32 Index : BoundChannels)
	{
		FaceMesh->SetMorphTarget(Channels[Index], Curves[Index]);
	}
}
