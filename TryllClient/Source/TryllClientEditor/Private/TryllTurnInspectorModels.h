// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllDialogLabChecks.h"

class FJsonObject;
struct FTryllDialogLabDataset;
struct FTryllLabCellView;

/** Terminal outcome shared by Lab and Chat without importing either native status enum. */
enum class ETryllTurnTerminalStatus : uint8
{
	Success,
	Cancelled,
	Error,
	Skipped,
};

/** Why a turn carries no usable schema-v2 diagnostics. */
enum class ETryllDiagnosticsAbsenceReason : uint8
{
	None,
	/** Injected with AppendInteractions — the graph never ran. */
	Seeded,
	/** Earlier failure skipped this turn. */
	Skipped,
	/** Agent was created without enable_diagnostics, or payload is empty. */
	Unavailable,
	/** Agent existed before the first diagnostics capture lease was acquired. */
	CreatedBeforeDiagnosticsCapture,
	/** Payload present but schema_version is missing or not 2. */
	UnsupportedSchema,
	/** Diagnostics expected but empty/malformed in a non-schema way. */
	Empty,
};

/** One node on the turn's route breadcrumb. */
struct FTryllTurnRouteNode
{
	FString Name;
	FString Type;
	FString Exit;
	int32   Ms = 0;
};

/** Forward: DiffPeer is a shared pointer so Lab context can nest a peer turn. */
struct FTryllTurnInspection;

/** Lab-only overlay. Omit entirely for chat. */
struct FTryllTurnLabContext
{
	FString PinA;
	FString PinB;
	TFunction<void(const FString&)> OnClearPin;

	TArray<FTryllLabCheck>         AuthoredChecks;
	TArray<FTryllLabCheckOutcome>  Failed;

	/**
	 * Peer turn for A/B prompt diff. Shared pointer (not TOptional&lt;FTryllTurnInspection&gt;)
	 * so this type can be complete before FTryllTurnInspection is defined.
	 */
	TSharedPtr<FTryllTurnInspection> DiffPeer;

	/** Empty-state copy when nothing is selected but a run exists. */
	bool bShowLabEmptyPlaceholder = true;
};

/**
 * Neutral per-turn input for the shared diagnostics inspector. Usable by Dialog Lab
 * and Chat without either importing the other's dataset types.
 */
struct FTryllTurnInspection
{
	/** Local stable key (Lab cell key, or chat turn id). Used for node-scope reset. */
	FString TurnKey;

	/** Hint under the title, e.g. "A1 · turn 3 · seed 42" or "turn 2". */
	FString Hint;

	/** Route section trailing when unscoped — variant label or empty. */
	FString RouteTrailing;

	FString AnswerText;
	FString ErrorText;
	ETryllTurnTerminalStatus Status = ETryllTurnTerminalStatus::Success;
	TArray<FTryllTurnRouteNode> Route;
	int32 DurationMs = 0;
	int32 TtftMs     = 0;

	/** Raw schema-v2 debug_info, or null when absent. */
	TSharedPtr<FJsonObject> DebugInfo;

	/** Original diagnostics payload, retained even when parsing or schema validation fails. */
	FString RawDebugInfoText;

	ETryllDiagnosticsAbsenceReason AbsenceReason = ETryllDiagnosticsAbsenceReason::Unavailable;

	/** Unset outside Dialog Lab — pins, checks, and A/B peer are omitted. */
	TOptional<FTryllTurnLabContext> Lab;
};

/** Builds and validates FTryllTurnInspection payloads. */
struct TRYLLCLIENTEDITOR_API FTryllTurnInspectionFactory
{
	static constexpr int32 SupportedSchemaVersion = 2;

	static FTryllTurnInspection Empty(const FString& Hint = TEXT("no run yet"));

	static FTryllTurnInspection FromLab(
		const FTryllDialogLabDataset* Dataset,
		const FString& SelectedKey,
		const FString& PinA,
		const FString& PinB,
		TFunction<void(const FString&)> OnClearPin);

	static FTryllTurnInspection FromChatTurn(
		const FString& TurnKey,
		const FString& Hint,
		const FString& AnswerText,
		const FString& ErrorText,
		ETryllTurnTerminalStatus Status,
		const TArray<FTryllTurnRouteNode>& Route,
		int32 DurationMs,
		int32 TtftMs,
		const TSharedPtr<FJsonObject>& DebugInfo,
		const FString& RawDebugInfoText,
		ETryllDiagnosticsAbsenceReason FallbackAbsence);

	static ETryllDiagnosticsAbsenceReason AbsenceFromLabCell(
		const FTryllLabCellView& Cell);

	static ETryllDiagnosticsAbsenceReason ClassifySchema(const TSharedPtr<FJsonObject>& DebugInfo);

	static ETryllTurnTerminalStatus StatusFromLab(ETryllLabCellStatus Status);

	static void ApplySchemaGate(
		FTryllTurnInspection& Turn, ETryllDiagnosticsAbsenceReason FallbackAbsence);

	static bool HasUsableDiagnostics(const FTryllTurnInspection& Turn);

private:
	static FTryllTurnInspection FromCellCore(const FTryllLabCellView& Cell);
};
