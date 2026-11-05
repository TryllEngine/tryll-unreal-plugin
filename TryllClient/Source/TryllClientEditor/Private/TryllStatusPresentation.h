// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllEditorStatus.h"

enum class ETryllPanelOperation : uint8
{
	None,
	CreatingAgent,
	CreatingVoiceInput,
	Refreshing,
	Listing,
	Waiting,
	Capturing,
};

enum class ETryllEditorPanel : uint8
{
	Chat,
	ModelManager,
	Memory,
	AgentLog,
	DialogLab,
	Knowledge,
};

enum class ETryllStatusSeverity : uint8
{
	Neutral,
	Progress,
	Success,
	Warning,
	Error,
};

enum class ETryllStatusActionIntent : uint8
{
	None,
	Retry,
	Start,
	Stop,
};

/**
 * Pure owner-phase + panel-operation status projection. Phase 8 wires this to
 * real window presenters; Phase 1 only characterizes the concurrent captions.
 */
struct TRYLLCLIENTEDITOR_API FTryllStatusPresentation
{
	ETryllStatusSeverity Severity = ETryllStatusSeverity::Neutral;
	FString Caption;
	FString Detail;
	ETryllStatusActionIntent Action = ETryllStatusActionIntent::None;

	static FTryllStatusPresentation Compose(
		ETryllOwnerPhase OwnerPhase,
		const FString& OwnerDetail,
		ETryllPanelOperation LocalOp,
		ETryllEditorPanel Panel,
		int32 ModelCount = 0);
};
