// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllNodeEvent.h"

namespace
{
const FString* FindWireValue(const TArray<TPair<FString, FString>>& WirePairs, const TCHAR* Key)
{
	for (const TPair<FString, FString>& Kv : WirePairs)
	{
		if (Kv.Key == Key)
		{
			return &Kv.Value;
		}
	}
	return nullptr;
}

FString WireValue(const TArray<TPair<FString, FString>>& WirePairs, const TCHAR* Key)
{
	if (const FString* Found = FindWireValue(WirePairs, Key))
	{
		return *Found;
	}
	return FString();
}
} // namespace

FTryllDecodedNodeEvent TryllDecodeNodeEvent(
	const FString& NodeName,
	const FString& EventType,
	const TArray<TPair<FString, FString>>& WirePairs)
{
	FTryllDecodedNodeEvent Out;
	Out.NodeName = NodeName;
	Out.EventType = EventType;
	Out.KvPairs.Reserve(WirePairs.Num());
	for (const TPair<FString, FString>& Kv : WirePairs)
	{
		FTryllNodeEventKeyValue Item;
		Item.Key = Kv.Key;
		Item.Value = Kv.Value;
		Out.KvPairs.Add(MoveTemp(Item));
	}

	if (EventType == TEXT("tool_call"))
	{
		Out.Kind = ETryllNodeEventKind::ToolCall;
		Out.ToolCall.CallId = WireValue(WirePairs, TEXT("call_id"));
		Out.ToolCall.NodeName = NodeName;
		Out.ToolCall.ToolName = WireValue(WirePairs, TEXT("tool_name"));
		Out.ToolCall.ArgumentsJson = WireValue(WirePairs, TEXT("arguments_json"));
		Out.ToolCall.Disposition = WireValue(WirePairs, TEXT("disposition"));
	}
	else if (EventType == TEXT("intent_classified"))
	{
		Out.Kind = ETryllNodeEventKind::IntentClassified;
		Out.Intent.NodeName = NodeName;
		Out.Intent.Intent = WireValue(WirePairs, TEXT("intent"));
		Out.Intent.RecordId = WireValue(WirePairs, TEXT("record_id"));
		Out.Intent.RecordIndex = FCString::Atoi64(*WireValue(WirePairs, TEXT("record_index")));
		Out.Intent.Distance = FCString::Atof(*WireValue(WirePairs, TEXT("distance")));
		Out.Intent.NotFoundReason = WireValue(WirePairs, TEXT("not_found_reason"));
	}
	else if (EventType == TEXT("intent_llm_classified"))
	{
		Out.Kind = ETryllNodeEventKind::IntentLlmClassified;
		Out.IntentLlm.NodeName = NodeName;
		Out.IntentLlm.Intent = WireValue(WirePairs, TEXT("intent"));
		Out.IntentLlm.TopProb = FCString::Atof(*WireValue(WirePairs, TEXT("top_prob")));
		Out.IntentLlm.SecondProb = FCString::Atof(*WireValue(WirePairs, TEXT("second_prob")));
		Out.IntentLlm.NotFoundReason = WireValue(WirePairs, TEXT("not_found_reason"));
		for (const TPair<FString, FString>& Kv : WirePairs)
		{
			if (Kv.Key.EndsWith(TEXT(".prob"))
				&& Kv.Key != TEXT("top_prob")
				&& Kv.Key != TEXT("second_prob"))
			{
				FTryllIntentLlmLabelProb Entry;
				Entry.Label = Kv.Key.LeftChop(5);
				Entry.Prob = FCString::Atof(*Kv.Value);
				Out.IntentLlm.LabelProbs.Add(MoveTemp(Entry));
			}
		}
	}
	else if (EventType == TEXT("paused"))
	{
		Out.Kind = ETryllNodeEventKind::Paused;
		Out.PendingExit = WireValue(WirePairs, TEXT("exit_route"));
	}

	return Out;
}
