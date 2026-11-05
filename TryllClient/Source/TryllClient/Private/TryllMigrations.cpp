// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllMigrations.h"
#include "TryllAssetVersion.h"
#include "Generated/Nodes/TryllCannedResponseParams.h"
#include "Generated/Nodes/TryllClassifyIntentLLMParams.h"
#include "Generated/Nodes/TryllIntentToInstructionParams.h"
#include "TryllClientModule.h"

namespace
{
TArray<FString> SplitLegacyCsv(const FString& Csv)
{
	TArray<FString> Pieces;
	Csv.ParseIntoArray(Pieces, TEXT(","), false);
	TArray<FString> Out;
	for (FString& Piece : Pieces)
	{
		Piece.TrimStartAndEndInline();
		if (!Piece.IsEmpty())
			Out.Add(MoveTemp(Piece));
	}
	return Out;
}

bool MoveCsvIntents(UTryllClassifyIntentLLMParams& Params)
{
	const TArray<FString> Ids = SplitLegacyCsv(Params.IntentsIds_DEPRECATED);
	const TArray<FString> Prompts = SplitLegacyCsv(Params.IntentsPrompt_DEPRECATED);
	if (Params.Intents.Num() > 0 || (Ids.IsEmpty() && Prompts.IsEmpty()))
		return false;

	if (Ids.Num() != Prompts.Num())
	{
		UE_LOG(LogTryll, Warning,
			TEXT("%s: old intents_ids has %d entries but intents_prompt has %d. Moved them onto Intents "
				 "with empty text where one side was missing; fill those in before use."),
			*Params.GetPathName(), Ids.Num(), Prompts.Num());
	}

	const int32 Count = FMath::Max(Ids.Num(), Prompts.Num());
	Params.Intents.Reserve(Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FTryllClassifyIntentEntry& Entry = Params.Intents.AddDefaulted_GetRef();
		Entry.Id = Ids.IsValidIndex(Index) ? Ids[Index] : FString();
		Entry.Prompt = Prompts.IsValidIndex(Index) ? Prompts[Index] : FString();
	}
	return true;
}

bool MoveLegacyStrings(TArray<FTryllLocalizableText>& Dest, const TArray<FString>& Source)
{
	if (Dest.Num() > 0 || Source.IsEmpty())
		return false;
	Dest.Reserve(Source.Num());
	for (const FString& Text : Source)
	{
		FTryllLocalizableText& Line = Dest.AddDefaulted_GetRef();
		Line.Text = Text;
	}
	return true;
}
}

void TryllApplyOldDefaults(UTryllNodeParamsBase& Params, int32 SavedVersion)
{
}

bool TryllUpgradeParams(UTryllNodeParamsBase& Params, int32 SavedVersion)
{
	bool bChanged = false;
	if (SavedVersion < FTryllAssetVersion::ClassifyIntentLLMIntentsList)
		if (auto* P = Cast<UTryllClassifyIntentLLMParams>(&Params))
			bChanged |= MoveCsvIntents(*P);
	if (SavedVersion < FTryllAssetVersion::LocalizableInlineText)
	{
		if (auto* Canned = Cast<UTryllCannedResponseParams>(&Params))
			bChanged |= MoveLegacyStrings(Canned->Lines, Canned->InlineStrings_DEPRECATED);
		if (auto* Intent = Cast<UTryllIntentToInstructionParams>(&Params))
			bChanged |= MoveLegacyStrings(Intent->Instructions, Intent->InlineStrings_DEPRECATED);
	}
	return bChanged;
}
