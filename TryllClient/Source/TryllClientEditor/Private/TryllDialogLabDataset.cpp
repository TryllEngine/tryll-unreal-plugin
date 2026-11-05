// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogLabDataset.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

namespace
{
	TArray<FTryllLabRouteNode> ExtractRoute(const TSharedPtr<FJsonObject>& DebugInfo)
	{
		TArray<FTryllLabRouteNode> Route;
		if (!DebugInfo.IsValid())
		{
			return Route;
		}
		const TArray<TSharedPtr<FJsonValue>>* Nodes = nullptr;
		if (!DebugInfo->TryGetArrayField(TEXT("nodes"), Nodes) || Nodes == nullptr)
		{
			return Route;
		}
		for (const TSharedPtr<FJsonValue>& Entry : *Nodes)
		{
			const TSharedPtr<FJsonObject>* Node = nullptr;
			if (!Entry.IsValid() || !Entry->TryGetObject(Node) || Node == nullptr)
			{
				continue;
			}
			FTryllLabRouteNode RouteNode;
			(*Node)->TryGetStringField(TEXT("name"), RouteNode.Name);
			(*Node)->TryGetStringField(TEXT("_type"), RouteNode.Type);
			(*Node)->TryGetStringField(TEXT("exit_route"), RouteNode.Exit);
			double Seconds = 0.0;
			if ((*Node)->TryGetNumberField(TEXT("duration_s"), Seconds))
			{
				RouteNode.Ms = static_cast<int32>(Seconds * 1000.0);
			}
			Route.Add(MoveTemp(RouteNode));
		}
		return Route;
	}

	FString BuildDiffLabel(const FTryllLabVariantSnapshot& Variant)
	{
		TArray<FString> Parts;
		// Agent first: with per-variant agents it is the coarsest difference between columns.
		if (Variant.bAgentIsOverride)
		{
			Parts.Add(TEXT("agent ") + Variant.AgentName);
		}
		// The headline when variants compare models, so it precedes the parameter overrides.
		if (!Variant.ModelOverride.IsEmpty())
		{
			Parts.Add(TEXT("model ") + Variant.ModelOverride);
		}
		// Worth a chip: it changes what every turn after the first one sees, and it is
		// invisible in both the graph and the overrides.
		if (!Variant.bMaintainDialogueHistory)
		{
			Parts.Add(TEXT("stateless"));
		}
		if (Variant.bUseLiveTools)
		{
			Parts.Add(TEXT("live tools"));
		}
		for (const FTryllLabParamOverride& Override : Variant.Overrides)
		{
			Parts.Add(FString::Printf(TEXT("%s.%s"), *Override.NodeName, *Override.FieldPath));
		}
		return Parts.Num() > 0 ? FString::Join(Parts, TEXT(", ")) : FString(TEXT("baseline"));
	}
}

FString FTryllDialogLabDataset::VariantId(int32 Index)
{
	return FString::Chr(static_cast<TCHAR>(TEXT('A') + Index));
}

FString FTryllDialogLabDataset::CellKey(const FString& SampleId, int32 TurnIndex)
{
	return FString::Printf(TEXT("%s:%d"), *SampleId, TurnIndex);
}

ETryllLabCellStatus FTryllDialogLabDataset::StatusOf(const FTryllLabCellView& Cell)
{
	if (Cell.Status == ETryllLabCellStatus::Error || Cell.Status == ETryllLabCellStatus::Skipped)
	{
		return Cell.Status;
	}
	return Cell.Failed.Num() > 0 ? ETryllLabCellStatus::CheckFail : ETryllLabCellStatus::Ok;
}

FTryllDialogLabDataset FTryllDialogLabDataset::FromRunResult(
	const FTryllDialogLabRunResult& Run, bool bInImported, int32 VisibleDialogues)
{
	FTryllDialogLabDataset Dataset;
	Dataset.Repeats        = Run.Snapshot.Repeats;
	Dataset.AgentName      = Run.Snapshot.DefaultAgentName;
	Dataset.TotalDialogues = Run.Snapshot.Dialogues.Num();
	Dataset.bImported      = bInImported;
	const int32 Shown = VisibleDialogues < 0
		? Run.Snapshot.Dialogues.Num()
		: FMath::Clamp(VisibleDialogues, 0, Run.Snapshot.Dialogues.Num());

	for (int32 Index = 0; Index < Run.Snapshot.Variants.Num(); ++Index)
	{
		const FTryllLabVariantSnapshot& Snapshot = Run.Snapshot.Variants[Index];
		FTryllLabVariantView View;
		View.Id        = VariantId(Index);
		View.Label     = Snapshot.Label;
		View.Color     = Snapshot.Swatch;
		View.DiffLabel = BuildDiffLabel(Snapshot);
		View.AgentName = Snapshot.AgentName;
		Dataset.Variants.Add(MoveTemp(View));
	}

	for (int32 D = 0; D < Shown; ++D)
	{
		const FTryllLabScriptSnapshot& Script = Run.Snapshot.Dialogues[D];
		FTryllLabDialogueView Dialogue;
		Dialogue.Name        = Script.Name;
		Dialogue.FirstTurn   = Dataset.Turns.Num();
		Dialogue.TurnCount   = Script.Steps.Num();
		Dialogue.SeededTurns = Script.SeedTurnCount;
		Dataset.Dialogues.Add(MoveTemp(Dialogue));
		for (int32 Index = 0; Index < Script.Steps.Num(); ++Index)
		{
			const FTryllLabStepSnapshot& Step = Script.Steps[Index];
			FTryllLabTurnView View;
			View.User          = Step.User;
			View.bSeeded       = Index < Script.SeedTurnCount;
			View.Checks        = FTryllDialogLabChecks::AuthoredChecks(Script, Step, View.bSeeded);
			View.DialogueIndex = D;
			View.StepIndex     = Index;
			Dataset.Turns.Add(MoveTemp(View));
		}
	}
	const int32 ShownTurns = Dataset.Turns.Num();

	for (const FTryllLabSampleResult& Sample : Run.Samples)
	{
		FTryllLabSampleView View;
		View.Id              = Sample.SampleId;
		View.VariantId       = VariantId(Sample.VariantIndex);
		View.Seed            = Sample.Seed;
		View.Color           = Dataset.Variants.IsValidIndex(Sample.VariantIndex)
			? Dataset.Variants[Sample.VariantIndex].Color
			: FLinearColor::White;
		View.bFirstOfVariant = Sample.Repeat == 0;
		Dataset.Samples.Add(MoveTemp(View));

		// A sample's records are flat in dialogue order, like Turns.
		for (int32 Flat = 0; Flat < Sample.Turns.Num() && Flat < ShownTurns; ++Flat)
		{
			const FTryllLabTurnRecord& Record = Sample.Turns[Flat];
			FTryllLabCellView Cell;
			Cell.Key           = CellKey(Sample.SampleId, Flat);
			Cell.SampleId      = Sample.SampleId;
			Cell.TurnIndex     = Flat;
			Cell.DialogueIndex = Record.DialogueIndex;
			Cell.StepIndex     = Record.TurnIndex;
			Cell.bSeeded       = Record.bSeeded;
			Cell.Text      = Record.Status == ETryllLabCellStatus::Error
				? (Record.ErrorMessage.IsEmpty() ? FString(TEXT("Error")) : Record.ErrorMessage)
				: Record.AnswerText;
			Cell.Words        = Record.Words;
			Cell.Status       = Record.Status;
			Cell.Route        = ExtractRoute(Record.DebugInfo);
			Cell.Ms           = Record.DurationMs;
			Cell.Ttft         = Record.TtftMs;
			Cell.Failed       = Record.FailedChecks();
			Cell.Outcomes     = Record.CheckOutcomes;
			Cell.RawDebugInfo = Record.DebugInfo;
			Dataset.Cells.Add(Cell.Key, MoveTemp(Cell));
		}
	}

	return Dataset;
}
