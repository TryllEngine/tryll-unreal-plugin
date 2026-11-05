// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

/**
 * Scroll follow-tail state machine. Selection follow stays on
 * FTryllChatTurnLogic::IsFollowingLatest. Host adapters live in FTryllTailFollow.
 */
struct TRYLLCLIENTEDITOR_API FTryllTailFollowController
{
	enum class EFollowState : uint8
	{
		Following,
		Suspended,
	};

	EFollowState State = EFollowState::Following;

	/** Never requests a programmatic scroll — the user is already moving the view. */
	bool OnUserScroll(float DistanceFromBottom, float Threshold);

	/** Requests scroll-to-end only while already following. */
	bool OnAppend() const;

	/** User started a new stream (send, agent switch). Resume following. */
	void ResetFollowing();
};
