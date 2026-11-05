// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "AgentLog/AgentLogProjection.h"
#include "TryllObservationTypes.h"

enum class EAgentLogTurnCollapseOverride : uint8
{
	None = 0,
	ForcedExpanded = 1,
	ForcedCollapsed = 2,
};

/**
 * Session-local per-turn collapse overrides. "Collapse completed turns" only
 * affects turns the user has not manually toggled.
 */
class FAgentLogTurnCollapseState
{
public:
	void Clear() { Overrides.Empty(); }

	EAgentLogTurnCollapseOverride GetOverride(const FTryllTurnKey& Key) const
	{
		if (!Key.bHasValue)
		{
			return EAgentLogTurnCollapseOverride::None;
		}
		if (const EAgentLogTurnCollapseOverride* Found = Overrides.Find(Key))
		{
			return *Found;
		}
		return EAgentLogTurnCollapseOverride::None;
	}

	bool IsCollapsed(const FTryllTurnKey& Key, const bool bTurnComplete, const bool bCollapseCompletedTurns) const
	{
		if (!Key.bHasValue)
		{
			return false;
		}
		switch (GetOverride(Key))
		{
		case EAgentLogTurnCollapseOverride::ForcedExpanded:
			return false;
		case EAgentLogTurnCollapseOverride::ForcedCollapsed:
			return true;
		default:
			return bCollapseCompletedTurns && bTurnComplete;
		}
	}

	void Toggle(const FTryllTurnKey& Key, const bool bCurrentlyCollapsed)
	{
		if (!Key.bHasValue)
		{
			return;
		}
		Overrides.Add(
			Key,
			bCurrentlyCollapsed
				? EAgentLogTurnCollapseOverride::ForcedExpanded
				: EAgentLogTurnCollapseOverride::ForcedCollapsed);
	}

private:
	TMap<FTryllTurnKey, EAgentLogTurnCollapseOverride> Overrides;
};

struct TRYLLCLIENTEDITOR_API FAgentLogTurnCollapse
{
	/** Keep headers of collapsed turns; hide body rows; annotate with event count. */
	static TArray<FAgentLogRow> Apply(
		const TArray<FAgentLogRow>& Rows,
		const FAgentLogTurnCollapseState& State,
		bool bCollapseCompletedTurns);
};
