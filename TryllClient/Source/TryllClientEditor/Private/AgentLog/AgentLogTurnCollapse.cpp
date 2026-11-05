// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "AgentLog/AgentLogTurnCollapse.h"

namespace AgentLogTurnCollapsePrivate
{

FAgentLogRow Annotate(
	const FAgentLogRow& Source,
	const bool bCanCollapse,
	const bool bCollapsed,
	const int32 HiddenBody)
{
	FAgentLogRow Out = Source;
	Out.bCanCollapseTurn = bCanCollapse;
	Out.bIsTurnCollapsed = bCollapsed;
	Out.HiddenBodyCount  = HiddenBody;
	if (bCollapsed && HiddenBody > 0 && !Out.OverrideSummary.IsEmpty())
	{
		Out.OverrideSummary = FString::Printf(
			TEXT("%s  ·  %d events"), *Out.OverrideSummary, HiddenBody + 1);
	}
	return Out;
}

} // namespace AgentLogTurnCollapsePrivate

TArray<FAgentLogRow> FAgentLogTurnCollapse::Apply(
	const TArray<FAgentLogRow>& Rows,
	const FAgentLogTurnCollapseState& State,
	const bool bCollapseCompletedTurns)
{
	using namespace AgentLogTurnCollapsePrivate;

	TArray<FAgentLogRow> Output;
	if (Rows.Num() == 0)
	{
		return Output;
	}

	int32 Index = 0;
	while (Index < Rows.Num())
	{
		const FAgentLogRow& Row = Rows[Index];
		if (!Row.TurnKey.bHasValue)
		{
			Output.Add(Annotate(Row, false, false, 0));
			++Index;
			continue;
		}

		const int32 Start = Index;
		int32 End = Index + 1;
		while (End < Rows.Num()
			&& Rows[End].TurnKey.bHasValue
			&& Rows[End].TurnKey == Row.TurnKey)
		{
			++End;
		}

		const int32 Count = End - Start;
		bool bComplete = false;
		for (int32 i = Start; i < End; ++i)
		{
			if (Rows[i].bIsTurnTerminal)
			{
				bComplete = true;
				break;
			}
		}

		const bool bCanCollapse = Count > 1;
		const bool bCollapsed = bCanCollapse
			&& State.IsCollapsed(Row.TurnKey, bComplete, bCollapseCompletedTurns);

		if (!bCanCollapse)
		{
			Output.Add(Annotate(Rows[Start], false, false, 0));
		}
		else if (bCollapsed)
		{
			Output.Add(Annotate(Rows[Start], true, true, Count - 1));
		}
		else
		{
			Output.Add(Annotate(Rows[Start], true, false, 0));
			for (int32 i = Start + 1; i < End; ++i)
			{
				Output.Add(Annotate(Rows[i], false, false, 0));
			}
		}

		Index = End;
	}

	return Output;
}
