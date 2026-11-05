// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

class TRYLLCLIENT_API FTryllTtsPlaybackEvents
{
public:
	void OnFormat();

	bool IsSuppressed() const { return bSuppressed; }

	/** Returns true when OnTtsAudioStarted should fire. */
	bool OnAudioQueued();

	void OnTurnComplete();

	bool IsAwaitingDrain() const { return bAwaitingDrain; }

	/** Returns true when OnTtsAudioFinished should fire. */
	bool OnQueueLevel(int64 QueuedBytes);

	/** Returns true when OnTtsAudioFinished should fire. */
	bool OnStop();

	void Reset();

private:
	bool bSpeaking      = false;
	bool bSuppressed    = false;
	bool bAwaitingDrain = false;
};
