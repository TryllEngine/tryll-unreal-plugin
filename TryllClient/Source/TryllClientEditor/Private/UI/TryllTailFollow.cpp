// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "UI/TryllTailFollow.h"

FTryllTailFollow::FTryllTailFollow(TFunction<float()> DistanceFromBottom, TFunction<void()> ScrollToEnd)
	: DistanceFromBottomFn(MoveTemp(DistanceFromBottom))
	, ScrollToEndFn(MoveTemp(ScrollToEnd))
{
}

float FTryllTailFollow::DistanceFromBottom(
	const float ContentHeight, const float ViewportHeight, const float OffsetY)
{
	if (ContentHeight <= ViewportHeight)
	{
		return 0.0f;
	}
	return FMath::Max(0.0f, ContentHeight - ViewportHeight - OffsetY);
}

void FTryllTailFollow::NotifyUserScroll()
{
	if (bSuppressUserScroll || !DistanceFromBottomFn)
	{
		return;
	}
	Controller.OnUserScroll(DistanceFromBottomFn(), FTryllEditorUiContract::FollowTailThreshold);
}

void FTryllTailFollow::NotifyAppend()
{
	if (Controller.OnAppend() && ScrollToEndFn)
	{
		ScrollToEndFn();
	}
}

void FTryllTailFollow::ResetFollowing()
{
	Controller.ResetFollowing();
	if (ScrollToEndFn)
	{
		ScrollToEndFn();
	}
}

void FTryllTailFollow::BeginProgrammaticScroll()
{
	bSuppressUserScroll = true;
}

void FTryllTailFollow::EndProgrammaticScroll()
{
	bSuppressUserScroll = false;
}
