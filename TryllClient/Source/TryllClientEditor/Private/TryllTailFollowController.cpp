// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllTailFollowController.h"

bool FTryllTailFollowController::OnUserScroll(const float DistanceFromBottom, const float Threshold)
{
	State = DistanceFromBottom <= Threshold
		? EFollowState::Following
		: EFollowState::Suspended;
	return false;
}

bool FTryllTailFollowController::OnAppend() const
{
	return State == EFollowState::Following;
}

void FTryllTailFollowController::ResetFollowing()
{
	State = EFollowState::Following;
}
