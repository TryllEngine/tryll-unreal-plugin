// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllDialogLabChecks.h"
#include "TryllDialogLabRunData.h"

struct FTryllLabVariantView
{
	FString      Id;
	FString      Label;
	FLinearColor Color = FLinearColor::White;
	/** "agent X, model Y, stateless, Node.Field" — or the literal "baseline". */
	FString      DiffLabel;
	FString      AgentName;
};

struct FTryllLabSampleView
{
	FString      Id;
	FString      VariantId;
	int32        Seed = 0;
	FLinearColor Color = FLinearColor::White;
	/** Repeat 0 — drives the variant-boundary gap between Summary dots. */
	bool         bFirstOfVariant = false;
};

/** One step row. Turns is flat across dialogues; these say where it came from. */
struct FTryllLabTurnView
{
	FString User;
	bool    bSeeded = false;
	TArray<FTryllLabCheck> Checks;
	int32   DialogueIndex = 0;
	/** Step index within its dialogue (what the UI numbers from). */
	int32   StepIndex = 0;
};

/** One dialogue's contiguous slice of FTryllDialogLabDataset::Turns. */
struct FTryllLabDialogueView
{
	FString Name;
	int32   FirstTurn   = 0;
	int32   TurnCount   = 0;
	int32   SeededTurns = 0;
};

struct FTryllLabRouteNode
{
	FString Name;
	FString Type;
	FString Exit;
	int32   Ms = 0;
};

struct FTryllLabCellView
{
	FString Key;
	FString SampleId;
	/** Index into the flat Turns; also the cell key's turn part. */
	int32   TurnIndex = 0;
	int32   DialogueIndex = 0;
	int32   StepIndex = 0;
	bool    bSeeded = false;
	/** An Error cell shows its message here, not its (empty) answer. */
	FString Text;
	int32   Words = 0;
	ETryllLabCellStatus Status = ETryllLabCellStatus::Skipped;
	TArray<FTryllLabRouteNode> Route;
	int32   Ms   = 0;
	int32   Ttft = 0;
	TArray<FTryllLabCheckOutcome> Failed;
	/** Every evaluated check, passes included. */
	TArray<FTryllLabCheckOutcome> Outcomes;
	TSharedPtr<FJsonObject> RawDebugInfo;
};

/**
 * A completed or imported run reshaped into the flat per-cell form the views iterate. Grid and
 * Summary read only this, never the run result.
 */
struct TRYLLCLIENTEDITOR_API FTryllDialogLabDataset
{
	TArray<FTryllLabVariantView> Variants;
	TArray<FTryllLabSampleView>  Samples;
	/** Every shown dialogue's steps, back to back. */
	TArray<FTryllLabTurnView>    Turns;
	TArray<FTryllLabDialogueView> Dialogues;
	/** How many dialogues the run has in total; more than Dialogues.Num() while a run is still
	 *  publishing dialogue by dialogue. */
	int32 TotalDialogues = 0;
	TMap<FString, FTryllLabCellView> Cells;

	int32   Repeats = 0;
	FString AgentName;
	bool    bImported = false;

	/** VisibleDialogues: show only the first N dialogues (a run still in progress); -1 = all. */
	static FTryllDialogLabDataset FromRunResult(
		const FTryllDialogLabRunResult& Run, bool bInImported = false, int32 VisibleDialogues = -1);

	/** Re-derives status: a stored Ok is never trusted over a non-empty failure list. */
	static ETryllLabCellStatus StatusOf(const FTryllLabCellView& Cell);

	static FString VariantId(int32 Index);
	static FString CellKey(const FString& SampleId, int32 TurnIndex);
};
