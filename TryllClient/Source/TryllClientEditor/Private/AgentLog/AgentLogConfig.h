// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

/** Editor-user retention and layout defaults for the Agent Log window (Phase 4 UI). */
struct FAgentLogConfig
{
	static constexpr int32 DefaultMaxCompletedTurnsPerAgent   = 200;
	static constexpr int32 DefaultMaxObservationsPerAgent     = 4000;
	static constexpr int32 DefaultMaxRawSessionObservations   = 4000;
	static constexpr float DefaultAgentListWidth                = 220.0f;
	static constexpr float DefaultTimelineWidth                 = 480.0f;
	static constexpr float DefaultInspectorWidth                = 400.0f;

	static constexpr const TCHAR* IniSection = TEXT("TryllAgentLog");

	int32 MaxCompletedTurnsPerAgent = DefaultMaxCompletedTurnsPerAgent;
	int32 MaxObservationsPerAgent   = DefaultMaxObservationsPerAgent;
	int32 MaxRawSessionObservations = DefaultMaxRawSessionObservations;
	float AgentListWidth            = DefaultAgentListWidth;
	float TimelineWidth             = DefaultTimelineWidth;
	float InspectorWidth            = DefaultInspectorWidth;
	bool  bInspectorVisible           = false;
	bool  bCoalesceAnswerText         = true;
	/** Collapse completed turns the user has not manually expanded. */
	bool  bCollapseCompletedTurns     = true;

	static FAgentLogConfig Load();
	void                   Save() const;
};
