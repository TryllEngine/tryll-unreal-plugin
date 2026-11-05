// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllChatConfig.generated.h"

/**
 * Persisted UI state for the Tryll Chat panel.
 *
 * Separate from UTryllDialogLabConfig — chat and Lab are independent panels with their own
 * layout preferences. Voice / STT choices live here (not on UTryllRuntimeSettings) because
 * they are editor-tool UX and the available STT set is machine-specific.
 */
UCLASS(config = EditorPerProjectUserSettings)
class UTryllChatConfig : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(config)
	float InspectorWidth = 400.0f;

	UPROPERTY(config)
	bool bInspectorVisible = true;

	/** Legacy checkbox. Read only when VoiceSource is still Unset (-1). */
	UPROPERTY(config)
	bool bEnableVoice = false;

	/**
	 * ETryllChatVoiceSource as int. -1 means this config predates the selector;
	 * load maps bEnableVoice (true → Chat).
	 */
	UPROPERTY(config)
	int32 VoiceSource = -1;

	/** Last selected registered STT catalog name. Empty until voice is enabled once. */
	UPROPERTY(config)
	FString SttModelName;

	/** 0 = Push-to-talk, 1 = Hands-free. Matches STryllChatPanel::EVoiceMode. */
	UPROPERTY(config)
	int32 VoiceMode = 0;
};
