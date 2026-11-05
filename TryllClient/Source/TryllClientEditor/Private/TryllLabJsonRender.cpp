// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllLabJsonRender.h"

#include "TryllDialogLabDataset.h"
#include "TryllTurnInspectorModels.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	FString EscapeForJson(const FString& In)
	{
		FString Out = In;
		Out.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
		Out.ReplaceInline(TEXT("\""), TEXT("\\\""));
		Out.ReplaceInline(TEXT("\n"), TEXT("\\n"));
		return Out;
	}

	/** Numbers render without a trailing ".0" so whole values read as integers, matching .NET. */
	FString NumberToString(double Value)
	{
		const double Rounded = FMath::RoundToDouble(Value);
		return FMath::IsNearlyEqual(Value, Rounded)
			? FString::Printf(TEXT("%lld"), static_cast<int64>(Rounded))
			: FString::SanitizeFloat(Value);
	}
}

TArray<FTryllLabPromptSegment> FTryllLabJsonRender::PromptSegments(
	const TSharedPtr<FJsonObject>& DebugInfo)
{
	TArray<FTryllLabPromptSegment> Segments;
	if (!DebugInfo.IsValid())
	{
		return Segments;
	}

	const TArray<TSharedPtr<FJsonValue>>* Nodes = nullptr;
	if (!DebugInfo->TryGetArrayField(TEXT("nodes"), Nodes) || Nodes == nullptr)
	{
		return Segments;
	}

	for (int32 Index = 0; Index < Nodes->Num(); ++Index)
	{
		const TSharedPtr<FJsonObject>* Node = nullptr;
		if (!(*Nodes)[Index].IsValid() || !(*Nodes)[Index]->TryGetObject(Node) || Node == nullptr)
		{
			continue;
		}

		FTryllLabPromptSegment Segment;
		// The array position, not the name: a node re-entered within a turn appears more than
		// once, and the index is what tells the two executions apart.
		Segment.NodeIndex = Index;
		(*Node)->TryGetStringField(TEXT("name"), Segment.NodeName);
		(*Node)->TryGetStringField(TEXT("_type"), Segment.NodeType);
		(*Node)->TryGetStringField(TEXT("exit_route"), Segment.ExitRoute);

		const TSharedPtr<FJsonObject>* Diagnostics = nullptr;
		if ((*Node)->TryGetObjectField(TEXT("diagnostics"), Diagnostics) && Diagnostics != nullptr)
		{
			const TSharedPtr<FJsonObject>* Input = nullptr;
			if ((*Diagnostics)->TryGetObjectField(TEXT("input"), Input) && Input != nullptr)
			{
				const TArray<TSharedPtr<FJsonValue>>* Prompt = nullptr;
				if ((*Input)->TryGetArrayField(TEXT("prompt"), Prompt) && Prompt != nullptr)
				{
					for (const TSharedPtr<FJsonValue>& MessageValue : *Prompt)
					{
						const TSharedPtr<FJsonObject>* Message = nullptr;
						if (!MessageValue.IsValid() || !MessageValue->TryGetObject(Message)
							|| Message == nullptr)
						{
							continue;
						}
						FTryllLabPromptMessage Entry;
						(*Message)->TryGetStringField(TEXT("role"), Entry.Role);
						(*Message)->TryGetStringField(TEXT("content"), Entry.Content);
						// An empty entry must not emit a bare role header.
						if (Entry.Content.IsEmpty())
						{
							continue;
						}
						Segment.Prompt.Add(MoveTemp(Entry));
					}
				}
			}

			const TSharedPtr<FJsonObject>* Output = nullptr;
			if ((*Diagnostics)->TryGetObjectField(TEXT("output"), Output) && Output != nullptr)
			{
				(*Output)->TryGetStringField(TEXT("text"), Segment.OutputText);
			}
		}

		if (Segment.HasContent())
		{
			Segments.Add(MoveTemp(Segment));
		}
	}
	return Segments;
}

FString FTryllLabJsonRender::SegmentHeader(const FTryllLabPromptSegment& Segment)
{
	FString Out = FString::Printf(TEXT("── #%d %s"), Segment.NodeIndex, *Segment.NodeName);
	if (!Segment.NodeType.IsEmpty())
	{
		Out += FString::Printf(TEXT(" (%s)"), *Segment.NodeType);
	}
	if (!Segment.ExitRoute.IsEmpty())
	{
		Out += FString::Printf(TEXT(" [%s]"), *Segment.ExitRoute);
	}
	Out += TEXT(" ──");
	return Out;
}

FString FTryllLabJsonRender::RenderSegmentBody(const FTryllLabPromptSegment& Segment)
{
	FString Out;
	for (const FTryllLabPromptMessage& Message : Segment.Prompt)
	{
		Out += FString::Printf(TEXT("<|%s|>\n%s\n\n"), *Message.Role, *Message.Content);
	}
	if (!Segment.OutputText.IsEmpty())
	{
		Out += FString::Printf(TEXT("<|assistant|>\n%s\n\n"), *Segment.OutputText);
	}
	return Out.TrimEnd();
}

FString FTryllLabJsonRender::RenderSegments(const TArray<FTryllLabPromptSegment>& Segments)
{
	if (Segments.Num() == 0)
	{
		return TEXT("(no prompt data on this turn)");
	}

	// Headings only when there is more than one producer to tell apart.
	const bool bLabel = Segments.Num() > 1;
	TArray<FString> Parts;
	for (const FTryllLabPromptSegment& Segment : Segments)
	{
		const FString Body = RenderSegmentBody(Segment);
		Parts.Add(bLabel ? SegmentHeader(Segment) + TEXT("\n") + Body : Body);
	}
	return FString::Join(Parts, TEXT("\n\n"));
}

TArray<FTryllLabSegmentDiff> FTryllLabJsonRender::DiffSegments(
	const TArray<FTryllLabPromptSegment>& Mine,
	const TArray<FTryllLabPromptSegment>& Theirs)
{
	auto Paired = [](const FTryllLabPromptSegment& A, const FTryllLabPromptSegment& B)
	{
		FTryllLabSegmentDiff Diff;
		Diff.Pairing = ETryllLabSegmentPairing::Both;
		Diff.Mine    = A;
		Diff.Theirs  = B;
		Diff.Lines   = DiffLines(RenderSegmentBody(A), RenderSegmentBody(B));
		return Diff;
	};
	auto OneSided = [](const FTryllLabPromptSegment& Segment, bool bIsMine)
	{
		FTryllLabSegmentDiff Diff;
		Diff.Pairing = bIsMine ? ETryllLabSegmentPairing::OnlyMine
							   : ETryllLabSegmentPairing::OnlyTheirs;
		(bIsMine ? Diff.Mine : Diff.Theirs) = Segment;

		TArray<FString> Lines;
		RenderSegmentBody(Segment).ParseIntoArray(Lines, TEXT("\n"), false);
		for (const FString& Line : Lines)
		{
			Diff.Lines.Add({bIsMine ? ETryllLabDiffOp::Added : ETryllLabDiffOp::Removed, Line});
		}
		return Diff;
	};

	const TArray<FTryllLabPromptSegment>& A = Theirs;
	const TArray<FTryllLabPromptSegment>& B = Mine;
	TArray<FTryllLabSegmentDiff> Result;

	// LCS over the pairing keys, so a route that gained or lost a node reports that node on one
	// side only rather than sliding every later segment out of alignment.
	const int32 N = A.Num();
	const int32 M = B.Num();
	TArray<int32> Lcs;
	Lcs.SetNumZeroed((N + 1) * (M + 1));
	auto At = [M](int32 I, int32 J) { return I * (M + 1) + J; };

	for (int32 I = N - 1; I >= 0; --I)
	{
		for (int32 J = M - 1; J >= 0; --J)
		{
			Lcs[At(I, J)] = A[I].PairKey() == B[J].PairKey()
				? Lcs[At(I + 1, J + 1)] + 1
				: FMath::Max(Lcs[At(I + 1, J)], Lcs[At(I, J + 1)]);
		}
	}

	int32 X = 0;
	int32 Y = 0;
	while (X < N && Y < M)
	{
		if (A[X].PairKey() == B[Y].PairKey())
		{
			Result.Add(Paired(B[Y], A[X]));
			++X;
			++Y;
		}
		else if (Lcs[At(X + 1, Y)] >= Lcs[At(X, Y + 1)])
		{
			Result.Add(OneSided(A[X], /*bIsMine*/ false));
			++X;
		}
		else
		{
			Result.Add(OneSided(B[Y], /*bIsMine*/ true));
			++Y;
		}
	}
	while (X < N) { Result.Add(OneSided(A[X], /*bIsMine*/ false)); ++X; }
	while (Y < M) { Result.Add(OneSided(B[Y], /*bIsMine*/ true));  ++Y; }
	return Result;
}

FString FTryllLabJsonRender::RenderedPromptFromJson(const TSharedPtr<FJsonObject>& DebugInfo)
{
	return RenderSegments(PromptSegments(DebugInfo));
}

FString FTryllLabJsonRender::NoDiagnosticsNote(const ETryllDiagnosticsAbsenceReason Reason)
{
	switch (Reason)
	{
		case ETryllDiagnosticsAbsenceReason::Seeded:
			return TEXT("Seeded turn — injected with AppendInteractions, so the graph never ran and ")
			       TEXT("there are no diagnostics for it.");
		case ETryllDiagnosticsAbsenceReason::Skipped:
			return TEXT("Not run — an earlier turn in this column errored, so this one never executed.");
		case ETryllDiagnosticsAbsenceReason::UnsupportedSchema:
			return TEXT("Unsupported diagnostics schema — this inspector renders schema_version 2 only.");
		case ETryllDiagnosticsAbsenceReason::Empty:
			return TEXT("Diagnostics payload was empty or unreadable.");
		case ETryllDiagnosticsAbsenceReason::CreatedBeforeDiagnosticsCapture:
			return TEXT("Created before diagnostics capture — this agent was already running when ")
			       TEXT("Agent Log capture started, so earlier traffic was not recorded and per-turn ")
			       TEXT("debug_info is unavailable for those turns.");
		case ETryllDiagnosticsAbsenceReason::Unavailable:
		case ETryllDiagnosticsAbsenceReason::None:
		default:
			return TEXT("No diagnostics for this turn.");
	}
}

FString FTryllLabJsonRender::NoDiagnosticsNote(const FTryllLabCellView& Cell)
{
	return NoDiagnosticsNote(FTryllTurnInspectionFactory::AbsenceFromLabCell(Cell));
}

FString FTryllLabJsonRender::JsonScalarToString(const TSharedPtr<FJsonValue>& Value)
{
	if (!Value.IsValid())
	{
		return FString();
	}
	switch (Value->Type)
	{
		case EJson::String: return Value->AsString();
		case EJson::Number: return NumberToString(Value->AsNumber());
		case EJson::Boolean: return Value->AsBool() ? TEXT("true") : TEXT("false");
		case EJson::Null: return TEXT("null");
		case EJson::Array: return FString::Printf(TEXT("[%d]"), Value->AsArray().Num());
		case EJson::Object:
			return FString::Printf(TEXT("{%d}"), Value->AsObject()->Values.Num());
		default: return FString();
	}
}

FString FTryllLabJsonRender::PrettyPrintJson(const TSharedPtr<FJsonValue>& Value, int32 Indent)
{
	if (!Value.IsValid())
	{
		return TEXT("null");
	}

	const FString Pad      = FString::ChrN(Indent * 2, TEXT(' '));
	const FString ChildPad = FString::ChrN((Indent + 1) * 2, TEXT(' '));

	switch (Value->Type)
	{
		case EJson::Object:
		{
			const TSharedPtr<FJsonObject>& Object = Value->AsObject();
			if (!Object.IsValid() || Object->Values.Num() == 0)
			{
				return TEXT("{}");
			}
			TArray<FString> Lines;
			for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Object->Values)
			{
				Lines.Add(FString::Printf(TEXT("%s\"%s\": %s"), *ChildPad,
					*EscapeForJson(Pair.Key), *PrettyPrintJson(Pair.Value, Indent + 1)));
			}
			return TEXT("{\n") + FString::Join(Lines, TEXT(",\n")) + TEXT("\n") + Pad + TEXT("}");
		}
		case EJson::Array:
		{
			const TArray<TSharedPtr<FJsonValue>> Items = Value->AsArray();
			if (Items.Num() == 0)
			{
				return TEXT("[]");
			}
			TArray<FString> Lines;
			for (const TSharedPtr<FJsonValue>& Item : Items)
			{
				Lines.Add(ChildPad + PrettyPrintJson(Item, Indent + 1));
			}
			return TEXT("[\n") + FString::Join(Lines, TEXT(",\n")) + TEXT("\n") + Pad + TEXT("]");
		}
		case EJson::String:
			return FString::Printf(TEXT("\"%s\""), *EscapeForJson(Value->AsString()));
		case EJson::Number:
			return NumberToString(Value->AsNumber());
		case EJson::Boolean:
			return Value->AsBool() ? TEXT("true") : TEXT("false");
		default:
			return TEXT("null");
	}
}

FString FTryllLabJsonRender::PrettyPrintJson(const TSharedPtr<FJsonObject>& Object, int32 Indent)
{
	return Object.IsValid()
		? PrettyPrintJson(MakeShared<FJsonValueObject>(Object), Indent)
		: FString(TEXT("null"));
}

FString FTryllLabJsonRender::PrettyJsonOrRaw(const FString& Json)
{
	if (Json.IsEmpty())
	{
		return TEXT("{}");
	}
	TSharedPtr<FJsonValue> Parsed;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (FJsonSerializer::Deserialize(Reader, Parsed) && Parsed.IsValid())
	{
		return PrettyPrintJson(Parsed, 0);
	}
	return Json;
}

TArray<FTryllLabDiffLine> FTryllLabJsonRender::DiffLines(const FString& Mine, const FString& Theirs)
{
	TArray<FString> Baseline;  // Removed comes from here
	TArray<FString> Current;   // Added comes from here
	Theirs.ParseIntoArray(Baseline, TEXT("\n"), /*CullEmpty*/ false);
	Mine.ParseIntoArray(Current, TEXT("\n"), /*CullEmpty*/ false);

	TArray<FTryllLabDiffLine> Result;

	if (Baseline.Num() > MaxDiffLines || Current.Num() > MaxDiffLines)
	{
		TSet<FString> BaselineSet;
		TSet<FString> CurrentSet;
		for (const FString& Line : Baseline) { BaselineSet.Add(Line.TrimStartAndEnd()); }
		for (const FString& Line : Current)  { CurrentSet.Add(Line.TrimStartAndEnd()); }

		for (const FString& Line : Current)
		{
			Result.Add({BaselineSet.Contains(Line.TrimStartAndEnd())
				? ETryllLabDiffOp::Same : ETryllLabDiffOp::Added, Line});
		}
		for (const FString& Line : Baseline)
		{
			if (!CurrentSet.Contains(Line.TrimStartAndEnd()))
			{
				Result.Add({ETryllLabDiffOp::Removed, Line});
			}
		}
		return Result;
	}

	const int32 N = Baseline.Num();
	const int32 M = Current.Num();

	TArray<FString> BaselineTrimmed;
	TArray<FString> CurrentTrimmed;
	BaselineTrimmed.Reserve(N);
	CurrentTrimmed.Reserve(M);
	for (const FString& Line : Baseline) { BaselineTrimmed.Add(Line.TrimStartAndEnd()); }
	for (const FString& Line : Current)  { CurrentTrimmed.Add(Line.TrimStartAndEnd()); }

	// Lcs[i][j] = longest common subsequence of Baseline[i..] and Current[j..].
	TArray<int32> Lcs;
	Lcs.SetNumZeroed((N + 1) * (M + 1));
	auto At = [M](int32 I, int32 J) { return I * (M + 1) + J; };

	for (int32 I = N - 1; I >= 0; --I)
	{
		for (int32 J = M - 1; J >= 0; --J)
		{
			Lcs[At(I, J)] = BaselineTrimmed[I] == CurrentTrimmed[J]
				? Lcs[At(I + 1, J + 1)] + 1
				: FMath::Max(Lcs[At(I + 1, J)], Lcs[At(I, J + 1)]);
		}
	}

	int32 X = 0;
	int32 Y = 0;
	while (X < N && Y < M)
	{
		if (BaselineTrimmed[X] == CurrentTrimmed[Y])
		{
			Result.Add({ETryllLabDiffOp::Same, Current[Y]});
			++X;
			++Y;
		}
		else if (Lcs[At(X + 1, Y)] >= Lcs[At(X, Y + 1)])
		{
			Result.Add({ETryllLabDiffOp::Removed, Baseline[X]});
			++X;
		}
		else
		{
			Result.Add({ETryllLabDiffOp::Added, Current[Y]});
			++Y;
		}
	}
	while (X < N) { Result.Add({ETryllLabDiffOp::Removed, Baseline[X]}); ++X; }
	while (Y < M) { Result.Add({ETryllLabDiffOp::Added, Current[Y]}); ++Y; }

	return Result;
}
