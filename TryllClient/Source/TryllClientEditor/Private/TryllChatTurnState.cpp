// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllChatTurnState.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

FString FTryllChatTurnLogic::MakeTurnKey(const int32 Sequence)
{
	return FString::Printf(TEXT("chat:%d"), Sequence);
}

FTryllChatTurnRecord FTryllChatTurnLogic::BeginTurn(
	const int32 Sequence, const FString& UserText, const bool bFromVoice)
{
	FTryllChatTurnRecord Turn;
	Turn.TurnKey     = MakeTurnKey(Sequence);
	Turn.Sequence    = Sequence;
	Turn.UserText    = UserText;
	Turn.bFromVoice  = bFromVoice;
	Turn.Display     = ETryllChatTurnDisplay::Streaming;
	Turn.bComplete   = false;
	Turn.StartedUtc  = FDateTime::UtcNow();
	return Turn;
}

void FTryllChatTurnLogic::AppendAssistantDelta(
	FTryllChatTurnRecord& Turn, const FString& Text, const bool bIsDelta)
{
	if (Turn.bComplete)
	{
		return;
	}
	if (bIsDelta)
	{
		Turn.AssistantText += Text;
	}
	else
	{
		Turn.AssistantText = Text;
	}
}

void FTryllChatTurnLogic::FinalizeTurn(
	FTryllChatTurnRecord& Turn,
	const ETryllTurnStatus Status,
	const FString& DebugInfoRaw,
	const int32 TokensGenerated,
	const FString& ExplicitErrorText)
{
	Turn.Status           = Status;
	Turn.TokensGenerated  = TokensGenerated;
	Turn.DebugInfoRaw     = DebugInfoRaw;
	Turn.bComplete        = true;

	const FTimespan Wall = FDateTime::UtcNow() - Turn.StartedUtc;
	Turn.DurationMs = static_cast<int32>(Wall.GetTotalMilliseconds());

	Turn.DebugInfo.Reset();
	if (!DebugInfoRaw.IsEmpty())
	{
		TSharedPtr<FJsonObject> Parsed;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(DebugInfoRaw);
		if (FJsonSerializer::Deserialize(Reader, Parsed) && Parsed.IsValid())
		{
			Turn.DebugInfo = Parsed;

			double Seconds = 0.0;
			if (Parsed->TryGetNumberField(TEXT("duration_s"), Seconds))
			{
				Turn.DurationMs = static_cast<int32>(Seconds * 1000.0);
			}
			double Ttft = 0.0;
			if (Parsed->TryGetNumberField(TEXT("time_to_first_token_ms"), Ttft))
			{
				Turn.TtftMs = static_cast<int32>(Ttft);
			}
		}
	}

	Turn.ErrorText = ResolveErrorText(Status, Turn.DebugInfo, ExplicitErrorText);
	Turn.Display = ClassifyDisplay(Status, Turn.AssistantText);
}

ETryllChatTurnDisplay FTryllChatTurnLogic::ClassifyDisplay(
	const ETryllTurnStatus Status, const FString& AssistantText)
{
	// A pause that timed out is an *interrupted* turn — partial interaction kept,
	// exactly as with StopAndKeep — but it gets its own label, because "Cancelled"
	// would tell the reader somebody asked for it when nobody did.
	if (Status == ETryllTurnStatus::PauseTimedOut)
	{
		return ETryllChatTurnDisplay::PauseTimedOut;
	}
	if (Status == ETryllTurnStatus::Cancelled)
	{
		return ETryllChatTurnDisplay::Cancelled;
	}
	if (Status == ETryllTurnStatus::Error)
	{
		return ETryllChatTurnDisplay::Error;
	}
	if (AssistantText.IsEmpty())
	{
		return ETryllChatTurnDisplay::Silent;
	}
	return ETryllChatTurnDisplay::Success;
}

FString FTryllChatTurnLogic::StatusRowLabel(const ETryllChatTurnDisplay Display)
{
	switch (Display)
	{
	case ETryllChatTurnDisplay::Cancelled:     return TEXT("Cancelled");
	case ETryllChatTurnDisplay::PauseTimedOut: return TEXT("Pause timed out");
	case ETryllChatTurnDisplay::Error:         return TEXT("Error");
	case ETryllChatTurnDisplay::Silent:        return TEXT("No visible output");
	default:                                   return FString();
	}
}

bool FTryllChatTurnLogic::UsesStatusRow(const FTryllChatTurnRecord& Turn)
{
	return Turn.bComplete && Turn.AssistantText.IsEmpty();
}

ETryllTurnTerminalStatus FTryllChatTurnLogic::ToTerminalStatus(const ETryllTurnStatus Status)
{
	switch (Status)
	{
	case ETryllTurnStatus::Error:
		return ETryllTurnTerminalStatus::Error;
	case ETryllTurnStatus::Cancelled:
	case ETryllTurnStatus::PauseTimedOut:
		return ETryllTurnTerminalStatus::Cancelled;
	default:
		return ETryllTurnTerminalStatus::Success;
	}
}

ETryllDiagnosticsAbsenceReason FTryllChatTurnLogic::FallbackAbsence(
	const FTryllChatTurnRecord& /*Turn*/)
{
	// Chat creates its agent with diagnostics enabled, so absence is never merely "unavailable".
	return ETryllDiagnosticsAbsenceReason::Empty;
}

FTryllTurnInspection FTryllChatTurnLogic::ToInspection(const FTryllChatTurnRecord& Turn)
{
	if (!Turn.bComplete)
	{
		return FTryllTurnInspectionFactory::Empty(TEXT("select an answer"));
	}

	const TArray<FTryllTurnRouteNode> Route = ExtractRoute(Turn.DebugInfo);
	const FString Hint = FString::Printf(
		TEXT("turn %d%s"), Turn.Sequence, Turn.bFromVoice ? TEXT(" · voice") : TEXT(""));

	return FTryllTurnInspectionFactory::FromChatTurn(
		Turn.TurnKey,
		Hint,
		Turn.AssistantText,
		Turn.ErrorText,
		ToTerminalStatus(Turn.Status),
		Route,
		Turn.DurationMs,
		Turn.TtftMs,
		Turn.DebugInfo,
		Turn.DebugInfoRaw,
		FallbackAbsence(Turn));
}

TArray<FTryllTurnRouteNode> FTryllChatTurnLogic::ExtractRoute(
	const TSharedPtr<FJsonObject>& DebugInfo)
{
	TArray<FTryllTurnRouteNode> Route;
	if (!DebugInfo.IsValid())
	{
		return Route;
	}

	const TArray<TSharedPtr<FJsonValue>>* Nodes = nullptr;
	if (!DebugInfo->TryGetArrayField(TEXT("nodes"), Nodes) || Nodes == nullptr)
	{
		return Route;
	}

	Route.Reserve(Nodes->Num());
	for (const TSharedPtr<FJsonValue>& Value : *Nodes)
	{
		const TSharedPtr<FJsonObject>* NodeObj = nullptr;
		if (!Value.IsValid() || !Value->TryGetObject(NodeObj) || NodeObj == nullptr || !NodeObj->IsValid())
		{
			continue;
		}

		FTryllTurnRouteNode Node;
		(*NodeObj)->TryGetStringField(TEXT("name"), Node.Name);
		(*NodeObj)->TryGetStringField(TEXT("_type"), Node.Type);
		(*NodeObj)->TryGetStringField(TEXT("exit_route"), Node.Exit);

		double Seconds = 0.0;
		if ((*NodeObj)->TryGetNumberField(TEXT("duration_s"), Seconds))
		{
			Node.Ms = static_cast<int32>(Seconds * 1000.0);
		}
		Route.Add(MoveTemp(Node));
	}
	return Route;
}

FString FTryllChatTurnLogic::ResolveErrorText(
	const ETryllTurnStatus Status,
	const TSharedPtr<FJsonObject>& DebugInfo,
	const FString& ExplicitErrorText)
{
	if (Status == ETryllTurnStatus::PauseTimedOut)
	{
		// Say why the turn stopped: "Cancelled" alone would imply someone asked.
		return TEXT("Pause timed out — the turn was abandoned because Resume was "
		            "never called. Bind OnPaused (or register a tool handler), or "
		            "raise the node's Pause Timeout Ms.");
	}
	if (Status != ETryllTurnStatus::Error)
	{
		return FString();
	}
	if (!ExplicitErrorText.IsEmpty())
	{
		return ExplicitErrorText;
	}

	if (DebugInfo.IsValid())
	{
		const TArray<TSharedPtr<FJsonValue>>* Errors = nullptr;
		if (DebugInfo->TryGetArrayField(TEXT("errors"), Errors)
			&& Errors != nullptr
			&& Errors->Num() > 0
			&& (*Errors)[0].IsValid())
		{
			const TSharedPtr<FJsonValue>& First = (*Errors)[0];
			if (First->Type == EJson::String)
			{
				const FString Message = First->AsString();
				if (!Message.IsEmpty())
				{
					return Message;
				}
			}
			else if (First->Type == EJson::Object)
			{
				const TSharedPtr<FJsonObject>& ErrorObject = First->AsObject();
				FString Message;
				if (ErrorObject.IsValid()
					&& (ErrorObject->TryGetStringField(TEXT("message"), Message)
						|| ErrorObject->TryGetStringField(TEXT("error"), Message)
						|| ErrorObject->TryGetStringField(TEXT("detail"), Message))
					&& !Message.IsEmpty())
				{
					return Message;
				}
			}
		}
	}
	return TEXT("Turn failed");
}

bool FTryllChatTurnLogic::CanBeginTurn(
	const bool bTurnInFlight, const int32 InFlightTurnIndex)
{
	return !bTurnInFlight && InFlightTurnIndex == INDEX_NONE;
}

bool FTryllChatTurnLogic::SettleInFlightTurn(
	TArray<FTryllChatTurnRecord>& Turns,
	int32& InFlightTurnIndex,
	bool& bTurnInFlight,
	FString& SelectedTurnKey,
	const ETryllTurnStatus Status,
	const FString& DebugInfoRaw,
	const int32 TokensGenerated,
	const FString& ExplicitErrorText)
{
	if (!Turns.IsValidIndex(InFlightTurnIndex))
	{
		InFlightTurnIndex = INDEX_NONE;
		bTurnInFlight = false;
		return false;
	}

	FTryllChatTurnRecord& Turn = Turns[InFlightTurnIndex];
	FinalizeTurn(Turn, Status, DebugInfoRaw, TokensGenerated, ExplicitErrorText);
	SelectedTurnKey = OnTurnCompleted(SelectedTurnKey, Turns, Turn);
	InFlightTurnIndex = INDEX_NONE;
	bTurnInFlight = false;
	return true;
}

bool FTryllChatTurnLogic::ShouldCaptureForCompare(const FTryllChatTurnRecord& Turn)
{
	return Turn.bComplete && !Turn.AssistantText.IsEmpty();
}

FString FTryllChatTurnLogic::OnTurnCompleted(
	const FString& SelectedKey,
	const TArray<FTryllChatTurnRecord>& Turns,
	const FTryllChatTurnRecord& Completed)
{
	if (!Completed.bComplete)
	{
		return SelectedKey;
	}
	if (SelectedKey.IsEmpty())
	{
		return Completed.TurnKey;
	}

	FString PreviousLatest;
	for (int32 Index = Turns.Num() - 1; Index >= 0; --Index)
	{
		const FTryllChatTurnRecord& Turn = Turns[Index];
		if (!Turn.bComplete)
		{
			continue;
		}
		if (Turn.TurnKey == Completed.TurnKey)
		{
			continue;
		}
		PreviousLatest = Turn.TurnKey;
		break;
	}

	if (PreviousLatest.IsEmpty() || SelectedKey == PreviousLatest)
	{
		return Completed.TurnKey;
	}
	return SelectedKey;
}

bool FTryllChatTurnLogic::IsFollowingLatest(
	const FString& SelectedKey, const TArray<FTryllChatTurnRecord>& Turns)
{
	if (SelectedKey.IsEmpty() || Turns.Num() == 0)
	{
		return true;
	}

	for (int32 Index = Turns.Num() - 1; Index >= 0; --Index)
	{
		if (Turns[Index].bComplete)
		{
			return Turns[Index].TurnKey == SelectedKey;
		}
	}
	return true;
}

FString FTryllChatTurnLogic::SanitizeSelection(
	const FString& SelectedKey, const TArray<FTryllChatTurnRecord>& Turns)
{
	if (SelectedKey.IsEmpty())
	{
		return FString();
	}

	for (const FTryllChatTurnRecord& Turn : Turns)
	{
		if (Turn.TurnKey == SelectedKey)
		{
			return SelectedKey;
		}
	}
	return FString();
}
