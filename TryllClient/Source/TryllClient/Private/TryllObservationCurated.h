// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "messages_generated.h"

// Curated Agent Log messages that scope inference does not explain.
// TryllMessageDescriptors.gen.h supplies name, direction, and agent_id /
// voice_input_id. CreateAgent has no agent id on the request, and
// BeginUtterance is voice-scoped but still curated.
inline bool TryllObservationIsCuratedOverride(::Tryll::MessageBody Body)
{
	switch (Body)
	{
	case ::Tryll::MessageBody_CreateAgentRequest:
	case ::Tryll::MessageBody_CreateAgentResponse:
	case ::Tryll::MessageBody_BeginUtteranceRequest:
		return true;
	default:
		return false;
	}
}
