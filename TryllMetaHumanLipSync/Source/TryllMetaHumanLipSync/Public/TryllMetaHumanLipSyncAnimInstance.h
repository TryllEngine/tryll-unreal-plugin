// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "LiveLinkTypes.h"
#include "Animation/PoseAsset.h"

#include "TryllMetaHumanLipSyncAnimInstance.generated.h"

UCLASS()
class TRYLLMETAHUMANLIPSYNC_API UTryllMetaHumanLipSyncAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly, Category="Tryll MetaHuman Lip Sync")
	FLiveLinkSubjectName Subject;

	/**
	 * True when the subject carries ARKit curves from the viseme solver rather
	 * than MetaHuman rig controls from the neural solver. The AnimBP routes the
	 * ARKit curves through ArkitMapping (a Pose Blend node) to reach the rig.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Tryll MetaHuman Lip Sync")
	bool bVisemeSolver = false;

	/** ARKit → MetaHuman pose asset for the viseme path (Epic's PA_MetaHuman_ARKit_Mapping). */
	UPROPERTY(BlueprintReadOnly, Category="Tryll MetaHuman Lip Sync")
	TObjectPtr<UPoseAsset> ArkitMapping;
};
