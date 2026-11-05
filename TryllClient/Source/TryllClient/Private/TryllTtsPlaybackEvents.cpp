// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllTtsPlaybackEvents.h"

void FTryllTtsPlaybackEvents::OnFormat()
{
	bSuppressed = false;
}

bool FTryllTtsPlaybackEvents::OnAudioQueued()
{
	bAwaitingDrain = false;
	if (bSpeaking)
	{
		return false;
	}
	bSpeaking = true;
	return true;
}

void FTryllTtsPlaybackEvents::OnTurnComplete()
{
	bAwaitingDrain = bSpeaking;
}

bool FTryllTtsPlaybackEvents::OnQueueLevel(int64 QueuedBytes)
{
	if (!bAwaitingDrain || QueuedBytes > 0)
	{
		return false;
	}
	bAwaitingDrain = false;
	bSpeaking      = false;
	return true;
}

bool FTryllTtsPlaybackEvents::OnStop()
{
	bSuppressed    = true;
	bAwaitingDrain = false;
	if (!bSpeaking)
	{
		return false;
	}
	bSpeaking = false;
	return true;
}

void FTryllTtsPlaybackEvents::Reset()
{
	bSpeaking      = false;
	bSuppressed    = false;
	bAwaitingDrain = false;
}
