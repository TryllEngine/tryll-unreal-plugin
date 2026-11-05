// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

/** Where Tryll Chat gets speech-to-text. Persisted as an int on UTryllChatConfig. */
enum class ETryllChatVoiceSource : uint8
{
	Off = 0,
	/** Chat creates its own STT handle and captures the microphone. */
	Chat = 1,
	/** Use the voice component on the selected actor (settings in Unreal, the live component in Unity). */
	Component = 2,
};

/**
 * Pure Voice Input selector rules. Extracted so the panel and tests share one
 * migration and availability check.
 */
struct TRYLLCLIENTEDITOR_API FTryllChatVoiceSource
{
	/** Saved value meaning "this build has never written VoiceSource". */
	static constexpr int32 Unset = -1;

	/** Old checkbox: on becomes Chat, off stays Off. A component assignment is not stored on Unreal. */
	[[nodiscard]] static ETryllChatVoiceSource FromLegacy(bool bEnableVoice);

	[[nodiscard]] static bool IsComponentAvailable(int32 NumVoiceComps);

	/** Out-of-range values, including Unset, become Off. Load migrates Unset before calling this. */
	[[nodiscard]] static ETryllChatVoiceSource Clamp(int32 Saved);

	[[nodiscard]] static int32 ToInt(ETryllChatVoiceSource Source);
};
