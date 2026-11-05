// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "AudioDrivenAnimationMood.h"
#include "TryllVisemeEvent.h"
#include "TryllVisemePlayer.h"

#include "TryllMetaHumanLipSyncComponent.generated.h"

UENUM(BlueprintType)
enum class ETryllMetaHumanLipSyncStatus : uint8
{
	NotStarted,
	Bound,
	SpeakerNotFound,
	SpeakerAmbiguous,
	FaceNotFound,
	LiveLinkUnavailable,
	AnimBlueprintMissing,
	ModelMissing,
	ModelLoadFailed,
	InferenceFailed,
	/** Viseme solver: the ARKit mapping pose asset could not be loaded. */
	ArkitMappingMissing
};

/** Which solver turns speech into face motion. Chosen at BeginPlay. */
UENUM(BlueprintType)
enum class ETryllMetaHumanLipSyncSolver : uint8
{
	/** Epic's speech-to-animation model, run on the TTS audio. Drives the whole face. */
	Neural,
	/** The Tryll server's viseme track, mapped through Epic's ARKit mapping. Drives the mouth. */
	Viseme
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTryllMetaHumanLipSyncStatusChanged, ETryllMetaHumanLipSyncStatus, NewStatus);

class FTryllMetaHumanLipSyncLiveLinkSource;
class UPoseAsset;
class FTryllMetaHumanLipSyncSolver;
class UTryllSpeakerComponent;
class USkeletalMeshComponent;

UCLASS(ClassGroup=(Tryll), meta=(BlueprintSpawnableComponent), DisplayName="Tryll MetaHuman Lip Sync")
class TRYLLMETAHUMANLIPSYNC_API UTryllMetaHumanLipSyncComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UTryllMetaHumanLipSyncComponent();

	/** MetaHuman to animate. If unset, the owner and then its child actors are searched for a skeletal mesh named Face. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character")
	TObjectPtr<AActor> TargetMetaHuman;

	/** Speaker whose TTS audio drives the face. If unset, the owner must have exactly one Tryll Speaker. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TObjectPtr<UTryllSpeakerComponent> Speaker;

	/** Neural: Epic's model on the audio (whole face). Viseme: the server's viseme track (mouth; the agent's voice profile needs Emit Visemes on). Read at BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation", meta=(DisplayName="Solver"))
	ETryllMetaHumanLipSyncSolver SolverType = ETryllMetaHumanLipSyncSolver::Neural;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation", meta=(EditCondition="SolverType == ETryllMetaHumanLipSyncSolver::Neural", EditConditionHides))
	EAudioDrivenAnimationMood Mood = EAudioDrivenAnimationMood::Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation", meta=(ClampMin=0.0, ClampMax=1.0, EditCondition="SolverType == ETryllMetaHumanLipSyncSolver::Neural", EditConditionHides))
	float MoodIntensity = 1.0f;

	/** Solver lookahead in milliseconds. Higher is smoother and adds latency. Read once at BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation", meta=(ClampMin=80, ClampMax=240, EditCondition="SolverType == ETryllMetaHumanLipSyncSolver::Neural", EditConditionHides))
	int32 Lookahead = 80;

	/** Viseme look and sync. Live-editable in Play. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation", meta=(DisplayName="Viseme Settings", EditCondition="SolverType == ETryllMetaHumanLipSyncSolver::Viseme", EditConditionHides))
	FTryllVisemeSettings VisemeSettings;

	/** Viseme poses as ARKit channel weights. Empty uses the built-in table (the same one the plain Tryll Lip Sync uses). Read at BeginPlay. */
	UPROPERTY(EditAnywhere, Category="Animation", meta=(EditCondition="SolverType == ETryllMetaHumanLipSyncSolver::Viseme", EditConditionHides))
	TArray<FTryllVisemePose> VisemePoses;

	/** Epic's ARKit → MetaHuman pose asset. Every exported MetaHuman carries one under Common/Face/ARKit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation", meta=(DisplayName="ARKit Mapping", EditCondition="SolverType == ETryllMetaHumanLipSyncSolver::Viseme", EditConditionHides))
	TSoftObjectPtr<UPoseAsset> ArkitMapping;

	/** Live Link subject name. If empty, one is derived from the owner's label. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Advanced")
	FString SubjectName;

	UFUNCTION(BlueprintCallable, Category="Tryll MetaHuman Lip Sync")
	void SetMood(EAudioDrivenAnimationMood InMood, float InIntensity);

	UFUNCTION(BlueprintPure, Category="Tryll MetaHuman Lip Sync")
	FString GetEffectiveSubjectName() const { return EffectiveSubjectName; }

	UFUNCTION(BlueprintPure, Category="Tryll MetaHuman Lip Sync")
	ETryllMetaHumanLipSyncStatus GetLipSyncStatus() const { return Status; }

	UPROPERTY(BlueprintAssignable, Category="Tryll MetaHuman Lip Sync")
	FTryllMetaHumanLipSyncStatusChanged OnLipSyncStatusChanged;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	bool ResolveSpeaker();
	USkeletalMeshComponent* FindFaceMesh() const;
	void HandleTtsPcm(const TArray<float>& Samples, int32 SampleRate);
	bool StartNeural();
	bool StartViseme(UPoseAsset*& OutMapping);
	void HandleTurnStarted(int32 SampleRate, int64 TurnOriginSamples);
	void HandleVisemes(const FString& NodeName, TArrayView<const FTryllVisemeEvent> Events);
	void Fail(ETryllMetaHumanLipSyncStatus FailureStatus, const TCHAR* Message);
	void SetStatus(ETryllMetaHumanLipSyncStatus NewStatus);
	void Teardown();

	FString EffectiveSubjectName;
	ETryllMetaHumanLipSyncStatus Status = ETryllMetaHumanLipSyncStatus::NotStarted;

	TSharedPtr<FTryllMetaHumanLipSyncLiveLinkSource> Source;
	TSharedPtr<FTryllMetaHumanLipSyncSolver> Solver;
	FGuid SourceGuid;
	FDelegateHandle PcmHandle;

	FTryllVisemePlayer VisemePlayer;
	FDelegateHandle TurnStartedHandle;
	FDelegateHandle VisemesHandle;
};
