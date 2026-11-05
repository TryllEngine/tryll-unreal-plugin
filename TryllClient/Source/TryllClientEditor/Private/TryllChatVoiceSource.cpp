// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllChatVoiceSource.h"

ETryllChatVoiceSource FTryllChatVoiceSource::FromLegacy(bool bEnableVoice)
{
	return bEnableVoice ? ETryllChatVoiceSource::Chat : ETryllChatVoiceSource::Off;
}

bool FTryllChatVoiceSource::IsComponentAvailable(int32 NumVoiceComps)
{
	return NumVoiceComps > 0;
}

ETryllChatVoiceSource FTryllChatVoiceSource::Clamp(int32 Saved)
{
	if (Saved < static_cast<int32>(ETryllChatVoiceSource::Off)
		|| Saved > static_cast<int32>(ETryllChatVoiceSource::Component))
	{
		return ETryllChatVoiceSource::Off;
	}
	return static_cast<ETryllChatVoiceSource>(Saved);
}

int32 FTryllChatVoiceSource::ToInt(ETryllChatVoiceSource Source)
{
	return static_cast<int32>(Source);
}
