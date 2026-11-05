// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllTurnInspectorModels.h"

#include "TryllDialogLabDataset.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

FTryllTurnInspection FTryllTurnInspectionFactory::Empty(const FString& Hint)
{
	FTryllTurnInspection Turn;
	Turn.Hint          = Hint;
	Turn.AbsenceReason = ETryllDiagnosticsAbsenceReason::Unavailable;
	return Turn;
}

FTryllTurnInspection FTryllTurnInspectionFactory::FromLab(
	const FTryllDialogLabDataset* Dataset,
	const FString& SelectedKey,
	const FString& PinA,
	const FString& PinB,
	TFunction<void(const FString&)> OnClearPin)
{
	FTryllTurnLabContext LabShell;
	LabShell.PinA       = PinA;
	LabShell.PinB       = PinB;
	LabShell.OnClearPin = MoveTemp(OnClearPin);

	if (Dataset == nullptr)
	{
		FTryllTurnInspection Turn = Empty(TEXT("no run yet"));
		LabShell.bShowLabEmptyPlaceholder = true;
		Turn.Lab = MoveTemp(LabShell);
		return Turn;
	}

	if (SelectedKey.IsEmpty())
	{
		FTryllTurnInspection Turn = Empty(TEXT("click a cell"));
		LabShell.bShowLabEmptyPlaceholder = true;
		Turn.Lab = MoveTemp(LabShell);
		return Turn;
	}

	const FTryllLabCellView* Cell = Dataset->Cells.Find(SelectedKey);
	if (Cell == nullptr)
	{
		FTryllTurnInspection Turn = Empty(TEXT("click a cell"));
		LabShell.bShowLabEmptyPlaceholder = true;
		Turn.Lab = MoveTemp(LabShell);
		return Turn;
	}

	const FTryllLabSampleView* Sample = Dataset->Samples.FindByPredicate(
		[Cell](const FTryllLabSampleView& S) { return S.Id == Cell->SampleId; });
	const FTryllLabVariantView* Variant = (Sample != nullptr)
		? Dataset->Variants.FindByPredicate(
			[Sample](const FTryllLabVariantView& V) { return V.Id == Sample->VariantId; })
		: nullptr;

	FTryllTurnInspection Turn = FromCellCore(*Cell);
	Turn.TurnKey = Cell->Key;
	const FString Dialogue = Dataset->TotalDialogues > 1 && Dataset->Dialogues.IsValidIndex(Cell->DialogueIndex)
		? Dataset->Dialogues[Cell->DialogueIndex].Name + TEXT(" · ")
		: FString();
	if (Sample != nullptr)
	{
		Turn.Hint = FString::Printf(TEXT("%s · %sturn %d · seed %d"),
			*Cell->SampleId, *Dialogue, Cell->StepIndex + 1, Sample->Seed);
	}
	else
	{
		Turn.Hint = FString::Printf(TEXT("%s · %sturn %d"), *Cell->SampleId, *Dialogue, Cell->StepIndex + 1);
	}
	Turn.RouteTrailing = Variant != nullptr ? Variant->Label : FString();

	const bool bDiffing = !PinA.IsEmpty() && !PinB.IsEmpty();
	if (bDiffing)
	{
		const FString OtherSample = Cell->SampleId == PinA ? PinB : PinA;
		if (const FTryllLabCellView* OtherCell = Dataset->Cells.Find(
			FTryllDialogLabDataset::CellKey(OtherSample, Cell->TurnIndex)))
		{
			LabShell.DiffPeer = MakeShared<FTryllTurnInspection>(
				FromCellCore(*OtherCell));
		}
	}

	if (Dataset->Turns.IsValidIndex(Cell->TurnIndex))
	{
		LabShell.AuthoredChecks = Dataset->Turns[Cell->TurnIndex].Checks;
	}
	LabShell.Failed                     = Cell->Failed;
	LabShell.bShowLabEmptyPlaceholder   = false;
	Turn.Lab = MoveTemp(LabShell);
	return Turn;
}

FTryllTurnInspection FTryllTurnInspectionFactory::FromChatTurn(
	const FString& TurnKey,
	const FString& Hint,
	const FString& AnswerText,
	const FString& ErrorText,
	const ETryllTurnTerminalStatus Status,
	const TArray<FTryllTurnRouteNode>& Route,
	const int32 DurationMs,
	const int32 TtftMs,
	const TSharedPtr<FJsonObject>& DebugInfo,
	const FString& RawDebugInfoText,
	const ETryllDiagnosticsAbsenceReason FallbackAbsence)
{
	FTryllTurnInspection Turn;
	Turn.TurnKey          = TurnKey;
	Turn.Hint             = Hint;
	Turn.AnswerText       = AnswerText;
	Turn.ErrorText        = ErrorText;
	Turn.Status           = Status;
	Turn.Route            = Route;
	Turn.DurationMs       = DurationMs;
	Turn.TtftMs           = TtftMs;
	Turn.DebugInfo        = DebugInfo;
	Turn.RawDebugInfoText = RawDebugInfoText;
	ApplySchemaGate(Turn, FallbackAbsence);
	return Turn;
}

ETryllDiagnosticsAbsenceReason FTryllTurnInspectionFactory::AbsenceFromLabCell(
	const FTryllLabCellView& Cell)
{
	if (Cell.bSeeded)
	{
		return ETryllDiagnosticsAbsenceReason::Seeded;
	}
	if (Cell.Status == ETryllLabCellStatus::Skipped)
	{
		return ETryllDiagnosticsAbsenceReason::Skipped;
	}
	if (!Cell.RawDebugInfo.IsValid())
	{
		return ETryllDiagnosticsAbsenceReason::Unavailable;
	}
	return ClassifySchema(Cell.RawDebugInfo);
}

ETryllDiagnosticsAbsenceReason FTryllTurnInspectionFactory::ClassifySchema(
	const TSharedPtr<FJsonObject>& DebugInfo)
{
	if (!DebugInfo.IsValid())
	{
		return ETryllDiagnosticsAbsenceReason::Empty;
	}

	const TSharedPtr<FJsonValue>* VerField = DebugInfo->Values.Find(TEXT("schema_version"));
	if (VerField == nullptr || !VerField->IsValid() || (*VerField)->Type == EJson::Null)
	{
		return ETryllDiagnosticsAbsenceReason::UnsupportedSchema;
	}

	double Ver = 0.0;
	if ((*VerField)->Type == EJson::Number)
	{
		Ver = (*VerField)->AsNumber();
	}
	else
	{
		return ETryllDiagnosticsAbsenceReason::UnsupportedSchema;
	}

	if (!FMath::IsFinite(Ver) || Ver != static_cast<double>(SupportedSchemaVersion))
	{
		return ETryllDiagnosticsAbsenceReason::UnsupportedSchema;
	}

	return ETryllDiagnosticsAbsenceReason::None;
}

ETryllTurnTerminalStatus FTryllTurnInspectionFactory::StatusFromLab(
	const ETryllLabCellStatus Status)
{
	switch (Status)
	{
	case ETryllLabCellStatus::Error:
		return ETryllTurnTerminalStatus::Error;
	case ETryllLabCellStatus::Skipped:
		return ETryllTurnTerminalStatus::Skipped;
	default:
		return ETryllTurnTerminalStatus::Success;
	}
}

void FTryllTurnInspectionFactory::ApplySchemaGate(
	FTryllTurnInspection& Turn, const ETryllDiagnosticsAbsenceReason FallbackAbsence)
{
	if (!Turn.DebugInfo.IsValid())
	{
		Turn.AbsenceReason = FallbackAbsence;
		return;
	}

	const ETryllDiagnosticsAbsenceReason Schema = ClassifySchema(Turn.DebugInfo);
	if (Schema != ETryllDiagnosticsAbsenceReason::None)
	{
		Turn.AbsenceReason = Schema;
		return;
	}

	Turn.AbsenceReason = ETryllDiagnosticsAbsenceReason::None;
}

bool FTryllTurnInspectionFactory::HasUsableDiagnostics(const FTryllTurnInspection& Turn)
{
	return Turn.AbsenceReason == ETryllDiagnosticsAbsenceReason::None
		&& Turn.DebugInfo.IsValid();
}

FTryllTurnInspection FTryllTurnInspectionFactory::FromCellCore(
	const FTryllLabCellView& Cell)
{
	TArray<FTryllTurnRouteNode> Mapped;
	Mapped.Reserve(Cell.Route.Num());
	for (const FTryllLabRouteNode& Node : Cell.Route)
	{
		FTryllTurnRouteNode Out;
		Out.Name = Node.Name;
		Out.Type = Node.Type;
		Out.Exit = Node.Exit;
		Out.Ms   = Node.Ms;
		Mapped.Add(MoveTemp(Out));
	}

	FTryllTurnInspection Turn;
	const ETryllLabCellStatus LabStatus = FTryllDialogLabDataset::StatusOf(Cell);
	Turn.AnswerText = Cell.Text;
	Turn.ErrorText  = LabStatus == ETryllLabCellStatus::Error
		|| LabStatus == ETryllLabCellStatus::Skipped
			? Cell.Text
			: FString();
	Turn.Status     = StatusFromLab(LabStatus);
	Turn.Route      = MoveTemp(Mapped);
	Turn.DurationMs = Cell.Ms;
	Turn.TtftMs     = Cell.Ttft;
	Turn.DebugInfo  = Cell.RawDebugInfo;

	const ETryllDiagnosticsAbsenceReason Absence = AbsenceFromLabCell(Cell);
	if (Absence == ETryllDiagnosticsAbsenceReason::None
		|| Absence == ETryllDiagnosticsAbsenceReason::UnsupportedSchema
		|| Absence == ETryllDiagnosticsAbsenceReason::Empty)
	{
		ApplySchemaGate(Turn,
			Absence == ETryllDiagnosticsAbsenceReason::None
				? ETryllDiagnosticsAbsenceReason::Empty
				: Absence);
	}
	else
	{
		Turn.AbsenceReason = Absence;
	}

	return Turn;
}
