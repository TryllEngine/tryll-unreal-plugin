// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TryllVisemeEvent.h"
#include "TryllVisemePlayer.h"
#include "TryllLipSyncComponent.generated.h"

class UTryllSpeakerComponent;
class USkeletalMeshComponent;

/**
 * Moves a character's mouth with its voice. Plays the server's viseme track
 * (the agent's voice profile needs Emit Visemes on) on a face with
 * ARKit morph targets, in step with the audio the Tryll Speaker is playing.
 *
 * Only the morph targets its pose table names are written; everything else on
 * the mesh (blinks, brows, expressions) is left to the character's animation.
 * Written morph targets override the AnimBP's value for that name while
 * non-zero, and return to it at rest.
 *
 * Requires a UTryllSpeakerComponent: it is the audio, the playback clock and
 * the source of the viseme track.
 */
UCLASS(ClassGroup=(Tryll), meta=(BlueprintSpawnableComponent), DisplayName="Tryll Lip Sync")
class TRYLLCLIENT_API UTryllLipSyncComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTryllLipSyncComponent();

	/** Speaker whose TTS drives the face. Auto-discovered on the owner if unset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lip Sync")
	TObjectPtr<UTryllSpeakerComponent> Speaker;

	/** Mesh carrying the ARKit morph targets. If unset, the owner's skeletal meshes are searched for one that has them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lip Sync")
	TObjectPtr<USkeletalMeshComponent> FaceMesh;

	/** Look and sync. Live-editable in Play. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lip Sync", meta=(ShowOnlyInnerProperties))
	FTryllVisemeSettings Settings;

	/** Convenience for the most-used knob. */
	UFUNCTION(BlueprintCallable, Category="Tryll|Lip Sync")
	void SetIntensity(float InIntensity) { Settings.Intensity = InIntensity; }

	/** Pose per viseme, as ARKit channel weights. Empty uses the built-in table. Read at BeginPlay. */
	UPROPERTY(EditAnywhere, Category="Lip Sync|Mapping")
	TArray<FTryllVisemePose> Poses;

	/** Rebinds to a face mesh (e.g. after spawning or swapping it at runtime). */
	UFUNCTION(BlueprintCallable, Category="Tryll|Lip Sync")
	void BindFace(USkeletalMeshComponent* InFaceMesh);

	/** True once at least one morph target in the pose table was found on the face. */
	UFUNCTION(BlueprintPure, Category="Tryll|Lip Sync")
	bool IsFaceBound() const { return BoundChannels.Num() > 0; }

	/** Number of pose-table channels found as morph targets on the face. */
	UFUNCTION(BlueprintPure, Category="Tryll|Lip Sync")
	int32 GetBoundChannelCount() const { return BoundChannels.Num(); }

	UFUNCTION(BlueprintPure, Category="Tryll|Lip Sync")
	ETryllViseme GetActiveViseme() const { return Player.GetActiveViseme(); }

	/** jawOpen as written this frame (0..1). */
	UFUNCTION(BlueprintPure, Category="Tryll|Lip Sync")
	float GetAppliedJawOpen() const { return Player.GetCurve(TEXT("jawOpen")); }

	/** The player, for integrations that apply the curves themselves (e.g. a MetaHuman face). */
	const FTryllVisemePlayer& GetPlayer() const { return Player; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void HandleTurnStarted(int32 SampleRate, int64 TurnOriginSamples);
	void HandleVisemes(const FString& NodeName, TArrayView<const FTryllVisemeEvent> Events);
	USkeletalMeshComponent* FindFaceMesh() const;
	void ReleaseFace();

	FTryllVisemePlayer Player;

	/** Index into Player.GetChannels() of each channel the face has as a morph target. */
	TArray<int32> BoundChannels;

	FDelegateHandle TurnStartedHandle;
	FDelegateHandle VisemesHandle;
};
