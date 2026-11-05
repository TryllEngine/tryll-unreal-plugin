// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllGraphDescription.h"
#include "TryllTurnInspectorModels.h"

class FJsonObject;

/** How an assistant-side chat turn should appear in the transcript. */
enum class ETryllChatTurnDisplay : uint8
{
	Streaming,
	Success,
	Cancelled,
	Error,
	/** Completed successfully but produced no visible assistant text. */
	Silent,
	/**
	 * A pause outlived its budget and the turn was abandoned. Presented like
	 * Cancelled (an interrupted turn) but labelled distinctly: "Cancelled" would
	 * imply somebody asked for it, when in fact Resume was simply never called.
	 */
	PauseTimedOut,
};

/** One retained chat turn. Package 1 keeps this chat-local — not a general event stream. */
struct FTryllChatTurnRecord
{
	FString TurnKey;
	int32   Sequence = 0;
	FString UserText;
	bool    bFromVoice = false;
	FString AssistantText;
	FString ErrorText;
	ETryllTurnStatus Status = ETryllTurnStatus::Success;
	TSharedPtr<FJsonObject> DebugInfo;
	FString DebugInfoRaw;
	int32 TokensGenerated = 0;
	int32 DurationMs = 0;
	int32 TtftMs = 0;
	ETryllChatTurnDisplay Display = ETryllChatTurnDisplay::Streaming;
	bool bComplete = false;
	FDateTime StartedUtc;
};

/**
 * Pure selection / finalization helpers for editor chat. Extracted so Unreal and tests
 * share one set of follow rules (R2).
 */
struct TRYLLCLIENTEDITOR_API FTryllChatTurnLogic
{
	static FString MakeTurnKey(int32 Sequence);

	static FTryllChatTurnRecord BeginTurn(int32 Sequence, const FString& UserText, bool bFromVoice);

	static void AppendAssistantDelta(FTryllChatTurnRecord& Turn, const FString& Text, bool bIsDelta);

	static void FinalizeTurn(
		FTryllChatTurnRecord& Turn,
		ETryllTurnStatus Status,
		const FString& DebugInfoRaw,
		int32 TokensGenerated,
		const FString& ExplicitErrorText = FString());

	static ETryllChatTurnDisplay ClassifyDisplay(
		ETryllTurnStatus Status, const FString& AssistantText);

	static FString StatusRowLabel(ETryllChatTurnDisplay Display);

	/** True when the assistant side should render a compact status row instead of answer text. */
	static bool UsesStatusRow(const FTryllChatTurnRecord& Turn);

	static ETryllTurnTerminalStatus ToTerminalStatus(ETryllTurnStatus Status);

	static ETryllDiagnosticsAbsenceReason FallbackAbsence(const FTryllChatTurnRecord& Turn);

	static FTryllTurnInspection ToInspection(const FTryllChatTurnRecord& Turn);

	static TArray<FTryllTurnRouteNode> ExtractRoute(const TSharedPtr<FJsonObject>& DebugInfo);

	/** Detailed diagnostics.errors[0] when available, otherwise the supplied fallback. */
	static FString ResolveErrorText(
		ETryllTurnStatus Status,
		const TSharedPtr<FJsonObject>& DebugInfo,
		const FString& ExplicitErrorText = FString());

	/** Admission rule shared by typed and voice turn starts. */
	static bool CanBeginTurn(bool bTurnInFlight, int32 InFlightTurnIndex);

	/**
	 * Finalizes the indexed turn and clears all in-flight state. Returns false when no current
	 * record existed, while still repairing the state flags.
	 */
	static bool SettleInFlightTurn(
		TArray<FTryllChatTurnRecord>& Turns,
		int32& InFlightTurnIndex,
		bool& bTurnInFlight,
		FString& SelectedTurnKey,
		ETryllTurnStatus Status,
		const FString& DebugInfoRaw,
		int32 TokensGenerated,
		const FString& ExplicitErrorText = FString());

	/** Compare-in-Lab captures only complete turns with real assistant output. */
	static bool ShouldCaptureForCompare(const FTryllChatTurnRecord& Turn);

	/**
	 * Selection-follow rules (call after Completed is already in Turns and finalized):
	 * 1. First completed answer becomes selected when nothing was selected.
	 * 2. If selection was the previous latest, advance to the new completion.
	 * 3. An older explicit selection is preserved.
	 */
	static FString OnTurnCompleted(
		const FString& SelectedKey,
		const TArray<FTryllChatTurnRecord>& Turns,
		const FTryllChatTurnRecord& Completed);

	/** True when SelectedKey is the newest completed turn (or empty). */
	static bool IsFollowingLatest(
		const FString& SelectedKey, const TArray<FTryllChatTurnRecord>& Turns);

	/** After reset/stop: clear selection when the selected record is gone. */
	static FString SanitizeSelection(
		const FString& SelectedKey, const TArray<FTryllChatTurnRecord>& Turns);
};
