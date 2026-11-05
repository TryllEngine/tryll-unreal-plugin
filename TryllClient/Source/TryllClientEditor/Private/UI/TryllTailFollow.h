// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllEditorUiContract.h"
#include "TryllTailFollowController.h"

/**
 * Host adapter for FTryllTailFollowController. Chat and Agent Log inject
 * measure + scroll-to-end; this type owns suppress and the contract threshold.
 */
struct TRYLLCLIENTEDITOR_API FTryllTailFollow
{
	FTryllTailFollow() = default;

	FTryllTailFollow(TFunction<float()> DistanceFromBottom, TFunction<void()> ScrollToEnd);

	FTryllTailFollowController::EFollowState GetState() const { return Controller.State; }

	static float DistanceFromBottom(float ContentHeight, float ViewportHeight, float OffsetY);

	void NotifyUserScroll();
	void NotifyAppend();
	void ResetFollowing();
	void BeginProgrammaticScroll();
	void EndProgrammaticScroll();

private:
	FTryllTailFollowController Controller;
	TFunction<float()> DistanceFromBottomFn;
	TFunction<void()> ScrollToEndFn;
	bool bSuppressUserScroll = false;
};
