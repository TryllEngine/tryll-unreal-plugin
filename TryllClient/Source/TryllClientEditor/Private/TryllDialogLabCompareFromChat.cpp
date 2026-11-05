// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogLabCompareFromChat.h"

#include "TryllDialogScriptAsset.h"

const TCHAR* FTryllDialogLabCompareFromChat::ScriptName = TEXT("Captured from Chat");
const TCHAR* FTryllDialogLabCompareFromChat::ScriptDescription =
	TEXT("Captured via 'Compare in Lab' from Tryll Chat.");

void FTryllDialogLabCompareFromChat::ApplyToAsset(
	UTryllDialogScriptAsset* Asset, const TArray<FTryllChatHistoryEntry>& History)
{
	if (Asset == nullptr)
	{
		return;
	}

	Asset->Description = ScriptDescription;
	// A captured script carries no global checks; clear rather than inherit whatever the
	// asset instance happened to hold.
	Asset->GlobalChecks.Reset();
	Asset->Steps.Reset();

	int32 Pairs = 0;
	for (int32 Index = 0; Index + 1 < History.Num(); Index += 2)
	{
		// Strict alternation: the moment it breaks, stop. A trailing unanswered user line must not
		// become a seeded step with no assistant reply — §3.6 would then refuse the whole run.
		if (!History[Index].bIsUser || History[Index + 1].bIsUser)
		{
			break;
		}
		FTryllDialogScriptStep Step;
		Step.User      = History[Index].Text;
		Step.Assistant = History[Index + 1].Text;
		Asset->Steps.Add(Step);
		++Pairs;
	}

	Asset->SeedTurnCount = Pairs;
	// One blank generated step, so Run has work to do rather than refusing "zero generated steps".
	Asset->Steps.Add(FTryllDialogScriptStep());
}
